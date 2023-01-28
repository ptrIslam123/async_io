#include "asyncio_driver.h"

namespace asyncio {

AsyncIODriver::AsyncIODriver(IOEventDriverPtr&& ioEventDriver, ThreadPoolPtr&& threadPool):
iOEventDriver_(std::move(ioEventDriver)),
threadPool_(std::move(threadPool)) 
{}

void AsyncIODriver::run() {
    threadPool_->spawnTask([ioEventDriver = iOEventDriver_.get()]() {
        ioEventDriver->runEventLoop();
    });
}

void AsyncIODriver::stop() {
    threadPool_->join();
}

} // namespace asyncio