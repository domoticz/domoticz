

#ifdef LIBFTD2XX
	#ifdef _WINDOWS
		#include <windows.h>
		#include "win\ftd2xx.h"
	#else
		#include "osx/WinTypes.h"
		#include "osx/ftd2xx.h"
	#endif
#endif

#ifdef LIBFTDI
	#include <ftdi.h>
#endif
