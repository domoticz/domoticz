#include "../../main/stdafx.h"
#include "../../hardware/ASyncSerial.h"
#include "../../main/Logger.h"
#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <poll.h>
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
std::atomic_bool verifyPreReturnRead{false};
std::mutex preReturnMutex;
std::condition_variable preReturnCondition;
bool preReturnRead = false;
int preReturnMaster = -1;
std::function<void()> beforeThreadFailure;
struct TestSerial : AsyncSerial {
    using AsyncSerial::setReadCallback;
};
extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                             void* (*start)(void*), void* arg) {
    if (failThread.exchange(false)) {
        if (beforeThreadFailure) beforeThreadFailure();
        return EAGAIN;
    }
    using Function = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
    static auto real = reinterpret_cast<Function>(dlsym(RTLD_NEXT, "pthread_create"));
    int result = real(thread, attr, start, arg);
    // Hold the opener until the real serial worker has delivered input.
    // A delay alone cannot establish this ordering on a busy runner.
    if (result == 0 && verifyPreReturnRead.exchange(false)) {
        assert(::write(preReturnMaster, "r", 1) == 1);
        std::unique_lock<std::mutex> lock(preReturnMutex);
        assert(preReturnCondition.wait_for(lock, std::chrono::seconds(2),
                                          [] { return preReturnRead; }));
    }
    return result;
}

int main(int argc, char** argv) {
    for (bool onlyBaud : {false, true}) {
        if (argc > 1 && onlyBaud != (std::string(argv[1]) == "baud")) continue;
        // Exercise a successful open with the worker running before return.
        int raceMaster, raceSlave;
        char raceName[128];
        assert(openpty(&raceMaster, &raceSlave, raceName, nullptr, nullptr) == 0);
        TestSerial raceSerial;
        std::atomic_size_t raceReceived{0};
        raceSerial.setReadCallback([&](const char* data, size_t size) {
            assert(size == 1 && data[0] == 'r');
            raceReceived += size;
            {
                std::lock_guard<std::mutex> lock(preReturnMutex);
                preReturnRead = true;
            }
            preReturnCondition.notify_one();
        });
        preReturnRead = false;
        preReturnMaster = raceMaster;
        verifyPreReturnRead = true;
        if (onlyBaud) raceSerial.openOnlyBaud(raceName, 9600);
        else raceSerial.open(raceName, 9600);
        assert(raceSerial.isOpen());
        assert(raceReceived == 1); // Callback completed before open returned.
        raceSerial.close();
        ::close(raceMaster);
        ::close(raceSlave);

        int master, slave;
        char name[128];
        assert(openpty(&master, &slave, name, nullptr, nullptr) == 0);
        TestSerial serial;
        auto open = [&] {
            if (onlyBaud) serial.openOnlyBaud(name, 9600);
            else serial.open(name, 9600);
        };
        // Reproduce a public write accepted while isOpen() is already true,
        // before worker creation reports failure. The helper is started first
        // so its own pthread_create is not intercepted as the injected failure.
        std::mutex writeMutex;
        std::condition_variable writeCondition;
        bool writeRequested = false, writeQueued = false;
        std::thread writer([&] {
            std::unique_lock<std::mutex> lock(writeMutex);
            assert(writeCondition.wait_for(lock, std::chrono::seconds(2),
                                           [&] { return writeRequested; }));
            assert(serial.isOpen());
            serial.writeString("a");
            writeQueued = true;
            writeCondition.notify_one();
        });
        beforeThreadFailure = [&] {
            std::unique_lock<std::mutex> lock(writeMutex);
            writeRequested = true;
            writeCondition.notify_one();
            assert(writeCondition.wait_for(lock, std::chrono::seconds(2),
                                           [&] { return writeQueued; }));
        };
        failThread = true;
        bool threw = false;
        try { open(); } catch (...) { threw = true; }
        writer.join();
        beforeThreadFailure = {};
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
        // Preserve the queue policy: the next requested write drains queued
        // bytes, without inventing automatic retry or dropping accepted bytes.
        serial.writeString("b");
        std::string output;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (output.size() < 2 && std::chrono::steady_clock::now() < deadline) {
            pollfd fd{master, POLLIN, 0};
            if (::poll(&fd, 1, 20) > 0) {
                char bytes[2];
                const auto count = ::read(master, bytes, 2 - output.size());
                assert(count > 0);
                output.append(bytes, static_cast<size_t>(count));
            }
        }
        assert(output == "ab");
        assert(write(master, "x", 1) == 1);
        for (int attempt = 0; received == 0 && attempt < 200; ++attempt)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        assert(received == 1);
        serial.close();
        assert(!serial.isOpen());
        ::close(master);
        ::close(slave);
    }
    puts("Both pre-return worker scheduling and startup rollback/reopen paths passed");
}
