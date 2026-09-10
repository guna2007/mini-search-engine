#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

struct token {
    std::string text;
    uint32_t position;  // offset within document (for phrase queries)
};

// splits raw text into tokens on whitespace and punctuation boundaries
// lowercases all output, strips non-ascii
[[nodiscard]] std::vector<token> tokenize(std::string_view text);
