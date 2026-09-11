#include "bench/bench_runner.h"
#include "indexer/index_builder.h"
#include "indexer/stopwords.h"
#include "query/query_executor.h"

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
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
        queries.push_back(all_terms[dist(rng)]);
    }
    return queries;
}

static double measure_qps(query_executor& executor,
                          const std::vector<std::string>& queries,
                          int num_threads,
                          int duration_seconds) {
    std::atomic<int64_t> total_queries{0};
    std::atomic<bool>    stop{false};

    auto worker = [&](int tid) {
        int64_t count = 0;
        int i = tid;
        query_config config;
        config.top_k = 10;

        while (!stop.load(std::memory_order_relaxed)) {
            auto ast = parse_query(queries[i % queries.size()]);
            if (ast) {
                (void)executor.execute(*ast, config);
                count++;
            }
            i += num_threads;
        }
        total_queries.fetch_add(count);
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; t++)
        threads.emplace_back(worker, t);

    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop.store(true);

    for (auto& t : threads) t.join();

    return static_cast<double>(total_queries) / duration_seconds;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bench_throughput <corpus_path>\n";
        return 1;
    }

    load_stopwords("data/stopwords.txt");
    index_builder builder;
    (void)builder.build_from_jsonl(argv[1]);

    query_executor executor(builder.inv_index, builder.fwd_index);
    auto queries = generate_queries(builder.inv_index, 100000);

    std::vector<int> thread_counts = {1, 2, 4, 8, 12, 16};
    const int duration = 5;  // seconds per measurement

    std::cout << "threads,qps\n";
    for (int n : thread_counts) {
        double qps = measure_qps(executor, queries, n, duration);
        std::cout << n << "," << static_cast<int>(qps) << "\n";
        std::cerr << "threads=" << n << " qps=" << qps << "\n";
    }

    return 0;
}
