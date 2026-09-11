#pragma once

#include "index/inverted_index.h"
#include "index/forward_index.h"

#include <string>

// binary index format:
// [header]  magic(4) version(4) num_terms(8) num_docs(8) avg_dl(8)
// [forward] for each doc: doc_id(4) length(4) title_len(2) title_bytes(var)
// [terms]   for each term: term_len(2) term(var) idf(4) offset(8) count(4)
// [posts]   raw posting arrays: doc_id(4) term_freq(4) [positions if enabled]

constexpr uint32_t INDEX_MAGIC = 0x53524348;   // "SRCH"
constexpr uint32_t INDEX_VERSION = 1;

// serialize index to a binary file
bool serialize_index(const std::string& path,
                     const inverted_index& inv_idx,
                     const forward_index& fwd_idx);

// deserialize index from a binary file
bool deserialize_index(const std::string& path,
                       inverted_index& inv_idx,
                       forward_index& fwd_idx);
