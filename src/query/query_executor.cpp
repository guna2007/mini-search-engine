#include "query/query_executor.h"

#include <algorithm>
#include <queue>

// --- query cache implementation ---

const std::vector<search_result>* query_cache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        ++misses_;
        return nullptr;
    }
    ++hits_;
    // move to front (most recently used)
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    return &it->second->second;
}

void query_cache::put(const std::string& key, std::vector<search_result> results) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
        // update existing entry
        it->second->second = std::move(results);
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
        return;
    }

    // evict lru if at capacity
    if (cache_map_.size() >= capacity_) {
        auto& back = lru_list_.back();
        cache_map_.erase(back.first);
        lru_list_.pop_back();
    }

    lru_list_.emplace_front(key, std::move(results));
    cache_map_[key] = lru_list_.begin();
}

size_t query_cache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_map_.size();
}

void query_cache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_map_.clear();
    lru_list_.clear();
    hits_ = 0;
    misses_ = 0;
}

// --- query executor implementation ---

void query_executor::collect_terms(const query_node& node,
                                    std::vector<std::string>& terms) const {
    switch (node.type) {
        case query_type::TERM:
            terms.push_back(node.term);
            break;
        case query_type::PHRASE:
            for (const auto& t : node.phrase_terms) {
                terms.push_back(t);
            }
            break;
        case query_type::AND:
        case query_type::OR:
            for (const auto& child : node.children) {
                collect_terms(*child, terms);
            }
            break;
        case query_type::NOT:
            if (!node.children.empty()) {
                collect_terms(*node.children[0], terms);
            }
            break;
    }
}

bool query_executor::phrase_matches(
    uint32_t doc_id, const std::vector<std::string>& phrase_terms) const {
#ifdef ENABLE_POSITIONS
    if (phrase_terms.size() < 2) return true;

    std::vector<const std::vector<uint32_t>*> positions;
    for (const auto& term : phrase_terms) {
        const auto* pl = inv_idx.lookup(term);
        if (!pl) return false;
        const auto* p = pl->find(doc_id);
        if (!p) return false;
        positions.push_back(&p->positions);
    }

    for (uint32_t start_pos : *positions[0]) {
        bool found = true;
        for (size_t i = 1; i < positions.size(); ++i) {
            uint32_t expected = start_pos + static_cast<uint32_t>(i);
            if (!std::binary_search(positions[i]->begin(),
                                     positions[i]->end(), expected)) {
                found = false;
                break;
            }
        }
        if (found) return true;
    }
    return false;
#else
    (void)doc_id;
    (void)phrase_terms;
    return true;
#endif
}

std::vector<uint32_t> query_executor::resolve_candidates(
    const query_node& node) const {
    switch (node.type) {
        case query_type::TERM: {
            const auto* pl = inv_idx.lookup(node.term);
            if (!pl) return {};
            std::vector<uint32_t> result;
            result.reserve(pl->entries.size());
            for (const auto& p : pl->entries) {
                result.push_back(p.doc_id);
            }
            return result;
        }
        case query_type::PHRASE: {
            if (node.phrase_terms.empty()) return {};
            // start with the rarest term for minimum work
            size_t min_idx = 0;
            size_t min_size = SIZE_MAX;
            for (size_t i = 0; i < node.phrase_terms.size(); ++i) {
                const auto* pl = inv_idx.lookup(node.phrase_terms[i]);
                size_t sz = pl ? pl->entries.size() : 0;
                if (sz < min_size) { min_size = sz; min_idx = i; }
            }
            const auto* base = inv_idx.lookup(node.phrase_terms[min_idx]);
            if (!base) return {};

            std::vector<uint32_t> candidates;
            for (const auto& p : base->entries) {
                bool has_all = true;
                for (const auto& term : node.phrase_terms) {
                    const auto* pl = inv_idx.lookup(term);
                    if (!pl || !pl->find(p.doc_id)) {
                        has_all = false;
                        break;
                    }
                }
                if (has_all && phrase_matches(p.doc_id, node.phrase_terms)) {
                    candidates.push_back(p.doc_id);
                }
            }
            return candidates;
        }
        case query_type::AND: {
            if (node.children.size() < 2) {
                return node.children.empty() ? std::vector<uint32_t>{}
                                             : resolve_candidates(*node.children[0]);
            }
            auto left = resolve_candidates(*node.children[0]);
            auto right = resolve_candidates(*node.children[1]);

            // use galloping intersection for skewed sizes
            std::vector<uint32_t> result;
            if (left.size() > right.size() * 8 || right.size() > left.size() * 8) {
                auto& shorter = (left.size() <= right.size()) ? left : right;
                auto& longer = (left.size() <= right.size()) ? right : left;
                for (uint32_t id : shorter) {
                    auto it = std::lower_bound(longer.begin(), longer.end(), id);
                    if (it != longer.end() && *it == id) {
                        result.push_back(id);
                    }
                }
            } else {
                size_t i = 0, j = 0;
                while (i < left.size() && j < right.size()) {
                    if (left[i] == right[j]) {
                        result.push_back(left[i]);
                        ++i; ++j;
                    } else if (left[i] < right[j]) {
                        ++i;
                    } else {
                        ++j;
                    }
                }
            }
            return result;
        }
        case query_type::OR: {
            if (node.children.size() < 2) {
                return node.children.empty() ? std::vector<uint32_t>{}
                                             : resolve_candidates(*node.children[0]);
            }
            auto left = resolve_candidates(*node.children[0]);
            auto right = resolve_candidates(*node.children[1]);

            std::vector<uint32_t> result;
            result.reserve(left.size() + right.size());
            std::merge(left.begin(), left.end(), right.begin(), right.end(),
                       std::back_inserter(result));
            auto it = std::unique(result.begin(), result.end());
            result.erase(it, result.end());
            return result;
        }
        case query_type::NOT: {
            if (node.children.size() < 2) {
                return node.children.empty() ? std::vector<uint32_t>{}
                                             : resolve_candidates(*node.children[0]);
            }
            auto include = resolve_candidates(*node.children[0]);
            auto exclude = resolve_candidates(*node.children[1]);

            std::vector<uint32_t> result;
            size_t i = 0, j = 0;
            while (i < include.size() && j < exclude.size()) {
                if (include[i] == exclude[j]) {
                    ++i; ++j;
                } else if (include[i] < exclude[j]) {
                    result.push_back(include[i]);
                    ++i;
                } else {
                    ++j;
                }
            }
            while (i < include.size()) {
                result.push_back(include[i++]);
            }
            return result;
        }
    }
    return {};
}

std::vector<search_result> query_executor::execute(
    const query_node& query, const query_config& config) const {

    // collect all terms for scoring
    std::vector<std::string> all_terms;
    collect_terms(query, all_terms);
    if (all_terms.empty()) return {};

    // resolve candidate documents
    auto candidates = resolve_candidates(query);
    if (candidates.empty()) return {};

    // pre-resolve posting list pointers and precompute bm25 constants
    scorer sc(inv_idx, fwd_idx, all_terms, config.bm25);

    // min-heap of size top_k: (score, doc_id)
    using entry = std::pair<float, uint32_t>;
    auto cmp = [](const entry& a, const entry& b) { return a.first > b.first; };
    std::priority_queue<entry, std::vector<entry>, decltype(cmp)> heap(cmp);

    for (uint32_t doc_id : candidates) {
        // fetch doc length once, pass to scorer
        const auto* doc = fwd_idx.get(doc_id);
        if (!doc) continue;
        float dl = static_cast<float>(doc->length);

        float s = sc.score_with_length(doc_id, dl);
        if (s <= 0.0f) continue;

        if (heap.size() < config.top_k) {
            heap.push({s, doc_id});
        } else if (s > heap.top().first) {
            heap.pop();
            heap.push({s, doc_id});
        }
    }

    // extract results in descending score order
    std::vector<search_result> results;
    results.reserve(heap.size());
    while (!heap.empty()) {
        auto [s, id] = heap.top();
        heap.pop();
        const auto* doc = fwd_idx.get(id);
        std::string title = doc ? doc->title : "";
        results.push_back({id, s, std::move(title)});
    }

    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<search_result> query_executor::execute_cached(
    const std::string& query_string,
    const query_node& query, const query_config& config) {

    if (cache) {
        const auto* cached = cache->get(query_string);
        if (cached) return *cached;
    }

    auto results = execute(query, config);

    if (cache) {
        cache->put(query_string, results);
    }

    return results;
}
