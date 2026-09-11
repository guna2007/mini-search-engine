#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

struct posting {
    uint32_t doc_id;
    uint32_t term_freq;
#ifdef ENABLE_POSITIONS
    std::vector<uint32_t> positions;
#endif
};

// skip pointer: every SKIP_BLOCK_SIZE postings, store the doc_id and offset
// enables galloping search during intersection of unequal-length lists
constexpr uint32_t SKIP_BLOCK_SIZE = 128;

struct skip_entry {
    uint32_t doc_id;     // doc_id at this skip point
    uint32_t offset;     // index into entries[]
};

// a posting list for a single term: sorted by doc_id, with precomputed idf
struct posting_list {
    float idf = 0.0f;
    std::vector<posting> entries;
    std::vector<skip_entry> skips;   // built during finalize

    // build skip index over sorted entries
    void build_skip_index();

    // binary search for a doc_id - returns pointer or nullptr
    [[nodiscard]] const posting* find(uint32_t doc_id) const;

    // galloping search: find first entry with doc_id >= target, starting from hint
    // returns index into entries[], or entries.size() if not found
    [[nodiscard]] size_t advance_to(uint32_t target, size_t hint = 0) const;

    // advance past a given doc_id - returns iterator to first entry > doc_id
    [[nodiscard]] auto advance_past(uint32_t doc_id) const {
        auto it = std::upper_bound(
            entries.begin(), entries.end(), doc_id,
            [](uint32_t id, const posting& p) { return id < p.doc_id; }
        );
        return it;
    }
};

// intersect two sorted posting lists - used for AND queries
// uses skip pointers when list lengths differ significantly
[[nodiscard]] std::vector<uint32_t> intersect(const posting_list& a, const posting_list& b);

// union two sorted posting lists - used for OR queries
[[nodiscard]] std::vector<uint32_t> merge_union(const posting_list& a, const posting_list& b);

// subtract b from a - used for NOT queries
[[nodiscard]] std::vector<uint32_t> subtract(const posting_list& a, const posting_list& b);
