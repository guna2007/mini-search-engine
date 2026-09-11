#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct doc_info {
    uint32_t doc_id;
    uint32_t length;    // total token count after normalization
    std::string title;  // document title or file path
};

// forward index: doc_id -> document metadata
// stored as a flat vector indexed by doc_id for O(1) lookups
struct forward_index {
    std::vector<doc_info> docs;
    uint64_t total_tokens = 0;

    void add_document(uint32_t doc_id, uint32_t length, std::string title);

    [[nodiscard]] const doc_info* get(uint32_t doc_id) const;
    [[nodiscard]] uint32_t doc_count() const { return static_cast<uint32_t>(docs.size()); }
    [[nodiscard]] double avg_doc_length() const;
};
