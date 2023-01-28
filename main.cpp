#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <stdexcept>

#include <fcntl.h>
#include <unistd.h>

#include "ioevent_driver/ioevent_driver.h"
#include "ioevent_driver/ioevent_poll.h"

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

    auto ioEventDriver = std::unique_ptr<ioevent::IOEventDriver>(
        new ioevent::SingleThreadIOEventDriver<ioevent::LockFreeMute>());

    const auto fileName(argv[1]);
    const auto fdOnWrite = OpenFile(fileName, O_WRONLY);
    const auto fdOnRead = OpenFile(fileName, O_RDONLY);

    ioEventDriver->subscribeToEvent(fdOnWrite, ioevent::IOEventDriverType::Write, [](const int fd) {
        std::string msg("Hello worl from async callback!");
        write(fd, msg.data(), msg.size());
        std::cout << "I async write this messge: " << msg << std::endl;
    });

    ioEventDriver->subscribeToEvent(fdOnRead, ioevent::IOEventDriverType::Read, [](const int fd) {
        char buff[256] = {0};
        read(fd, buff, 256);

        std::cout << "I async read from file this message: " << buff << std::endl;
    });

    ioEventDriver->runEventLoop();
    return 0;
}