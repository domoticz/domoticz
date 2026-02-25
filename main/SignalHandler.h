#pragma once
#include <atomic>

extern bool g_stop_watchdog;
extern std::atomic<bool> g_bReopenLogFile;

void signal_handler(int sig_num
#ifndef WIN32
, siginfo_t * info, void * ucontext
#endif
);

void Do_Watchdog_Work();
