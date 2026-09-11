#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "indexer/tokenizer.h"
#include "indexer/stemmer.h"
#include "indexer/stopwords.h"
#include "indexer/normalizer.h"
#include "index/posting_list.h"
#include "index/inverted_index.h"
#include "index/forward_index.h"
#include "query/query_parser.h"
#include "query/scorer.h"
#include "query/query_executor.h"
#include "storage/serializer.h"

namespace {

int tests_run = 0;
int tests_passed = 0;

void check(bool condition, const std::string& name) {
    ++tests_run;
    if (condition) {
        ++tests_passed;
        std::cout << "  pass: " << name << "\n";
    } else {
        std::cout << "  FAIL: " << name << "\n";
    }
}

void test_tokenizer() {
    std::cout << "\n--- tokenizer ---\n";

    auto tokens = tokenize("Hello, World! This is a TEST.");
    check(tokens.size() == 6, "splits on punctuation and whitespace");
    check(tokens[0].text == "hello", "lowercases first token");
    check(tokens[5].text == "test", "lowercases last token");
    check(tokens[0].position == 0, "first token position is 0");
    check(tokens[5].position == 5, "last token position is 5");

    auto empty = tokenize("");
    check(empty.empty(), "empty input produces empty output");

    auto punct = tokenize("...---!!!");
    check(punct.empty(), "pure punctuation produces empty output");

    auto unicode = tokenize("café résumé naïve");
    check(!unicode.empty(), "handles words with non-ascii adjacent chars");
}

void test_stemmer() {
    std::cout << "\n--- stemmer ---\n";

    check(stem("running") == "run", "running -> run");
    check(stem("cats") == "cat", "cats -> cat");
    check(stem("caresses") == "caress", "caresses -> caress");
    check(stem("ponies") == "poni", "ponies -> poni");

    // short words should pass through
    check(stem("a") == "a", "single char unchanged");
    check(stem("an") == "an", "two char unchanged");
}

void test_stopwords() {
    std::cout << "\n--- stopwords ---\n";

    // load from the project data file
    load_stopwords("../data/stopwords.txt");

    check(is_stopword("the"), "'the' is a stopword");
    check(is_stopword("is"), "'is' is a stopword");
    check(is_stopword("and"), "'and' is a stopword");
    check(!is_stopword("computer"), "'computer' is not a stopword");
    check(!is_stopword("algorithm"), "'algorithm' is not a stopword");
}

void test_normalizer() {
    std::cout << "\n--- normalizer ---\n";

    load_stopwords("../data/stopwords.txt");

    auto tokens = normalize("The quick brown foxes are jumping over the lazy dogs.");
    check(!tokens.empty(), "produces tokens from normal text");

    // "the", "are", "over" should be removed as stopwords
    bool has_the = false;
    for (const auto& t : tokens) {
        if (t.text == "the") has_the = true;
    }
    check(!has_the, "removes stopwords");
}

void test_inverted_index() {
    std::cout << "\n--- inverted index ---\n";

    inverted_index idx;

#ifdef ENABLE_POSITIONS
    idx.add_posting("hello", 0, 2, {0, 5});
    idx.add_posting("hello", 1, 1, {3});
    idx.add_posting("world", 0, 1, {1});
    idx.add_posting("world", 2, 3, {0, 2, 4});
#else
    idx.add_posting("hello", 0, 2);
    idx.add_posting("hello", 1, 1);
    idx.add_posting("world", 0, 1);
    idx.add_posting("world", 2, 3);
#endif

    idx.finalize(3, 5.0);

    check(idx.vocabulary_size() == 2, "two unique terms");
    check(idx.total_postings() == 4, "four total postings");

    const auto* pl = idx.lookup("hello");
    check(pl != nullptr, "lookup finds 'hello'");
    check(pl->entries.size() == 2, "'hello' has 2 postings");
    check(pl->entries[0].doc_id < pl->entries[1].doc_id, "postings sorted by doc_id");
    check(pl->idf > 0.0f, "idf is positive");

    check(idx.lookup("nonexistent") == nullptr, "lookup returns null for missing term");
}

void test_posting_list_operations() {
    std::cout << "\n--- posting list operations ---\n";

    posting_list a, b;
    a.entries = {{0, 1, {}}, {2, 1, {}}, {4, 1, {}}, {6, 1, {}}};
    b.entries = {{1, 1, {}}, {2, 1, {}}, {4, 1, {}}, {7, 1, {}}};

    auto inter = intersect(a, b);
    check(inter.size() == 2, "intersection has 2 elements");
    check(inter[0] == 2 && inter[1] == 4, "intersection contains {2, 4}");

    auto un = merge_union(a, b);
    check(un.size() == 6, "union has 6 elements");

    auto sub = subtract(a, b);
    check(sub.size() == 2, "subtract has 2 elements");
    check(sub[0] == 0 && sub[1] == 6, "subtract contains {0, 6}");
}

void test_query_parser() {
    std::cout << "\n--- query parser ---\n";

    auto q1 = parse_query("hello");
    check(q1 != nullptr, "parses single term");
    check(q1->type == query_type::TERM, "single term is TERM type");

    auto q2 = parse_query("hello world");
    check(q2 != nullptr, "parses multi-term");
    check(q2->type == query_type::OR, "implicit OR for multi-term");

    auto q3 = parse_query("hello AND world");
    check(q3 != nullptr, "parses AND query");
    check(q3->type == query_type::AND, "AND query has AND type");

    auto q4 = parse_query("hello NOT world");
    check(q4 != nullptr, "parses NOT query");
    check(q4->type == query_type::NOT, "NOT query has NOT type");

    auto q5 = parse_query("\"hello world\"");
    check(q5 != nullptr, "parses phrase query");
    check(q5->type == query_type::PHRASE, "phrase query has PHRASE type");
}

void test_bm25_scorer() {
    std::cout << "\n--- bm25 scorer ---\n";

    inverted_index inv_idx;
    forward_index fwd_idx;

    fwd_idx.add_document(0, 10, "doc0");
    fwd_idx.add_document(1, 20, "doc1");
    fwd_idx.add_document(2, 5, "doc2");

#ifdef ENABLE_POSITIONS
    inv_idx.add_posting("test", 0, 3, {0, 3, 7});
    inv_idx.add_posting("test", 1, 1, {5});
#else
    inv_idx.add_posting("test", 0, 3);
    inv_idx.add_posting("test", 1, 1);
#endif

    inv_idx.finalize(3, fwd_idx.avg_doc_length());

    std::vector<std::string> terms = {"test"};
    scorer sc(inv_idx, fwd_idx, terms);

    float s0 = sc.score(0);
    float s1 = sc.score(1);
    float s2 = sc.score(2);

    check(s0 > 0.0f, "doc0 has positive score for 'test'");
    check(s1 > 0.0f, "doc1 has positive score for 'test'");
    check(s2 == 0.0f, "doc2 has zero score (term not present)");

    // doc0 has tf=3 in a short doc, should score higher than doc1 with tf=1 in longer doc
    check(s0 > s1, "higher tf in shorter doc scores higher");
}

void test_query_executor() {
    std::cout << "\n--- query executor ---\n";

    inverted_index inv_idx;
    forward_index fwd_idx;

    fwd_idx.add_document(0, 10, "doc about rust");
    fwd_idx.add_document(1, 15, "doc about python");
    fwd_idx.add_document(2, 12, "doc about rust and python");

    // stem "rust" and "python" by hand for test consistency
#ifdef ENABLE_POSITIONS
    inv_idx.add_posting("rust", 0, 3, {0, 4, 8});
    inv_idx.add_posting("rust", 2, 2, {0, 6});
    inv_idx.add_posting("python", 1, 4, {0, 3, 7, 12});
    inv_idx.add_posting("python", 2, 1, {3});
#else
    inv_idx.add_posting("rust", 0, 3);
    inv_idx.add_posting("rust", 2, 2);
    inv_idx.add_posting("python", 1, 4);
    inv_idx.add_posting("python", 2, 1);
#endif

    inv_idx.finalize(3, fwd_idx.avg_doc_length());

    query_executor exec(inv_idx, fwd_idx);
    query_config cfg;
    cfg.top_k = 10;

    // single term query
    auto ast = query_node::make_term("rust");
    auto results = exec.execute(*ast, cfg);
    check(results.size() == 2, "single term query returns 2 results");
    check(results[0].score >= results[1].score, "results sorted by score descending");

    // AND query
    auto and_ast = query_node::make_and(
        query_node::make_term("rust"),
        query_node::make_term("python")
    );
    auto and_results = exec.execute(*and_ast, cfg);
    check(and_results.size() == 1, "AND query returns only docs with both terms");
    check(and_results[0].doc_id == 2, "AND result is doc2");

    // NOT query
    auto not_ast = query_node::make_not(
        query_node::make_term("rust"),
        query_node::make_term("python")
    );
    auto not_results = exec.execute(*not_ast, cfg);
    check(not_results.size() == 1, "NOT query excludes correct doc");
    check(not_results[0].doc_id == 0, "NOT result is doc0 (has rust but not python)");
}

void test_serialization() {
    std::cout << "\n--- serialization ---\n";

    inverted_index inv_idx;
    forward_index fwd_idx;

    fwd_idx.add_document(0, 10, "first doc");
    fwd_idx.add_document(1, 20, "second doc");

#ifdef ENABLE_POSITIONS
    inv_idx.add_posting("hello", 0, 2, {0, 5});
    inv_idx.add_posting("hello", 1, 1, {3});
    inv_idx.add_posting("world", 0, 1, {1});
#else
    inv_idx.add_posting("hello", 0, 2);
    inv_idx.add_posting("hello", 1, 1);
    inv_idx.add_posting("world", 0, 1);
#endif

    inv_idx.finalize(2, fwd_idx.avg_doc_length());

    std::string path = "test_index.bin";
    check(serialize_index(path, inv_idx, fwd_idx), "serialization succeeds");

    inverted_index inv_idx2;
    forward_index fwd_idx2;
    check(deserialize_index(path, inv_idx2, fwd_idx2), "deserialization succeeds");

    check(inv_idx2.vocabulary_size() == inv_idx.vocabulary_size(),
          "vocab size preserved");
    check(fwd_idx2.doc_count() == fwd_idx.doc_count(),
          "doc count preserved");
    check(std::abs(inv_idx2.avg_doc_length - inv_idx.avg_doc_length) < 0.01,
          "avg doc length preserved");

    const auto* pl = inv_idx2.lookup("hello");
    check(pl != nullptr, "term 'hello' found after deserialization");
    check(pl->entries.size() == 2, "'hello' posting count preserved");

    // cleanup
    std::remove(path.c_str());
}

}  // namespace

int main() {
    std::cout << "=== search engine tests ===\n";

    test_tokenizer();
    test_stemmer();
    test_stopwords();
    test_normalizer();
    test_inverted_index();
    test_posting_list_operations();
    test_query_parser();
    test_bm25_scorer();
    test_query_executor();
    test_serialization();

    std::cout << "\n=== results: " << tests_passed << "/" << tests_run << " passed ===\n";

    return tests_passed == tests_run ? 0 : 1;
}
