#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "../main/Helper.h"
#include "DelayedLink.h"

namespace Plugins {
    SharedLibraryProxy* pythonLib = new SharedLibraryProxy();
}
#endif
