#include "index/posting_list.h"

#include <algorithm>

void posting_list::build_skip_index() {
    skips.clear();
    if (entries.size() < SKIP_BLOCK_SIZE * 2) return;  // not worth it for short lists

    skips.reserve(entries.size() / SKIP_BLOCK_SIZE);
    for (size_t i = 0; i < entries.size(); i += SKIP_BLOCK_SIZE) {
        skips.push_back({entries[i].doc_id, static_cast<uint32_t>(i)});
    }
}

const posting* posting_list::find(uint32_t doc_id) const {
    // use skip index if available for fast narrowing
    size_t lo = 0;
    size_t hi = entries.size();

    if (!skips.empty()) {
        // binary search in skip index to narrow range
        auto it = std::upper_bound(
            skips.begin(), skips.end(), doc_id,
            [](uint32_t id, const skip_entry& s) { return id < s.doc_id; }
        );
        if (it != skips.begin()) {
            --it;
            lo = it->offset;
        }
        if (it + 1 != skips.end()) {
            hi = std::min(hi, static_cast<size_t>((it + 1)->offset + 1));
        }
    }

    auto begin = entries.begin() + static_cast<long>(lo);
    auto end = entries.begin() + static_cast<long>(hi);
    auto found = std::lower_bound(begin, end, doc_id,
        [](const posting& p, uint32_t id) { return p.doc_id < id; });

    if (found != end && found->doc_id == doc_id) {
        return &(*found);
    }
    return nullptr;
}

size_t posting_list::advance_to(uint32_t target, size_t hint) const {
    if (hint >= entries.size()) return entries.size();

    // galloping search: double the step until we overshoot, then binary search
    size_t lo = hint;
    size_t step = 1;

    while (lo + step < entries.size() && entries[lo + step].doc_id < target) {
        lo += step;
        step *= 2;
    }

    size_t hi = std::min(lo + step, entries.size());
    lo = (step > 1) ? lo : hint;

    auto begin = entries.begin() + static_cast<long>(lo);
    auto end = entries.begin() + static_cast<long>(hi);
    auto it = std::lower_bound(begin, end, target,
        [](const posting& p, uint32_t id) { return p.doc_id < id; });

    return static_cast<size_t>(it - entries.begin());
}

std::vector<uint32_t> intersect(const posting_list& a, const posting_list& b) {
    // always iterate the shorter list and probe the longer one
    const posting_list& shorter = (a.entries.size() <= b.entries.size()) ? a : b;
    const posting_list& longer = (a.entries.size() <= b.entries.size()) ? b : a;

    std::vector<uint32_t> result;

    // if the length ratio is high, use galloping on the longer list
    if (longer.entries.size() > shorter.entries.size() * 8) {
        size_t j = 0;
        for (size_t i = 0; i < shorter.entries.size(); ++i) {
            uint32_t target = shorter.entries[i].doc_id;
            j = longer.advance_to(target, j);
            if (j < longer.entries.size() && longer.entries[j].doc_id == target) {
                result.push_back(target);
                ++j;
            }
        }
    } else {
        // standard merge intersection for similarly-sized lists
        size_t i = 0, j = 0;
        while (i < shorter.entries.size() && j < longer.entries.size()) {
            if (shorter.entries[i].doc_id == longer.entries[j].doc_id) {
                result.push_back(shorter.entries[i].doc_id);
                ++i; ++j;
            } else if (shorter.entries[i].doc_id < longer.entries[j].doc_id) {
                ++i;
            } else {
                ++j;
            }
        }
    }

    return result;
}

std::vector<uint32_t> merge_union(const posting_list& a, const posting_list& b) {
    std::vector<uint32_t> result;
    result.reserve(a.entries.size() + b.entries.size());
    size_t i = 0, j = 0;

    while (i < a.entries.size() && j < b.entries.size()) {
        if (a.entries[i].doc_id == b.entries[j].doc_id) {
            result.push_back(a.entries[i].doc_id);
            ++i; ++j;
        } else if (a.entries[i].doc_id < b.entries[j].doc_id) {
            result.push_back(a.entries[i].doc_id);
            ++i;
        } else {
            result.push_back(b.entries[j].doc_id);
            ++j;
        }
    }

    while (i < a.entries.size()) {
        result.push_back(a.entries[i++].doc_id);
    }
    while (j < b.entries.size()) {
        result.push_back(b.entries[j++].doc_id);
    }

    return result;
}

std::vector<uint32_t> subtract(const posting_list& a, const posting_list& b) {
    std::vector<uint32_t> result;
    size_t i = 0, j = 0;

    while (i < a.entries.size() && j < b.entries.size()) {
        if (a.entries[i].doc_id == b.entries[j].doc_id) {
            ++i; ++j;
        } else if (a.entries[i].doc_id < b.entries[j].doc_id) {
            result.push_back(a.entries[i].doc_id);
            ++i;
        } else {
            ++j;
        }
    }

    while (i < a.entries.size()) {
        result.push_back(a.entries[i++].doc_id);
    }

    return result;
}
