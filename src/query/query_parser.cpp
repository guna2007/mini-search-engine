#include "query/query_parser.h"

#include "indexer/normalizer.h"

#include <sstream>

std::unique_ptr<query_node> query_node::make_term(std::string t) {
    auto node = std::make_unique<query_node>();
    node->type = query_type::TERM;
    node->term = std::move(t);
    return node;
}

std::unique_ptr<query_node> query_node::make_phrase(std::vector<std::string> terms) {
    auto node = std::make_unique<query_node>();
    node->type = query_type::PHRASE;
    node->phrase_terms = std::move(terms);
    return node;
}

std::unique_ptr<query_node> query_node::make_and(std::unique_ptr<query_node> left,
                                                   std::unique_ptr<query_node> right) {
    auto node = std::make_unique<query_node>();
    node->type = query_type::AND;
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
}

std::unique_ptr<query_node> query_node::make_or(std::unique_ptr<query_node> left,
                                                  std::unique_ptr<query_node> right) {
    auto node = std::make_unique<query_node>();
    node->type = query_type::OR;
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
}

std::unique_ptr<query_node> query_node::make_not(std::unique_ptr<query_node> include,
                                                   std::unique_ptr<query_node> exclude) {
    auto node = std::make_unique<query_node>();
    node->type = query_type::NOT;
    node->children.push_back(std::move(include));
    node->children.push_back(std::move(exclude));
    return node;
}

namespace {

struct token_stream {
    std::vector<std::string> tokens;
    size_t pos = 0;

    [[nodiscard]] bool has_next() const { return pos < tokens.size(); }
    [[nodiscard]] const std::string& peek() const { return tokens[pos]; }
    std::string consume() { return tokens[pos++]; }
};

// tokenize the query string into logical tokens, preserving quoted phrases
std::vector<std::string> lex_query(const std::string& query) {
    std::vector<std::string> tokens;
    size_t i = 0;

    while (i < query.size()) {
        while (i < query.size() && query[i] == ' ') ++i;
        if (i >= query.size()) break;

        if (query[i] == '"') {
            // phrase - collect everything until closing quote
            ++i;
            std::string phrase = "\"";
            while (i < query.size() && query[i] != '"') {
                phrase.push_back(query[i]);
                ++i;
            }
            if (i < query.size()) ++i;  // skip closing quote
            phrase.push_back('"');
            tokens.push_back(std::move(phrase));
        } else {
            // regular word or operator
            size_t start = i;
            while (i < query.size() && query[i] != ' ' && query[i] != '"') ++i;
            tokens.push_back(query.substr(start, i - start));
        }
    }

    return tokens;
}

std::unique_ptr<query_node> parse_atom(token_stream& ts) {
    if (!ts.has_next()) return nullptr;

    const auto& tok = ts.peek();

    if (tok.front() == '"' && tok.back() == '"' && tok.size() > 2) {
        // phrase query
        std::string inner = ts.consume().substr(1);
        inner.pop_back();  // remove trailing quote

        std::istringstream iss(inner);
        std::string word;
        std::vector<std::string> terms;
        while (iss >> word) {
            terms.push_back(normalize_term(word));
        }

        if (terms.size() == 1) {
            return query_node::make_term(std::move(terms[0]));
        }
        return query_node::make_phrase(std::move(terms));
    }

    // regular term - normalize it
    std::string term = normalize_term(ts.consume());
    return query_node::make_term(std::move(term));
}

}  // namespace

std::unique_ptr<query_node> parse_query(const std::string& query) {
    auto lex_tokens = lex_query(query);
    if (lex_tokens.empty()) return nullptr;

    token_stream ts{std::move(lex_tokens), 0};

    // parse atoms connected by AND, NOT, or implicit OR
    std::vector<std::unique_ptr<query_node>> atoms;
    std::vector<std::string> operators;

    auto first = parse_atom(ts);
    if (!first) return nullptr;
    atoms.push_back(std::move(first));

    while (ts.has_next()) {
        const auto& next = ts.peek();

        if (next == "AND") {
            operators.push_back("AND");
            ts.consume();
            auto rhs = parse_atom(ts);
            if (rhs) atoms.push_back(std::move(rhs));
        } else if (next == "NOT") {
            operators.push_back("NOT");
            ts.consume();
            auto rhs = parse_atom(ts);
            if (rhs) atoms.push_back(std::move(rhs));
        } else {
            // implicit OR
            operators.push_back("OR");
            auto rhs = parse_atom(ts);
            if (rhs) atoms.push_back(std::move(rhs));
        }
    }

    if (atoms.size() == 1) return std::move(atoms[0]);

    // build tree left-to-right, respecting operator precedence:
    // NOT > AND > OR (we process NOT first, then AND, then OR)

    // first pass: apply NOT
    for (size_t i = 0; i < operators.size(); ++i) {
        if (operators[i] == "NOT" && atoms[i] && atoms[i + 1]) {
            atoms[i] = query_node::make_not(std::move(atoms[i]), std::move(atoms[i + 1]));
            atoms.erase(atoms.begin() + static_cast<long>(i + 1));
            operators.erase(operators.begin() + static_cast<long>(i));
            --i;
        }
    }

    // second pass: apply AND
    for (size_t i = 0; i < operators.size(); ++i) {
        if (operators[i] == "AND" && atoms[i] && atoms[i + 1]) {
            atoms[i] = query_node::make_and(std::move(atoms[i]), std::move(atoms[i + 1]));
            atoms.erase(atoms.begin() + static_cast<long>(i + 1));
            operators.erase(operators.begin() + static_cast<long>(i));
            --i;
        }
    }

    // third pass: apply OR (what's left)
    auto result = std::move(atoms[0]);
    for (size_t i = 1; i < atoms.size(); ++i) {
        if (atoms[i]) {
            result = query_node::make_or(std::move(result), std::move(atoms[i]));
        }
    }

    return result;
}
