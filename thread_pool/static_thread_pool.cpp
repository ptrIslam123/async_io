#include "static_thread_pool.h"

#include <algorithm>
#include <stdexcept>

namespace thread {

StaticThreadPool::StaticThreadPool(const unsigned short threadsCount):
workers_(threadsCount),
queue_(),
queueEventDriver_(),
queueLock_(),
workerCount_(0)
{}

void StaticThreadPool::run() {
    std::generate_n(workers_.begin(), workers_.size(), [this]() {
        return std::thread([this]() {
            work();
        });
    });
    workerCount_.store(static_cast<int>(workers_.size()));
}

void StaticThreadPool::spawnTask(const StaticThreadPool::Task&& task) {
    std::lock_guard<std::mutex> lock(queueLock_);
    try {
        queue_.push(std::move(task));
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error("Can`t submit a new task to static thread pool. "
                                 "Resource limit exceeded");
    }
    queueEventDriver_.notify_one();
}

void StaticThreadPool::join() {
    for (auto i = 0; i < workers_.size(); ++i) {
        spawnTask({});
    }

    for(auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void StaticThreadPool::work() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueLock_);
        while (queue_.empty()) {
            queueEventDriver_.wait(lock);
        }

        auto task = queue_.back();
        queue_.pop();
        lock.unlock();

        if (!task) {
            auto curCount = workerCount_.load();
            while (workerCount_.compare_exchange_weak(curCount, curCount - 1)) {}
            break;
        } else {
            task();
        }
    }
}

} // namespace thread
