#include "index/inverted_index.h"

#include <algorithm>
#include <cmath>

void inverted_index::add_posting(const std::string& term, uint32_t doc_id, uint32_t term_freq
#ifdef ENABLE_POSITIONS
    , std::vector<uint32_t> positions
#endif
) {
    auto& pl = terms[term];
    posting p;
    p.doc_id = doc_id;
    p.term_freq = term_freq;
#ifdef ENABLE_POSITIONS
    p.positions = std::move(positions);
#endif
    pl.entries.push_back(std::move(p));
}

void inverted_index::finalize(uint32_t total_docs, double avg_dl) {
    num_docs = total_docs;
    avg_doc_length = avg_dl;

    for (auto& [term, pl] : terms) {
        // sort by doc_id - required for merge-based operations
        std::sort(pl.entries.begin(), pl.entries.end(),
            [](const posting& a, const posting& b) { return a.doc_id < b.doc_id; });

        // build skip index for long lists
        pl.build_skip_index();

        // precompute idf: log((N - df + 0.5) / (df + 0.5) + 1)
        double df = static_cast<double>(pl.entries.size());
        double n = static_cast<double>(total_docs);
        pl.idf = static_cast<float>(std::log((n - df + 0.5) / (df + 0.5) + 1.0));
    }
}

const posting_list* inverted_index::lookup(const std::string& term) const {
    auto it = terms.find(term);
    if (it == terms.end()) return nullptr;
    return &it->second;
}

size_t inverted_index::total_postings() const {
    size_t total = 0;
    for (const auto& [term, pl] : terms) {
        total += pl.entries.size();
    }
    return total;
}
