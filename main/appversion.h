#pragma once
#include "../appversion.h"

//#define BUILD_MASTER

#define VERSION_MAJOR               2023
#define VERSION_MINOR               2
#define VERSION_REVISION            0
#define VERSION_BUILD               APPVERSION

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#ifndef BUILD_MASTER
#if (VERSION_REVISION < 1)
#define VERSION_STRING  STRINGIZE(VERSION_MAJOR)        \
                        "." STRINGIZE(VERSION_MINOR)    \
                        " (build " STRINGIZE(VERSION_BUILD) ")"
#else
#define VERSION_STRING  STRINGIZE(VERSION_MAJOR)        \
                        "." STRINGIZE(VERSION_MINOR)    \
                        "." STRINGIZE(VERSION_REVISION) \
                        " (build " STRINGIZE(VERSION_BUILD) ")"
#endif
#else
#if (VERSION_REVISION < 1)
#define VERSION_STRING  STRINGIZE(VERSION_MAJOR)        \
                        "." STRINGIZE(VERSION_MINOR)
#else
#define VERSION_STRING  STRINGIZE(VERSION_MAJOR)        \
                        "." STRINGIZE(VERSION_MINOR)    \
                        "." STRINGIZE(VERSION_REVISION)
#endif
#endif

