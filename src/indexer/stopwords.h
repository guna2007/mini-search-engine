#pragma once

#include <string>
#include <string_view>

// loads stopwords from a file (one word per line) into a sorted flat vector
// uses binary search for lookup - better cache locality than unordered_set
void load_stopwords(const std::string& path);

// returns true if the word should be filtered out
[[nodiscard]] bool is_stopword(std::string_view word);
