#include "index/forward_index.h"

void forward_index::add_document(uint32_t doc_id, uint32_t length, std::string title) {
    if (doc_id >= docs.size()) {
        docs.resize(doc_id + 1);
    }
    docs[doc_id] = {doc_id, length, std::move(title)};
    total_tokens += length;
}

const doc_info* forward_index::get(uint32_t doc_id) const {
    if (doc_id >= docs.size()) return nullptr;
    return &docs[doc_id];
}

double forward_index::avg_doc_length() const {
    if (docs.empty()) return 0.0;
    return static_cast<double>(total_tokens) / static_cast<double>(docs.size());
}
