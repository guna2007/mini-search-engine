#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// fixed-size thread pool for concurrent query processing
// the index is shared read-only, so no locking is needed for index access
struct thread_pool {
    explicit thread_pool(size_t num_threads);
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    // submit a task to the pool
    void submit(std::function<void()> task);

    // wait for all submitted tasks to complete
    void wait_idle();

    [[nodiscard]] size_t num_threads() const { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable idle_cv_;
    size_t active_tasks_ = 0;
    bool shutdown_ = false;
};
