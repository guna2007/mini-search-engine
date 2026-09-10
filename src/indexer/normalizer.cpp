#include "indexer/normalizer.h"

#include "indexer/stemmer.h"
#include "indexer/stopwords.h"

#include <cctype>

std::vector<token> normalize(std::string_view text) {
    auto tokens = tokenize(text);

    std::vector<token> result;
    result.reserve(tokens.size());

    for (auto& tok : tokens) {
        if (is_stopword(tok.text)) continue;
        if (tok.text.empty()) continue;

        // skip pure numeric tokens - rarely useful for text search
        bool all_digits = true;
        for (char c : tok.text) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                all_digits = false;
                break;
            }
        }
        if (all_digits && tok.text.size() < 4) continue;

        tok.text = stem(tok.text);
        if (!tok.text.empty()) {
            result.push_back(std::move(tok));
        }
    }

    return result;
}

std::string normalize_term(std::string_view term) {
    // lowercase
    std::string lower;
    lower.reserve(term.size());
    for (char c : term) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return stem(lower);
}
