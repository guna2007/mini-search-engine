#pragma once

#include "index/inverted_index.h"
#include "index/forward_index.h"

#include <string>

struct index_stats {
    uint32_t docs_indexed;
    size_t vocab_size;
    size_t total_postings;
    double avg_doc_length;
    double build_time_ms;
};

// builds both inverted and forward indices from a corpus
// supports jsonl format (one {"id": "...", "title": "...", "text": "..."} per line)
// and directory-of-files format
struct index_builder {
    inverted_index inv_index;
    forward_index fwd_index;

    // build from a jsonl file
    [[nodiscard]] index_stats build_from_jsonl(const std::string& path);

    // build from a directory of text files
    [[nodiscard]] index_stats build_from_directory(const std::string& dir_path);

private:
    uint32_t next_doc_id = 0;
    void index_document(const std::string& title, const std::string& text);
};
