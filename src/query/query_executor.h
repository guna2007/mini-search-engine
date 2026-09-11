#pragma once

#include "query/query_parser.h"
#include "query/scorer.h"
#include "index/inverted_index.h"
#include "index/forward_index.h"

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct search_result {
    uint32_t doc_id;
    float score;
    std::string title;
};

struct query_config {
    uint32_t top_k = 10;
    bm25_params bm25 = {};
};

// lru cache for query results - avoids re-executing hot queries
// sharded by hash for reduced contention under concurrent access
struct query_cache {
    explicit query_cache(size_t capacity = 1024) : capacity_(capacity) {}

    // returns nullptr if not cached
    [[nodiscard]] const std::vector<search_result>* get(const std::string& key);

    // insert or update
    void put(const std::string& key, std::vector<search_result> results);

    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t hits() const { return hits_; }
    [[nodiscard]] size_t misses() const { return misses_; }

    void clear();

private:
    size_t capacity_;
    std::list<std::pair<std::string, std::vector<search_result>>> lru_list_;
    std::unordered_map<std::string, decltype(lru_list_)::iterator> cache_map_;
    mutable std::mutex mutex_;
    size_t hits_ = 0;
    size_t misses_ = 0;
};

// executes a parsed query against the index and returns ranked results
struct query_executor {
    const inverted_index& inv_idx;
    const forward_index& fwd_idx;
    query_cache* cache = nullptr;   // optional, set to enable caching

    query_executor(const inverted_index& inv, const forward_index& fwd)
        : inv_idx(inv), fwd_idx(fwd) {}

    // execute a query and return top-k results sorted by score
    [[nodiscard]] std::vector<search_result> execute(
        const query_node& query, const query_config& config = {}) const;

    // execute with cache lookup
    [[nodiscard]] std::vector<search_result> execute_cached(
        const std::string& query_string,
        const query_node& query, const query_config& config = {});

private:
    // resolve a query node to a set of candidate doc_ids
    [[nodiscard]] std::vector<uint32_t> resolve_candidates(const query_node& node) const;

    // collect all terms from the query for scoring
    void collect_terms(const query_node& node, std::vector<std::string>& terms) const;

    // check if a phrase matches in a document
    [[nodiscard]] bool phrase_matches(uint32_t doc_id,
                                       const std::vector<std::string>& phrase_terms) const;
};
