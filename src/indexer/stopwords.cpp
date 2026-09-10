#include "indexer/stopwords.h"

#include <algorithm>
#include <fstream>
#include <vector>

// sorted vector - binary search is faster than hash lookup for ~150 entries
// because the entire set fits in L1 cache
static std::vector<std::string> stopword_list;

void load_stopwords(const std::string& path) {
    stopword_list.clear();

    std::ifstream file(path);
    std::string word;
    while (std::getline(file, word)) {
        // strip whitespace
        while (!word.empty() && (word.back() == '\r' || word.back() == ' ')) {
            word.pop_back();
        }
        if (!word.empty()) {
            stopword_list.push_back(std::move(word));
        }
    }

    std::sort(stopword_list.begin(), stopword_list.end());
    auto it = std::unique(stopword_list.begin(), stopword_list.end());
    stopword_list.erase(it, stopword_list.end());
}

bool is_stopword(std::string_view word) {
    return std::binary_search(
        stopword_list.begin(), stopword_list.end(),
        word,
        [](const auto& a, const auto& b) {
            return std::string_view(a) < std::string_view(b);
        }
    );
}
