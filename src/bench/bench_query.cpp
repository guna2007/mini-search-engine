#include "bench/bench_runner.h"
#include "indexer/index_builder.h"
#include "indexer/stopwords.h"
#include "query/query_executor.h"

#include <iostream>
#include <vector>
#include <string>
#include <random>

std::vector<std::string> generate_queries(const inverted_index& idx, size_t count) {
    std::vector<std::string> all_terms;
    all_terms.reserve(idx.terms.size());
    for (const auto& [term, pl] : idx.terms) {
        if (pl.entries.size() > 2) {
            all_terms.push_back(term);
        }
    }
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, all_terms.size() - 1);

    std::vector<std::string> queries;
    queries.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        double r = static_cast<double>(i) / count;
        if (r < 0.40) queries.push_back(all_terms[dist(rng)]);
        else if (r < 0.75) queries.push_back(all_terms[dist(rng)] + " " + all_terms[dist(rng)]);
        else queries.push_back(all_terms[dist(rng)] + " AND " + all_terms[dist(rng)]);
    }
    return queries;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bench_query <corpus_path>\n";
        return 1;
    }

    load_stopwords("data/stopwords.txt");
    index_builder builder;
    (void)builder.build_from_jsonl(argv[1]);

    query_executor executor(builder.inv_index, builder.fwd_index);
    auto queries = generate_queries(builder.inv_index, 50000);
    query_config config;
    config.top_k = 10;

    const int warmup_count  = 500;
    const int measure_count = 10000;

    for (int i = 0; i < warmup_count; i++) {
        auto ast = parse_query(queries[i]);
        if (ast) (void)executor.execute(*ast, config);
    }

    std::vector<int64_t> latencies;
    latencies.reserve(measure_count);

    for (int i = 0; i < measure_count; i++) {
        auto ast = parse_query(queries[i % queries.size()]);
        if (!ast) continue;
        
        auto t0 = now_ns();
        (void)executor.execute(*ast, config);
        auto t1 = now_ns();
        latencies.push_back(t1 - t0);
    }

    std::cout << "latency_us\n";
    for (auto ns : latencies)
        std::cout << ns / 1000.0 << "\n";

    std::vector<double> lat_us;
    for (auto ns : latencies) lat_us.push_back(ns / 1000.0);
    auto s = compute_percentiles(lat_us);

    std::cerr << "p50="  << s.p50_us  << "µs "
              << "p95="  << s.p95_us  << "µs "
              << "p99="  << s.p99_us  << "µs "
              << "p99.9=" << s.p999_us << "µs\n";

    return 0;
}
