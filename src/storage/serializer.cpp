#include "storage/serializer.h"

#include <cstring>
#include <fstream>
#include <iostream>

namespace {

template <typename T>
void write_val(std::ofstream& out, T val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template <typename T>
bool read_val(std::ifstream& in, T& val) {
    in.read(reinterpret_cast<char*>(&val), sizeof(T));
    return in.good();
}

void write_string(std::ofstream& out, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    write_val(out, len);
    out.write(s.data(), len);
}

bool read_string(std::ifstream& in, std::string& s) {
    uint16_t len;
    if (!read_val(in, len)) return false;
    s.resize(len);
    in.read(s.data(), len);
    return in.good();
}

}  // namespace

bool serialize_index(const std::string& path,
                     const inverted_index& inv_idx,
                     const forward_index& fwd_idx) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "error: could not open " << path << " for writing\n";
        return false;
    }

    // header
    write_val(out, INDEX_MAGIC);
    write_val(out, INDEX_VERSION);
    uint64_t num_terms = inv_idx.vocabulary_size();
    uint64_t num_docs = fwd_idx.doc_count();
    write_val(out, num_terms);
    write_val(out, num_docs);
    write_val(out, inv_idx.avg_doc_length);

    // forward index
    for (const auto& doc : fwd_idx.docs) {
        write_val(out, doc.doc_id);
        write_val(out, doc.length);
        write_string(out, doc.title);
    }

    // collect posting data into a flat buffer to compute offsets
    std::vector<uint8_t> posting_data;
    struct term_entry {
        std::string term;
        float idf;
        uint64_t offset;
        uint32_t count;
    };
    std::vector<term_entry> term_entries;

    for (const auto& [term, pl] : inv_idx.terms) {
        uint64_t offset = posting_data.size();
        uint32_t count = static_cast<uint32_t>(pl.entries.size());

        for (const auto& p : pl.entries) {
            auto* src = reinterpret_cast<const uint8_t*>(&p.doc_id);
            posting_data.insert(posting_data.end(), src, src + 4);
            src = reinterpret_cast<const uint8_t*>(&p.term_freq);
            posting_data.insert(posting_data.end(), src, src + 4);

#ifdef ENABLE_POSITIONS
            uint32_t num_pos = static_cast<uint32_t>(p.positions.size());
            src = reinterpret_cast<const uint8_t*>(&num_pos);
            posting_data.insert(posting_data.end(), src, src + 4);
            for (uint32_t pos : p.positions) {
                src = reinterpret_cast<const uint8_t*>(&pos);
                posting_data.insert(posting_data.end(), src, src + 4);
            }
#endif
        }

        term_entries.push_back({term, pl.idf, offset, count});
    }

    // term dictionary
    for (const auto& te : term_entries) {
        write_string(out, te.term);
        write_val(out, te.idf);
        write_val(out, te.offset);
        write_val(out, te.count);
    }

    // posting data
    out.write(reinterpret_cast<const char*>(posting_data.data()),
              static_cast<std::streamsize>(posting_data.size()));

    return out.good();
}

bool deserialize_index(const std::string& path,
                       inverted_index& inv_idx,
                       forward_index& fwd_idx) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "error: could not open " << path << " for reading\n";
        return false;
    }

    // header
    uint32_t magic, version;
    read_val(in, magic);
    read_val(in, version);

    if (magic != INDEX_MAGIC) {
        std::cerr << "error: invalid index file (bad magic)\n";
        return false;
    }
    if (version != INDEX_VERSION) {
        std::cerr << "error: unsupported index version " << version << "\n";
        return false;
    }

    uint64_t num_terms, num_docs;
    read_val(in, num_terms);
    read_val(in, num_docs);
    read_val(in, inv_idx.avg_doc_length);
    inv_idx.num_docs = static_cast<uint32_t>(num_docs);

    // forward index
    fwd_idx.docs.resize(num_docs);
    fwd_idx.total_tokens = 0;
    for (uint64_t i = 0; i < num_docs; ++i) {
        auto& doc = fwd_idx.docs[i];
        read_val(in, doc.doc_id);
        read_val(in, doc.length);
        read_string(in, doc.title);
        fwd_idx.total_tokens += doc.length;
    }

    // term dictionary + posting data
    // first read term entries to know offsets
    struct term_entry {
        std::string term;
        float idf;
        uint64_t offset;
        uint32_t count;
    };
    std::vector<term_entry> term_entries(num_terms);

    for (uint64_t i = 0; i < num_terms; ++i) {
        read_string(in, term_entries[i].term);
        read_val(in, term_entries[i].idf);
        read_val(in, term_entries[i].offset);
        read_val(in, term_entries[i].count);
    }

    // read remaining posting data
    auto post_start = in.tellg();
    in.seekg(0, std::ios::end);
    auto post_end = in.tellg();
    size_t post_size = static_cast<size_t>(post_end - post_start);

    std::vector<uint8_t> posting_data(post_size);
    in.seekg(post_start);
    in.read(reinterpret_cast<char*>(posting_data.data()),
            static_cast<std::streamsize>(post_size));

    // reconstruct inverted index
    for (const auto& te : term_entries) {
        posting_list pl;
        pl.idf = te.idf;
        pl.entries.resize(te.count);

        size_t offset = te.offset;
        for (uint32_t j = 0; j < te.count; ++j) {
            auto& p = pl.entries[j];
            std::memcpy(&p.doc_id, &posting_data[offset], 4);
            offset += 4;
            std::memcpy(&p.term_freq, &posting_data[offset], 4);
            offset += 4;

#ifdef ENABLE_POSITIONS
            uint32_t num_pos;
            std::memcpy(&num_pos, &posting_data[offset], 4);
            offset += 4;
            p.positions.resize(num_pos);
            for (uint32_t k = 0; k < num_pos; ++k) {
                std::memcpy(&p.positions[k], &posting_data[offset], 4);
                offset += 4;
            }
#endif
        }

        inv_idx.terms[te.term] = std::move(pl);
    }

    return true;
}
