#include "../../main/stdafx.h"
#include "../../hardware/ASyncSerial.h"
#include "../../main/Logger.h"
#include <atomic>
#include <cassert>
#include <cerrno>
#include <dlfcn.h>
#include <pty.h>
#include <pthread.h>
#include <unistd.h>

// Only application logging and the sleep helper are stubbed. Serial I/O,
// Boost startup, and AsyncSerial itself are the production implementation.
CLogger::CLogger() {}
CLogger::~CLogger() {}
void CLogger::Log(_eLogLevel, const char*, ...) {}
CLogger _log;
void sleep_milliseconds(long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::atomic_bool failThread{false};
struct TestSerial : AsyncSerial {
    using AsyncSerial::setReadCallback;
};
extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                             void* (*start)(void*), void* arg) {
    if (failThread.exchange(false)) return EAGAIN;
    using Function = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
    static auto real = reinterpret_cast<Function>(dlsym(RTLD_NEXT, "pthread_create"));
    return real(thread, attr, start, arg);
}

int main() {
    for (bool onlyBaud : {false, true}) {
        int master, slave;
        char name[128];
        assert(openpty(&master, &slave, name, nullptr, nullptr) == 0);
        TestSerial serial;
        auto open = [&] {
            if (onlyBaud) serial.openOnlyBaud(name, 9600);
            else serial.open(name, 9600);
        };
        failThread = true;
        bool threw = false;
        try { open(); } catch (...) { threw = true; }
        assert(threw);
        assert(!serial.isOpen());
        assert(serial.errorStatus());

        std::atomic_size_t received{0};
        serial.setReadCallback([&](const char* data, size_t size) {
            assert(size == 1 && data[0] == 'x');
            received += size;
        });
        open();
        assert(serial.isOpen());
        assert(!serial.errorStatus());
        assert(write(master, "x", 1) == 1);
        for (int attempt = 0; received == 0 && attempt < 200; ++attempt)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        assert(received == 1);
        serial.close();
        assert(!serial.isOpen());
        ::close(master);
        ::close(slave);
    }
    puts("Both startup-failure rollback and PTY reopen/read paths passed");
}
