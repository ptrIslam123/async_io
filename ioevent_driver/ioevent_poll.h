#pragma once

#include "ioevent_driver.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <mutex>

#include <poll.h>

#include <iostream>

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
    bool subscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler&& eventHandler) override;
    bool unsubscribeFromEvent(int fd) override;
    void runEventLoop() override;
    void stopEventLoop() override;

private:
    void handleNewEvents(int eventCount);
    void updateCachesForAppend();
    void updateCachesForDelete();
    void updateCaches();
    bool removeUserDescriptorHandlerByIndex(int index);

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
        const auto timeout = 100;
        const auto res = poll(fdSet_.data(), fdSet_.size(), timeout);
        if (res < 0) {
            throw std::runtime_error("IOEventPool::runEventLoop: sys call pool was failed");
        }

        std::cout << "go out from poll call with resul=" << res << " and fdset size=" << fdSet_.size() << std::endl;
        handleNewEvents(res);
        mutexForEventHandlers_.unlock();

        updateCaches();
    }
}

template<typename Mutex>
void IOEventPool<Mutex>::handleNewEvents(int eventCount) {
    for (auto i = 0; i < fdSet_.size() && eventCount > 0; ++i) {
        auto& curFd = fdSet_[i];
        auto& curEventHandler = userEventHandlers_[i];
        std::cout << "Cur iter index=" << i << " and fd=" << curFd.fd << " and handler addr=" << &curEventHandler.eventHandler << std::endl;
        if (curFd.revents == curFd.events && curFd.fd >= 0 && curEventHandler.eventHandler) {
            /*
             * We need check descriptor state because sys call 'poll' can be to one thread and call method 'unsubscribeFromEvent(int fd)' to other thread.
             * Then user want unsubscribe him descriptor from notifications he can use 'unsubscribeFromEvent'. But this call put code (for erase this descriptor
             * from traced) to cache for deleting events and this cache can be don`t update long time and async io dirver will think that this descriptor need notify
             * about next events.
             */
            if (curEventHandler.eventHandler(curFd.fd) == ioevent::DescriptorState::Closed) {
                std::cout << "clear fd=" << curFd.fd << std::endl;
                curEventHandler.eventHandler = {};
                curFd.fd = -1;
            }
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
bool IOEventPool<Mutex>::removeUserDescriptorHandlerByIndex(const int index) {
    {
        auto it = userEventHandlers_.begin();
        std::advance(it, index);

        if (it != userEventHandlers_.end()) {
            userEventHandlers_.erase(it);
        } else {
            return false;
        }
    }

    {
        auto it = fdSet_.begin();
        std::advance(it, index);

        if (it != fdSet_.end()) {
            fdSet_.erase(it);
        } else {
            return false;
        }
    }

    return true;
}

template<typename Mutex>
void IOEventPool<Mutex>::updateCachesForDelete() {
    mutexCachForDeleter_.lock();
    mutexForEventHandlers_.lock();

    while (!cacheForDelete_.empty()) {
        auto fd = cacheForDelete_.front();
        cacheForDelete_.pop();

        std::stringstream ss;
        ss << "IOEventPool::unsubscribeFromEvent: Can`t remove fd: " << fd << "becaus it doesn`t exist";

        auto index = -1;
        for (auto i = 0; i < userEventHandlers_.size(); ++i) {
            if (userEventHandlers_[i].fd == fd) {
                index = i;
                break;
            }
        }

        if ((index >= 0) && removeUserDescriptorHandlerByIndex(index)) {
            // do nothing
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
    updateCaches();
}

template<typename Mutex>
bool IOEventPool<Mutex>::subscribeToEvent(const int fd, const IOEventDriverType eventType, IOEventHandler&& eventHandler) {
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
        
        cachRecord.userEventHandler = std::move(userEventHandler);
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
