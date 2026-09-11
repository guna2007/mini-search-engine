#include "indexer/index_builder.h"
#include "indexer/normalizer.h"
#include "indexer/stopwords.h"
#include "query/query_executor.h"
#include "query/query_parser.h"
#include "storage/serializer.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

struct cli_config {
    std::string corpus_path;
    std::string index_path = "index.bin";
    std::string stopwords_path = "data/stopwords.txt";
    std::string mode = "interactive";   // "build", "query", "interactive"
    std::string query;
    uint32_t top_k = 10;
    uint32_t cache_size = 1024;
    float k1 = 1.2f;
    float b = 0.75f;
};

void print_usage(const char* prog) {
    std::cerr << "usage: " << prog << " [options]\n"
              << "\noptions:\n"
              << "  --corpus <path>       path to jsonl file or text directory\n"
              << "  --index <path>        index file path (default: index.bin)\n"
              << "  --stopwords <path>    stopwords file (default: data/stopwords.txt)\n"
              << "  --mode <mode>         build | query | interactive (default: interactive)\n"
              << "  --query <string>      query string (for query mode)\n"
              << "  --top-k <n>           results per query (default: 10)\n"
              << "  --k1 <float>          bm25 k1 parameter (default: 1.2)\n"
              << "  --b <float>           bm25 b parameter (default: 0.75)\n"
              << "  --cache-size <n>      lru cache capacity (default: 1024)\n"
              << "  --help                show this message\n";
}

cli_config parse_args(int argc, char* argv[]) {
    cli_config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (i + 1 >= argc) break;

        if (arg == "--corpus")    cfg.corpus_path = argv[++i];
        else if (arg == "--index")     cfg.index_path = argv[++i];
        else if (arg == "--stopwords") cfg.stopwords_path = argv[++i];
        else if (arg == "--mode")      cfg.mode = argv[++i];
        else if (arg == "--query")     cfg.query = argv[++i];
        else if (arg == "--top-k")     cfg.top_k = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--k1")        cfg.k1 = std::stof(argv[++i]);
        else if (arg == "--b")         cfg.b = std::stof(argv[++i]);
        else if (arg == "--cache-size") cfg.cache_size = static_cast<uint32_t>(std::stoul(argv[++i]));
    }

    return cfg;
}

void print_results(const std::vector<search_result>& results) {
    if (results.empty()) {
        std::cout << "  (no results)\n";
        return;
    }

    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i + 1) << ". [" << results[i].doc_id << "] "
                  << results[i].title
                  << "  (score: " << results[i].score << ")\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    auto cfg = parse_args(argc, argv);
    load_stopwords(cfg.stopwords_path);

    inverted_index inv_idx;
    forward_index fwd_idx;

    if (cfg.mode == "build") {
        if (cfg.corpus_path.empty()) {
            std::cerr << "error: --corpus required for build mode\n";
            return 1;
        }

        index_builder builder;
        index_stats stats;

        if (std::filesystem::is_directory(cfg.corpus_path)) {
            stats = builder.build_from_directory(cfg.corpus_path);
        } else {
            stats = builder.build_from_jsonl(cfg.corpus_path);
        }

        std::cout << "index built: " << stats.docs_indexed << " docs, "
                  << stats.vocab_size << " terms, "
                  << stats.total_postings << " postings\n"
                  << "build time: " << stats.build_time_ms << " ms\n";

        if (serialize_index(cfg.index_path, builder.inv_index, builder.fwd_index)) {
            std::cout << "index saved to " << cfg.index_path << "\n";
        }
        return 0;
    }

    if (cfg.mode == "query") {
        if (cfg.query.empty()) {
            std::cerr << "error: --query required for query mode\n";
            return 1;
        }

        // try to load from disk, otherwise build
        if (std::filesystem::exists(cfg.index_path)) {
            std::cout << "loading index from " << cfg.index_path << "...\n";
            if (!deserialize_index(cfg.index_path, inv_idx, fwd_idx)) {
                return 1;
            }
        } else if (!cfg.corpus_path.empty()) {
            index_builder builder;
            if (std::filesystem::is_directory(cfg.corpus_path)) {
                (void)builder.build_from_directory(cfg.corpus_path);
            } else {
                (void)builder.build_from_jsonl(cfg.corpus_path);
            }
            inv_idx = std::move(builder.inv_index);
            fwd_idx = std::move(builder.fwd_index);
        } else {
            std::cerr << "error: need --index or --corpus\n";
            return 1;
        }

        auto ast = parse_query(cfg.query);
        if (!ast) {
            std::cerr << "error: could not parse query\n";
            return 1;
        }

        query_executor executor(inv_idx, fwd_idx);
        query_config qcfg;
        qcfg.top_k = cfg.top_k;
        qcfg.bm25 = {cfg.k1, cfg.b};

        auto results = executor.execute(*ast, qcfg);
        print_results(results);
        return 0;
    }

    // interactive mode
    if (std::filesystem::exists(cfg.index_path)) {
        std::cout << "loading index from " << cfg.index_path << "...\n";
        if (!deserialize_index(cfg.index_path, inv_idx, fwd_idx)) {
            return 1;
        }
    } else if (!cfg.corpus_path.empty()) {
        index_builder builder;
        if (std::filesystem::is_directory(cfg.corpus_path)) {
            (void)builder.build_from_directory(cfg.corpus_path);
        } else {
            (void)builder.build_from_jsonl(cfg.corpus_path);
        }
        inv_idx = std::move(builder.inv_index);
        fwd_idx = std::move(builder.fwd_index);

        std::cout << "saving index to " << cfg.index_path << "...\n";
        serialize_index(cfg.index_path, inv_idx, fwd_idx);
    } else {
        std::cerr << "error: need --index or --corpus to start\n";
        return 1;
    }

    std::cout << "index loaded: " << fwd_idx.doc_count() << " docs, "
              << inv_idx.vocabulary_size() << " terms\n"
              << "enter queries (empty line to quit):\n\n";

    query_executor executor(inv_idx, fwd_idx);
    query_cache cache(cfg.cache_size);
    executor.cache = &cache;
    query_config qcfg;
    qcfg.top_k = cfg.top_k;
    qcfg.bm25 = {cfg.k1, cfg.b};

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) break;

        auto ast = parse_query(line);
        if (!ast) {
            std::cout << "  (could not parse query)\n\n";
            continue;
        }

        auto results = executor.execute_cached(line, *ast, qcfg);
        print_results(results);
        std::cout << "\n";
    }

    return 0;
}
