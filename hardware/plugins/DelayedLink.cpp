#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "DelayedLink.h"

namespace Plugins {
    SharedLibraryProxy* pythonLib = new SharedLibraryProxy();
} // namespace Plugins
#endif
