#pragma once

#include <memory>
#include <string>
#include <vector>

enum class query_type {
    TERM,
    PHRASE,
    AND,
    OR,
    NOT
};

struct query_node {
    query_type type;
    std::string term;                              // leaf nodes only
    std::vector<std::string> phrase_terms;         // PHRASE nodes only
    std::vector<std::unique_ptr<query_node>> children;  // AND/OR/NOT nodes

    // convenience constructors
    static std::unique_ptr<query_node> make_term(std::string t);
    static std::unique_ptr<query_node> make_phrase(std::vector<std::string> terms);
    static std::unique_ptr<query_node> make_and(std::unique_ptr<query_node> left,
                                                 std::unique_ptr<query_node> right);
    static std::unique_ptr<query_node> make_or(std::unique_ptr<query_node> left,
                                                std::unique_ptr<query_node> right);
    static std::unique_ptr<query_node> make_not(std::unique_ptr<query_node> include,
                                                 std::unique_ptr<query_node> exclude);
};

// parse a query string into an AST
// supports: single terms, multi-term (implicit OR), AND, NOT, "phrase queries"
[[nodiscard]] std::unique_ptr<query_node> parse_query(const std::string& query);
