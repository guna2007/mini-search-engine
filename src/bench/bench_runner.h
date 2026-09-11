#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

struct latency_stats {
    double p50_us;
    double p95_us;
    double p99_us;
    double p999_us;
    double mean_us;
    double min_us;
    double max_us;
    size_t count;
};

inline latency_stats compute_percentiles(std::vector<double>& latencies_us) {
    if (latencies_us.empty()) return {};

    std::sort(latencies_us.begin(), latencies_us.end());

    size_t n = latencies_us.size();
    double sum = std::accumulate(latencies_us.begin(), latencies_us.end(), 0.0);

    return {
        latencies_us[n * 50 / 100],
        latencies_us[n * 95 / 100],
        latencies_us[std::min(n * 99 / 100, n - 1)],
        latencies_us[std::min(n * 999 / 1000, n - 1)],
        sum / static_cast<double>(n),
        latencies_us.front(),
        latencies_us.back(),
        n
    };
}

inline void print_latency_stats(const std::string& label, const latency_stats& stats) {
    std::cout << label << ":\n"
              << "  count:  " << stats.count << "\n"
              << "  p50:    " << stats.p50_us << " us\n"
              << "  p95:    " << stats.p95_us << " us\n"
              << "  p99:    " << stats.p99_us << " us\n"
              << "  p999:   " << stats.p999_us << " us\n"
              << "  mean:   " << stats.mean_us << " us\n"
              << "  min:    " << stats.min_us << " us\n"
              << "  max:    " << stats.max_us << " us\n";
}

// measure peak rss in bytes (mac only for now, linux via /proc/self/status)
inline size_t get_peak_rss() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size_max;
    }
    return 0;
#else
    // linux fallback
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    size_t rss = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmHWM:", 6) == 0) {
            sscanf(line + 6, "%zu", &rss);
            rss *= 1024;  // convert kb to bytes
            break;
        }
    }
    fclose(f);
    return rss;
#endif
}

// returns elapsed nanoseconds
inline int64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

// high-resolution timer wrapper
struct timer {
    int64_t start_time;

    void start() { start_time = now_ns(); }

    [[nodiscard]] double elapsed_ms() const {
        return (now_ns() - start_time) / 1'000'000.0;
    }

    [[nodiscard]] double elapsed_us() const {
        return (now_ns() - start_time) / 1000.0;
    }
};

// run a function `iterations` times and collect latency data
// runs `warmup` iterations first to prime caches
template <typename F>
latency_stats benchmark(F&& fn, size_t iterations, size_t warmup = 100) {
    // warmup phase
    for (size_t i = 0; i < warmup; ++i) {
        fn();
    }

    std::vector<double> latencies;
    latencies.reserve(iterations);

    timer t;
    for (size_t i = 0; i < iterations; ++i) {
        t.start();
        fn();
        latencies.push_back(t.elapsed_us());
    }

    return compute_percentiles(latencies);
}
