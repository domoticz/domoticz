#pragma once

extern bool g_stop_watchdog;

void signal_handler(int sig_num
#ifndef WIN32
, siginfo_t * info, void * ucontext
#endif
);

void Do_Watchdog_Work();
