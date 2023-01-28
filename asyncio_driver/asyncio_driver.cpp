#include "asyncio_driver.h"

namespace asyncio {

AsyncIODriver::AsyncIODriver(IOEventDriverPtr&& ioEventDriver, ThreadPoolPtr&& threadPool):
iOEventDriver_(std::move(ioEventDriver)),
threadPool_(std::move(threadPool)) 
{}

void AsyncIODriver::run() {

}

void AsyncIODriver::stop() {

}

} // namespace asyncio