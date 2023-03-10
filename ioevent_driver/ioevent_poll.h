#pragma once

#include "ioevent_driver.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <mutex>

#include <poll.h>

namespace ioevent {

struct LockFreeMute {
    inline void lock() {}
    inline void unlock() {}
};

template<typename Mutex>
class IOEventPool;

template<typename Mutex = LockFreeMute>
using SingleThreadIOEventDriver = IOEventPool<Mutex>;

template<typename Mutex = std::mutex>
using MultiThreadIOEventDriver = IOEventPool<Mutex>; 

template<typename Mutex>
class IOEventPool : public IOEventDriver {
public:
    IOEventPool();

protected:
    bool subscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler eventHandler) override;
    bool unsubscribeFromEvent(int fd) override;
    void runEventLoop() override;
    void stopEventLoop() override;

private:
    void handleNewEvents(int eventCount);
    void updateCachesForAppend();
    void updateCachesForDelete();
    void updateCaches();

    using PollFdStruct = struct pollfd;

    struct UserEventHandlerStruct {
        int fd;
        IOEventDriverType eventType;
        IOEventHandler eventHandler;
    };

    struct CacheStruct {
        UserEventHandlerStruct userEventHandler;
        PollFdStruct pollFd;
    };

    std::queue<CacheStruct> cacheForAppend_;
    std::queue<int> cacheForDelete_;

    std::vector<PollFdStruct> fdSet_;
    std::vector<UserEventHandlerStruct> userEventHandlers_;

    Mutex mutexForEventHandlers_;
    Mutex mutexCachForAppend_;
    Mutex mutexCachForDeleter_;
    
    bool isStoped_;
};

template<typename Mutex>
IOEventPool<Mutex>::IOEventPool():
cacheForAppend_(),
cacheForDelete_(),
fdSet_(),
userEventHandlers_(),
mutexForEventHandlers_(),
mutexCachForAppend_(),
mutexCachForDeleter_(),
isStoped_(true) 
{}

template<typename Mutex>
void IOEventPool<Mutex>::runEventLoop() {
    mutexForEventHandlers_.lock();
    isStoped_ = false;
    mutexForEventHandlers_.unlock();
    updateCaches();

    while (!isStoped_) {
        mutexForEventHandlers_.lock();
        const auto res = poll(fdSet_.data(), fdSet_.size(), -1);
        if (res < 0) {
            throw std::runtime_error("IOEventPool::runEventLoop: sys call pool was failed");
        }

        handleNewEvents(res);
        mutexForEventHandlers_.unlock();

        updateCaches();
    }
}

template<typename Mutex>
void IOEventPool<Mutex>::handleNewEvents(int eventCount) {
    for (auto i = 0; i < fdSet_.size() && eventCount > 0; ++i) {
        const auto curFd = fdSet_[i];
        const auto eventHandler = userEventHandlers_[i].eventHandler;
        if (curFd.revents == curFd.events) {
            eventHandler(curFd.fd);
            --eventCount;
        }
    }
}

template<typename Mutex>
void IOEventPool<Mutex>::updateCaches() {
    updateCachesForAppend();
    updateCachesForDelete();
}

template<typename Mutex>
void IOEventPool<Mutex>::updateCachesForAppend() {
    mutexCachForAppend_.lock();
    mutexForEventHandlers_.lock();

    while (!cacheForAppend_.empty()) {
        const auto [userEventHandler, pollFd] = cacheForAppend_.front();
        cacheForAppend_.pop();
        fdSet_.push_back(pollFd);
        userEventHandlers_.push_back(userEventHandler);
    }

    mutexForEventHandlers_.unlock();
    mutexCachForAppend_.unlock();
}

template<typename Mutex>
void IOEventPool<Mutex>::updateCachesForDelete() {
    mutexCachForDeleter_.lock();
    mutexForEventHandlers_.lock();

    while (!cacheForDelete_.empty()) {
        auto fd = cacheForDelete_.front();
        cacheForDelete_.pop();
        
        std::stringstream ss;
        ss << "IOEventPool::unsubscribeFromEvent: Can`t erase fd: " << fd << "becasu it doesn` exist";

        const auto fdSetIt = std::find_if(fdSet_.cbegin(), fdSet_.cend(), [fd](const auto& pollFd) {
            return pollFd.fd == fd;
        });

        if (fdSetIt != fdSet_.cend()) {
            fdSet_.erase(fdSetIt);
        } else {
            throw std::runtime_error(ss.str());
        }


        const auto userEventHandlerIt = std::find_if(userEventHandlers_.cbegin(), userEventHandlers_.cend(), [fd](const auto& userEventHarder) {
            return userEventHarder.fd == fd;
        });

        if (userEventHandlerIt != userEventHandlers_.cend()) {
            userEventHandlers_.erase(userEventHandlerIt);
        } else {
            throw std::runtime_error(ss.str());
        }
    }

    mutexForEventHandlers_.unlock();
    mutexCachForDeleter_.unlock();
}

template<typename Mutex>
void IOEventPool<Mutex>::stopEventLoop() {
    mutexForEventHandlers_.lock();
    isStoped_ = true;
    mutexForEventHandlers_.unlock();
}

template<typename Mutex>
bool IOEventPool<Mutex>::subscribeToEvent(const int fd, const IOEventDriverType eventType, const IOEventHandler eventHandler) {
    CacheStruct cachRecord;
    {
        PollFdStruct pollFd = {.fd = fd};
        switch (eventType) {
            case IOEventDriverType::Read: {
                pollFd.events = POLLRDNORM;
                break;
            }
            case IOEventDriverType::Write: {
                pollFd.events = POLLWRNORM;
                break;
            }
            default: {}
        }
        const UserEventHandlerStruct userEventHandler = {.fd = fd, .eventType = eventType, .eventHandler = eventHandler};
        
        cachRecord.userEventHandler = userEventHandler;
        cachRecord.pollFd = pollFd;
    }
    
    mutexCachForAppend_.lock();
    cacheForAppend_.push(cachRecord);
    mutexCachForAppend_.unlock();
    return true;
}

template<typename Mutex>
bool IOEventPool<Mutex>::unsubscribeFromEvent(const int fd) {
    mutexCachForDeleter_.lock();
    cacheForDelete_.push(fd);
    mutexCachForDeleter_.unlock();
    return true;
}

} // namespace ioevent
