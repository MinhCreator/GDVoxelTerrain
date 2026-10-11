#include "thread_pool.h"
#include <algorithm>
#include <atomic>

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] { workerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (auto &worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(task);
    }
    condition.notify_one();
}

void ThreadPool::parallel_for(size_t count, const std::function<void(size_t)> &fn) {
    if (count == 0)
        return;
    std::atomic<size_t> next{0};
    std::mutex doneMutex;
    std::condition_variable doneCv;
    size_t done = 0;
    const size_t runners = std::min(count, workers.size());
    for (size_t r = 0; r < runners; ++r) {
        enqueue([&] {
            for (size_t i; (i = next.fetch_add(1)) < count;)
                fn(i);
            // Notify under the lock: the waiter owns doneCv and may return
            // (destroying it) as soon as it observes done == runners.
            std::lock_guard<std::mutex> lock(doneMutex);
            ++done;
            doneCv.notify_one();
        });
    }
    std::unique_lock<std::mutex> lock(doneMutex);
    doneCv.wait(lock, [&] { return done == runners; });
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}
