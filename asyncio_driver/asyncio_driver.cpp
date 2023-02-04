#include "asyncio_driver.h"
#include <iostream>

namespace asyncio {

AsyncIODriver::AsyncIODriver(IOEventDriverPtr&& ioEventDriver, ThreadPoolPtr&& threadPool):
iOEventDriver_(std::move(ioEventDriver)),
threadPool_(std::move(threadPool)) 
{}

void AsyncIODriver::run() {
    threadPool_->run();
    threadPool_->spawnTask([ioEventDriver = iOEventDriver_.get()]() {
        ioEventDriver->runEventLoop();
    });
}

void AsyncIODriver::stop() {
    std::cout << "Stop io event driver" << std::endl;
    iOEventDriver_->stopEventLoop();
    std::cout << "Stop thread pool" << std::endl;
    threadPool_->join();
}

} // namespace asyncio
