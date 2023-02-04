#pragma once

#include <functional>

namespace ioevent {

enum class IOEventDriverType {
    Read,
    Write
};

enum class DescriptorState {
    Open,
    Closed
};

struct IOEventDriver {
    using IOEventHandler = std::function<DescriptorState(int fd)>;

    virtual bool subscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler&& eventHandler) = 0;
    virtual bool unsubscribeFromEvent(int fd) = 0;
    virtual void runEventLoop() = 0;
    virtual void stopEventLoop() = 0;
    virtual ~IOEventDriver() = default;
};

} // namespace ioevent
