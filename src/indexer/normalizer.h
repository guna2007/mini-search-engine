#pragma once

#include "indexer/tokenizer.h"

#include <string>
#include <string_view>
#include <vector>

// full normalization pipeline: tokenize -> stopword removal -> stemming
// returns positioned, stemmed tokens ready for indexing
[[nodiscard]] std::vector<token> normalize(std::string_view text);

// normalize a single query term (stem only, no stopword removal for query terms)
[[nodiscard]] std::string normalize_term(std::string_view term);
