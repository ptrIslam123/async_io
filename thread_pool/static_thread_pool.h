#pragma once

#include <vector>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace thread {

class StaticThreadPool final {
public:
    using Workers = std::vector<std::thread>;
    using Task = std::function<void()>;
    using TaskQueue = std::queue<Task>;

    explicit StaticThreadPool(unsigned short threadsCount = 1U);
    StaticThreadPool(StaticThreadPool &&other) = default;
    StaticThreadPool &operator=(StaticThreadPool &&other) = default;
    StaticThreadPool(const StaticThreadPool &other) = delete;
    StaticThreadPool &operator=(const StaticThreadPool &other) = delete;
    void run();
    void spawnTask(const Task&& task);
    void join();

private:
    void work();

    Workers workers_;
    TaskQueue queue_;
    std::condition_variable queueEventDriver_;
    std::mutex queueLock_;
    std::atomic<int> workerCount_;
};

} // namespace thread
