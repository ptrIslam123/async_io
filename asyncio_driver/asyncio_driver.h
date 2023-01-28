#pragma once

#include <future>
#include <memory>
#include <functional>

#include "thread_pool/static_thread_pool.h"
#include "ioevent_driver/ioevent_driver.h"

namespace asyncio {
    
class AsyncIODriver {
public:
    template<typename R>
    using CallbackType = std::function<R(int fd)>;

    using IOEventDriverPtr = std::unique_ptr<ioevent::IOEventDriver>;
    using ThreadPoolPtr = std::unique_ptr<thread::StaticThreadPool>;

    AsyncIODriver(IOEventDriverPtr&& ioEventDriver, ThreadPoolPtr&& threadPool);

    void run();
    void stop();

    template<typename R, typename Callback>
    std::future<R> read(int fd, Callback callback);

    template<typename R, typename Callback>
    std::future<R> write(int fd, Callback callback);

private:
    IOEventDriverPtr iOEventDriver_;
    ThreadPoolPtr threadPool_;
};

template<typename R, typename Callback>
std::future<R> AsyncIODriver::read(const int fd, Callback callback) {
    std::promise<R> promise(callback);
    auto future = promise.get_future();

    iOEventDriver_->subscribeToEvent(
        fd, 
        ioevent::IOEventDriverType::Read, 
        [threadPool = threadPool_.get(), callback = std::move(callback), promise = std::move(promise)](const int fd) {
            threadPool->spawnTask([fd, callback = std::move(callback), promise = std::move(promise)]() {
                try {
                    promise.set_value(callback(fd));
                } catch(...) {
                    // handle ...
                }
            });
        }
    );
    return future;
}

template<typename R, typename Callback>
std::future<R> AsyncIODriver::write(int fd, Callback callback) {
    std::promise<R> promise;
    auto future = promise.get_future();

    iOEventDriver_->subscribeToEvent(
        fd, 
        ioevent::IOEventDriverType::Write, 
        [threadPool = threadPool_.get(), callback = std::move(callback), promise = std::move(promise)](const int fd) {
            threadPool->spawnTask([fd, callback = std::move(callback), promise = std::move(promise)]() {
                try {
                    promise.set_value(callback(fd));
                } catch(...) {
                    // handle ...
                }
            });
        }
    );
    return future; 
}

} // namespace asyncio
