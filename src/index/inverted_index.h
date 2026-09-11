#pragma once

#include "index/posting_list.h"

#include <string>
#include <unordered_map>
#include <vector>

// the core inverted index: maps terms to their posting lists
// posting lists are sorted by doc_id for efficient merging
struct inverted_index {
    std::unordered_map<std::string, posting_list> terms;
    uint32_t num_docs = 0;
    double avg_doc_length = 0.0;

    // add a posting for a term
    void add_posting(const std::string& term, uint32_t doc_id, uint32_t term_freq
#ifdef ENABLE_POSITIONS
        , std::vector<uint32_t> positions
#endif
    );

    // finalize: sort posting lists, compute idf for all terms
    void finalize(uint32_t total_docs, double avg_dl);

    // look up a term - returns nullptr if not found
    [[nodiscard]] const posting_list* lookup(const std::string& term) const;

    // total unique terms
    [[nodiscard]] size_t vocabulary_size() const { return terms.size(); }

    // total postings across all terms
    [[nodiscard]] size_t total_postings() const;
};
