#include "indexer/tokenizer.h"

#include <cctype>

static bool is_token_char(char c) {
    // alphanumeric ascii only - everything else is a boundary
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

std::vector<token> tokenize(std::string_view text) {
    std::vector<token> tokens;
    tokens.reserve(text.size() / 5);  // rough estimate: avg 5 chars per token

    uint32_t pos = 0;
    size_t i = 0;

    while (i < text.size()) {
        // skip non-token characters
        while (i < text.size() && !is_token_char(text[i])) {
            ++i;
        }

        if (i >= text.size()) break;

        // accumulate token characters
        size_t start = i;
        while (i < text.size() && is_token_char(text[i])) {
            ++i;
        }

        // lowercase and store
        std::string word;
        word.reserve(i - start);
        for (size_t j = start; j < i; ++j) {
            word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[j]))));
        }

        if (!word.empty()) {
            tokens.push_back({std::move(word), pos});
            ++pos;
        }
    }

    return tokens;
}
