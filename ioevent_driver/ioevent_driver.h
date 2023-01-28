#pragma once

#include <functional>

namespace ioevent {

enum class IOEventDriverType {
    Read,
    Write
};

struct IOEventDriver {
    using IOEventHandler = std::function<void(int fd)>;

    virtual bool subscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler eventHandler) = 0;
    virtual bool unsubscribeFromEvent(int fd) = 0;
    virtual void runEventLoop() = 0;
    virtual void stopEventLoop() = 0;
    virtual ~IOEventDriver() = default;
};

} // namespace ioevent