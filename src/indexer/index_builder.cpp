#include "indexer/index_builder.h"

#include "indexer/normalizer.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

// minimal json field extraction - avoids pulling in a json library
// handles simple {"key": "value", ...} objects
static std::string extract_json_field(const std::string& line, const std::string& field) {
    std::string key = "\"" + field + "\"";
    auto pos = line.find(key);
    if (pos == std::string::npos) return "";

    // skip past the key and colon
    pos = line.find(':', pos + key.size());
    if (pos == std::string::npos) return "";
    ++pos;

    // skip whitespace
    while (pos < line.size() && line[pos] == ' ') ++pos;

    if (pos >= line.size()) return "";

    if (line[pos] == '"') {
        // string value - find closing quote, handling escapes
        ++pos;
        std::string result;
        while (pos < line.size() && line[pos] != '"') {
            if (line[pos] == '\\' && pos + 1 < line.size()) {
                ++pos;
                switch (line[pos]) {
                    case 'n':  result.push_back('\n'); break;
                    case 't':  result.push_back('\t'); break;
                    case '\\': result.push_back('\\'); break;
                    case '"':  result.push_back('"');  break;
                    default:   result.push_back(line[pos]); break;
                }
            } else {
                result.push_back(line[pos]);
            }
            ++pos;
        }
        return result;
    }

    // numeric or other value - read until comma or closing brace
    size_t end = line.find_first_of(",}", pos);
    if (end == std::string::npos) end = line.size();
    return line.substr(pos, end - pos);
}

void index_builder::index_document(const std::string& title, const std::string& text) {
    uint32_t doc_id = next_doc_id++;

    auto tokens = normalize(text);

    // aggregate term frequencies and positions
    std::unordered_map<std::string, uint32_t> term_freqs;
#ifdef ENABLE_POSITIONS
    std::unordered_map<std::string, std::vector<uint32_t>> term_positions;
#endif

    for (const auto& tok : tokens) {
        term_freqs[tok.text]++;
#ifdef ENABLE_POSITIONS
        term_positions[tok.text].push_back(tok.position);
#endif
    }

    fwd_index.add_document(doc_id, static_cast<uint32_t>(tokens.size()), title);

    for (const auto& [term, freq] : term_freqs) {
        inv_index.add_posting(term, doc_id, freq
#ifdef ENABLE_POSITIONS
            , term_positions[term]
#endif
        );
    }
}

index_stats index_builder::build_from_jsonl(const std::string& path) {
    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "error: could not open " << path << "\n";
        return {};
    }

    std::string line;
    uint32_t count = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::string id = extract_json_field(line, "id");
        std::string title = extract_json_field(line, "title");
        std::string text = extract_json_field(line, "text");

        if (title.empty()) title = id;
        if (text.empty()) continue;

        // combine title and text for indexing
        std::string full_text = title + " " + text;
        index_document(title, full_text);

        ++count;
        if (count % 10000 == 0) {
            std::cerr << "\rindexed " << count << " documents..." << std::flush;
        }
    }

    std::cerr << "\rindexed " << count << " documents    \n";

    double avg_dl = fwd_index.avg_doc_length();
    inv_index.finalize(count, avg_dl);

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        count,
        inv_index.vocabulary_size(),
        inv_index.total_postings(),
        avg_dl,
        elapsed
    };
}

index_stats index_builder::build_from_directory(const std::string& dir_path) {
    auto start = std::chrono::high_resolution_clock::now();

    uint32_t count = 0;

    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        if (ext != ".txt" && ext != ".md" && ext != ".text") continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

        std::string title = entry.path().filename().string();
        index_document(title, text);

        ++count;
        if (count % 1000 == 0) {
            std::cerr << "\rindexed " << count << " documents..." << std::flush;
        }
    }

    std::cerr << "\rindexed " << count << " documents    \n";

    double avg_dl = fwd_index.avg_doc_length();
    inv_index.finalize(count, avg_dl);

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        count,
        inv_index.vocabulary_size(),
        inv_index.total_postings(),
        avg_dl,
        elapsed
    };
}
