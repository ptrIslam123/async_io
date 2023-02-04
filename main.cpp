#include <iostream>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cassert>

#include <fcntl.h>
#include <unistd.h>

#include "ioevent_driver/ioevent_driver.h"
#include "ioevent_driver/ioevent_poll.h"
#include "asyncio_driver/asyncio_driver.h"

#include <poll.h>

int OpenFile(const std::string& fileName, const int mode) {
    const auto res = open(fileName.c_str(), mode);
    if (res < 0) {
        throw std::runtime_error(std::string("Can`t open file: ") + fileName);
    }
    return res;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Pass wrong args: <file-name>" << std::endl;
        return -1;
    }
    
    const auto fileName(argv[1]);
    const auto fdOnWrite = OpenFile(fileName, O_WRONLY);
    const auto fdOnRead = OpenFile(fileName, O_RDONLY);

    auto ioEventDriver = std::unique_ptr<ioevent::IOEventDriver>(
        new ioevent::SingleThreadIOEventDriver<ioevent::LockFreeMute>()
    );

    auto threadPool = std::make_unique<thread::StaticThreadPool>(4);

    asyncio::AsyncIODriver asyncIODriver(std::move(ioEventDriver), std::move(threadPool));

    std::cout << "Run async io driver" << std::endl;
    asyncIODriver.run();

    auto asyncWriteRes = asyncIODriver.write<int>(fdOnWrite, [](const int fd) {
        const std::string msg("Hello world!");
        const auto writeSize = write(fd, msg.data(), msg.size());
        std::cout << "async write size: " << writeSize << std::endl;
        return writeSize;
    });

    auto asyncReadRes = asyncIODriver.read<std::string>(fdOnRead, [](const int fd) {
        char buff[256] = {0};
        const auto readSize = read(fd, buff, 256);
        std::cout << "async read size: " << readSize << std::endl;
        return std::string(buff);
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "After sleep" << std::endl;


    std::cout << "Wait async results" << std::endl;
    std::cout << "write future value=" << asyncWriteRes.get() << std::endl;
    std::cout << "read future value=" << asyncReadRes.get() << std::endl;

    asyncIODriver.stop();
    std::cout << "Exit main thread" << std::endl;
    return 0;
}
