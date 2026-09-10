#include "indexer/stemmer.h"

#include <algorithm>
#include <string>

// porter stemmer implementation
// reference: https://tartarus.org/martin/PorterStemmer/def.txt

namespace {

bool is_consonant(const std::string& w, size_t i) {
    switch (w[i]) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            return false;
        case 'y':
            return (i == 0) || !is_consonant(w, i - 1);
        default:
            return true;
    }
}

// measure: the number of vc sequences in the stem before a given position
int measure(const std::string& w, size_t end) {
    int m = 0;
    size_t i = 0;
    while (i < end && !is_consonant(w, i)) ++i;  // skip leading vowels
    while (i < end) {
        while (i < end && is_consonant(w, i)) ++i;   // consonant sequence
        if (i >= end) break;
        ++m;
        while (i < end && !is_consonant(w, i)) ++i;  // vowel sequence
    }
    return m;
}

bool has_vowel(const std::string& w, size_t end) {
    for (size_t i = 0; i < end; ++i) {
        if (!is_consonant(w, i)) return true;
    }
    return false;
}

bool ends_double_consonant(const std::string& w) {
    size_t len = w.size();
    if (len < 2) return false;
    return w[len - 1] == w[len - 2] && is_consonant(w, len - 1);
}

bool ends_cvc(const std::string& w) {
    size_t len = w.size();
    if (len < 3) return false;
    if (!is_consonant(w, len - 1) || is_consonant(w, len - 2) || !is_consonant(w, len - 3))
        return false;
    char c = w[len - 1];
    return c != 'w' && c != 'x' && c != 'y';
}

bool ends_with(const std::string& w, const std::string& suffix) {
    if (w.size() < suffix.size()) return false;
    return w.compare(w.size() - suffix.size(), suffix.size(), suffix) == 0;
}



void step1a(std::string& w) {
    if (ends_with(w, "sses")) { w.resize(w.size() - 2); return; }
    if (ends_with(w, "ies"))  { w.resize(w.size() - 2); return; }
    if (ends_with(w, "ss"))   { return; }
    if (ends_with(w, "s"))    { w.resize(w.size() - 1); return; }
}

void step1b(std::string& w) {
    if (ends_with(w, "eed")) {
        size_t stem_end = w.size() - 3;
        if (measure(w, stem_end) > 0) {
            w.resize(w.size() - 1);
        }
        return;
    }

    bool found = false;
    if (ends_with(w, "ed")) {
        size_t stem_end = w.size() - 2;
        if (has_vowel(w, stem_end)) {
            w.resize(stem_end);
            found = true;
        }
    } else if (ends_with(w, "ing")) {
        size_t stem_end = w.size() - 3;
        if (has_vowel(w, stem_end)) {
            w.resize(stem_end);
            found = true;
        }
    }

    if (found) {
        if (ends_with(w, "at") || ends_with(w, "bl") || ends_with(w, "iz")) {
            w.push_back('e');
        } else if (ends_double_consonant(w)) {
            char last = w.back();
            if (last != 'l' && last != 's' && last != 'z') {
                w.resize(w.size() - 1);
            }
        } else if (measure(w, w.size()) == 1 && ends_cvc(w)) {
            w.push_back('e');
        }
    }
}

void step1c(std::string& w) {
    if (w.size() > 1 && w.back() == 'y' && has_vowel(w, w.size() - 1)) {
        w.back() = 'i';
    }
}

void step2(std::string& w) {
    if (w.size() < 3) return;

    struct rule { const char* suffix; const char* replacement; };
    static const rule rules[] = {
        {"ational", "ate"}, {"tional", "tion"}, {"enci", "ence"},
        {"anci", "ance"}, {"izer", "ize"}, {"abli", "able"},
        {"alli", "al"}, {"entli", "ent"}, {"eli", "e"},
        {"ousli", "ous"}, {"ization", "ize"}, {"ation", "ate"},
        {"ator", "ate"}, {"alism", "al"}, {"iveness", "ive"},
        {"fulness", "ful"}, {"ousness", "ous"}, {"aliti", "al"},
        {"iviti", "ive"}, {"biliti", "ble"},
    };

    for (auto& [suffix, replacement] : rules) {
        std::string s(suffix);
        if (ends_with(w, s)) {
            size_t stem_end = w.size() - s.size();
            if (measure(w, stem_end) > 0) {
                w.replace(stem_end, s.size(), replacement);
            }
            return;
        }
    }
}

void step3(std::string& w) {
    if (w.size() < 3) return;

    struct rule { const char* suffix; const char* replacement; };
    static const rule rules[] = {
        {"icate", "ic"}, {"ative", ""}, {"alize", "al"},
        {"iciti", "ic"}, {"ical", "ic"}, {"ful", ""},
        {"ness", ""},
    };

    for (auto& [suffix, replacement] : rules) {
        std::string s(suffix);
        if (ends_with(w, s)) {
            size_t stem_end = w.size() - s.size();
            if (measure(w, stem_end) > 0) {
                w.replace(stem_end, s.size(), replacement);
            }
            return;
        }
    }
}

void step4(std::string& w) {
    if (w.size() < 3) return;

    static const char* suffixes[] = {
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant",
        "ement", "ment", "ent", "ion", "ou", "ism", "ate", "iti",
        "ous", "ive", "ize",
    };

    for (auto suffix : suffixes) {
        std::string s(suffix);
        if (ends_with(w, s)) {
            size_t stem_end = w.size() - s.size();
            if (s == "ion" && stem_end > 0 && (w[stem_end - 1] == 's' || w[stem_end - 1] == 't')) {
                if (measure(w, stem_end) > 1) {
                    w.resize(stem_end);
                }
            } else {
                if (measure(w, stem_end) > 1) {
                    w.resize(stem_end);
                }
            }
            return;
        }
    }
}

void step5a(std::string& w) {
    if (w.empty()) return;
    if (w.back() == 'e') {
        size_t stem_end = w.size() - 1;
        if (measure(w, stem_end) > 1) {
            w.resize(stem_end);
        } else if (measure(w, stem_end) == 1 && !ends_cvc(w.substr(0, stem_end) + " ") && !ends_cvc(w)) {
            // only remove if not cvc
            std::string test = w.substr(0, stem_end);
            if (!ends_cvc(test)) {
                w.resize(stem_end);
            }
        }
    }
}

void step5b(std::string& w) {
    if (measure(w, w.size()) > 1 && ends_double_consonant(w) && w.back() == 'l') {
        w.resize(w.size() - 1);
    }
}

}  // namespace

std::string stem(std::string_view word) {
    if (word.size() <= 2) return std::string(word);

    std::string w(word);

    step1a(w);
    step1b(w);
    step1c(w);
    step2(w);
    step3(w);
    step4(w);
    step5a(w);
    step5b(w);

    return w;
}
