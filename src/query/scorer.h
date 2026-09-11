#pragma once

#include "index/inverted_index.h"
#include "index/forward_index.h"

#include <cstdint>
#include <string>
#include <vector>

struct bm25_params {
    float k1 = 1.2f;
    float b = 0.75f;
};

// precomputed per-term scoring context - avoids hash lookups during scoring
struct term_scorer {
    const posting_list* pl;     // cached pointer
    float idf;
    float k1_plus_1;
    float k1_times_one_minus_b;
    float k1_times_b_over_avgdl;
};

// bm25 scorer with precomputed constants for zero-overhead per-document scoring
struct scorer {
    const forward_index& fwd_idx;
    bm25_params params;
    std::vector<term_scorer> term_scorers;

    scorer(const inverted_index& inv, const forward_index& fwd,
           const std::vector<std::string>& query_terms, bm25_params p = {});

    // score a single document against all precomputed terms
    // uses pre-resolved posting list pointers - no hash lookups
    [[nodiscard]] float score(uint32_t doc_id) const;

    // score using a pre-fetched doc length to avoid forward index lookup
    [[nodiscard]] float score_with_length(uint32_t doc_id, float doc_length) const;
};
