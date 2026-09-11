#include "bench/bench_runner.h"
#include "indexer/index_builder.h"
#include "indexer/stopwords.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bench_index <corpus_path>\n";
        return 1;
    }

    load_stopwords("data/stopwords.txt");
    
    // Custom logic to load jsonl line by line so we can slice it
    std::vector<std::string> lines;
    std::ifstream f(argv[1]);
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(line);
    }

    std::vector<size_t> sizes = {5000, 10000, 25000, 50000};
    
    std::cout << "num_docs,build_time_ms\n";
    for (auto n : sizes) {
        if (n > lines.size()) break;
        
        // Write subset to a temp file
        std::ofstream tmp("temp_subset.jsonl");
        for (size_t i = 0; i < n; i++) tmp << lines[i] << "\n";
        tmp.close();

        index_builder builder;
        auto t0 = now_ns();
        (void)builder.build_from_jsonl("temp_subset.jsonl");
        auto t1 = now_ns();
        
        double ms = (t1 - t0) / 1'000'000.0;
        std::cout << n << "," << ms << "\n";
        std::cerr << "docs=" << n << " time=" << ms << "ms\n";
    }

    std::remove("temp_subset.jsonl");
    return 0;
}
