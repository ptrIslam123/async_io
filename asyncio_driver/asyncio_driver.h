#pragma once

#include <future>
#include <memory>
#include <functional>
#include <iostream>

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

    template<typename R>
    std::future<R> read(int fd, CallbackType<R> callback);

    template<typename R>
    std::future<R> write(int fd, CallbackType<R> callback);

private:
    template<typename R>
    std::future<R> makeASyncIOTask(int fd, ioevent::IOEventDriverType ioEventType, CallbackType<R>&& callback);

    IOEventDriverPtr iOEventDriver_;
    ThreadPoolPtr threadPool_;
};

template<typename R>
inline std::future<R> AsyncIODriver::read(const int fd, CallbackType<R> callback) {
    return makeASyncIOTask<R>(fd, ioevent::IOEventDriverType::Read, std::move(callback));
}

template<typename R>
inline std::future<R> AsyncIODriver::write(int fd, CallbackType<R> callback) {
    return makeASyncIOTask<R>(fd, ioevent::IOEventDriverType::Write, std::move(callback));
}

template<typename R>
std::future<R> AsyncIODriver::makeASyncIOTask(int fd, ioevent::IOEventDriverType ioEventType, CallbackType<R>&& callback) {
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();
    std::cout << "promise addr=" << promise.get() << std::endl;

//    auto ioTaskForExecOnThreadPool = [fd, ioEventDriver = iOEventDriver_.get(), callback = std::move(callback), promise = std::move(promise)]() mutable {
//        try {
//            std::cout << "call callback with fd=" << fd << " and promise addr=" << promise.get() << std::endl;
//            promise->set_value(callback(fd));
//            if (!ioEventDriver->unsubscribeFromEvent(fd)) {
//                // TODO
//            }
//        } catch (const std::exception& e) {
//            // TODO
//            std::cout << "##BED: exception with message=" << e.what() << std::endl;
//        } catch (...) {
//            // TODO
//        }
//    };

    struct Foo {
        std::shared_ptr<std::promise<R>> promise;
        CallbackType<R> callback;
        ioevent::IOEventDriver* ioEventDriver;
        int fd;
        Foo(std::shared_ptr<std::promise<R>>&& promise, CallbackType<R>&& callback, ioevent::IOEventDriver* ioEventDriver, int fd):
        promise(std::move(promise)),
        callback(std::move(callback)),
        ioEventDriver(ioEventDriver),
        fd(fd) {}

        void print() const {
            std::cout << "this=" << this << " promise addr=" << promise.get() << " fd=" << fd << std::endl;
        }

        void operator()() {
            try {
                std::cout << "call callback with fd=" << fd << " and promise addr=" << promise.get() << std::endl;
                promise->set_value(callback(fd));
                if (!ioEventDriver->unsubscribeFromEvent(fd)) {
                    // TODO
                }
            } catch (const std::exception& e) {
                // TODO
                std::cout << "##BED: exception with message=" << e.what() << std::endl;
            } catch (...) {
                // TODO
            }
        }
    };

   Foo foo(std::move(promise), std::move(callback), iOEventDriver_.get(), fd);
   iOEventDriver_->subscribeToEvent(fd, ioEventType, [threadPool = threadPool_.get(), callback = std::move(foo)](const int fd) {
       callback.print();
       threadPool->spawnTask(callback);
        return ioevent::DescriptorState::Closed;
    });
    return std::move(future);
}

} // namespace asyncio
