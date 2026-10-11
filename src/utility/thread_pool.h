#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    // Runs fn(0..count-1) on the pool and blocks until all calls returned.
    // Indices are handed out dynamically, so uneven work balances itself.
    // Must not be called from a pool thread (it would wait on itself).
    void parallel_for(size_t count, const std::function<void(size_t)> &fn);
    
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    
    void workerThread();
};
