#include "static_thread_pool.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace thread {

StaticThreadPool::StaticThreadPool(const unsigned short threadsCount):
workers_(threadsCount),
queue_(),
queueEventDriver_(),
queueLock_() {
    std::generate_n(workers_.begin(), threadsCount, [this]() {
        return std::thread([this]() {
            work();
        });
    });
}

void StaticThreadPool::submitTask(const StaticThreadPool::Task &task) {
    std::lock_guard<std::mutex> lock(queueLock_);
    try {
        queue_.push(task);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error("Can`t submit a new task to static thread pool. "
                                 "Resource limit exceeded");
    }
    queueEventDriver_.notify_one();
}

void StaticThreadPool::join() {
    for (auto i = 0; i < workers_.size(); ++i) {
        submitTask({});
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
            break;
        } else {
            task();
        }
    }
}

} // namespace thread