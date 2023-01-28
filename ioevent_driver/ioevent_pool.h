#pragma once

#include "ioevent_driver.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <pool.h>

namespace ioevent {

struct LockFreeMute {
    void lock() {}
    void unlock() {}
};

template<typename Mutex = LockFreeMute>
class IOEventPool : public IOEventDriver {
public:
    IOEventPool();

protected:
    bool sibscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler eventHandler) override;
    bool unsibscribeFromEvent(int fd) override;
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
    Mutex mutex_;
    bool isStoped_;
};

template<typename Mutex>
IOEventPool::IOEventPool():


template<typename Mutex>
void IOEventPool::runEventLoop() {
    mutex_.lock();
    isStoped_ = false;
    mutex_.unlock();

    while (!isStoped_) {
        const auto res = pool(fdSet_.data(), fdSet_.size(), NULL, -1);
        if (res < 0) {
            throw std::runtime_exception("IOEventPool::runEventLoop: sys call pool was failed");
        }

        handleNewEvents(res);
        updateEventListeners();
    }
}

template<typename Mutex>
void IOEventPool::handleNewEvents(int eventCount) {
    for (auto i = 0; i < fdSet_.size() && eventCount > 0; ++i) {
        const auto curFd = fdSet_[i];
        if (curFd.revet == curFd.event) {
            curFd.eventHandler(curFd.fd);
            --eventCount;
        }
    }
}

template<typename Mutex>
void IOEventPool::updateCaches() {
    mutex_.lock();
    updateCachesForAppend();
    updateCachesForDelete();
    mutex_.unlock();
}

template<typename Mutex>
void IOEventPool::updateCachesForAppend() {
    while (!cacheForAppend_.empty()) {
        const auto [userEventHandler, pollFd] = cacheForAppend_.top();
        cacheForAppend_.pop();
        fdSet_.push_back(pollFd);
        userEventHandlers_.push_back(userEventHandler);
    }
}

template<typename Mutex>
void IOEventPool::updateCachesForDelete() {
    while (!cacheForDelete_.empty()) {
        const auto fd = cacheForDelete_.top();
        cacheForDelete_.pop();
        
        std::strinngstream ss;
        ss << "IOEventPool::unsibscribeFromEvent: Can`t erase fd: " << fd << "becasu it doesn` exist";

        const auto fdSetIt = std::find_if(fdSet_.cbegin(), fdSet_.cend(), [fd](const auto& pollFd) {
            return pollFd.fd == fd;
        });

        if (fdSetIt != fdSet_.cend()) {
            fdSet_.erase(fdSetIt);
        } else {
            throw std::runtime_exception(ss.str());
        }


        const auto userEventHandlerIt = std::find_if(userEventHandlers_.cbegin(), userEventHandlers_.cend(), [fd](const auto& userEventHarder) {
            return userEventHandler.fd = fd;
        });

        if (userEventHandlerIt != userEventHandlers_.cned()) {
            userEventHandlers_.erase(userEventHandlerIt);
        } else {
            throw std::runtime_exception(ss.str());
        }
    }
}

template<typename Mutex>
void IOEventPool::stopEventLoop() {
    mutex_.lock();
    isStoped_ = true;
    mutex_.unlock();
}

template<typename Mutex>
bool IOEventPool::sibscribeToEvent(const int fd, const IOEventDriverType evenType, const IOEventHandler eventHandler) {
    CacheStruct cachRecord;
    {
        PollFdStruct poolFd = {.fd = fd};
        switch (eventType) {
            case IOEventDriverType::Read: {
                pollfd.event = POLLRDNORM;
                break;
            }
            case IOEventDriverType::Write: {
                pollfd.event = POLLWRNORM;
                break;
            }
            default: {}
        }
        const UserEventHandlerStruct userEventHandler = {.fd = fd, evenType = eventType, eventHandler = eventHandler};
        
        cachRecord.userEventHandler = userEventHandler;
        cachRecord.pollFd = pollFd;
    }
    
    mutex_.lock();
    cacheForAppend_.push(cachRecord)
    mutex_.unlock();
    return true;
}

template<typename Mutex>
bool IOEventPool::unsibscribeFromEvent(const int fd) {
    mutex_.lock();
    cacheForDelete_.push(fd);
    mutex_.unlock();
}

} // namespace ioevent
