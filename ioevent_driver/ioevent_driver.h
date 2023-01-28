#pragma once

#include <function>

namespace ioevent {

enum class IOEventDriverType {
    Read,
    Write
};

struct IOEventDriver {
    using IOEventHandler = std::function<void(int fd)>;

    virtual bool sibscribeToEvent(int fd, IOEventDriverType evenType, IOEventHandler eventHandler) override;
    virtual bool unsibscribeFromEvent(int fd) override;
    virtual void runEventLoop() override;
    virtual void stopEventLoop() override;
    virtual ~IOEventDriver() = default;
};

} // namespace ioevent