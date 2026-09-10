#pragma once

#include <string>
#include <string_view>

// porter stemmer - reduces english words to their stem form
// not linguistically perfect, but fast and good enough for ir
[[nodiscard]] std::string stem(std::string_view word);
