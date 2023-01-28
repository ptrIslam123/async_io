#pragma once

#include <future>
#include <memory>

#include "thread_pool/static_thread_pool.h"
#include "ioevent_driver/ioevent_driver.h"

namespace asyncio {
    
class AsyncIODriver {
public:
    using IOEventDriverPtr = std::unique_ptr<ioevent::IOEventDriver>;
    using ThreadPoolPtr = std::unique_ptr<thread::StaticThreadPool>;

    AsyncIODriver(IOEventDriverPtr&& ioEventDriver, ThreadPoolPtr&& threadPool);

    void run();
    void stop();

    template<typename R, typename Callback>
    std::future<R> read(int fd, ioevent::IOEventDriverType ioEventType, Callback callback);

    template<typename R, typename Callback>
    std::future<R> write(int fd, ioevent::IOEventDriverType ioEventType, Callback Callback);

private:
    IOEventDriverPtr iOEventDriver_;
    ThreadPoolPtr threadPool_;
};

template<typename R, typename Callback>
std::future<R> AsyncIODriver::read(int fd, ioevent::IOEventDriverType ioEventType, Callback callback) {

}

template<typename R, typename Callback>
std::future<R> AsyncIODriver::write(int fd, ioevent::IOEventDriverType ioEventType, Callback Callback) {
    
}

} // namespace asyncio
