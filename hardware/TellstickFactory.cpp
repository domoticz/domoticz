#include "stdafx.h"
#include "Tellstick.h"
#include "../main/Logger.h"

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <unistd.h> // for _POSIX_VERSION
#endif

#ifdef WITH_TELLDUSCORE
#include <telldus-core.h>

bool CTellstick::Create(CTellstick** tellstick, const int ID, int repeats, int repeatInterval)
{
    if (tellstick) {
        TelldusFunctions td;
        td.Init = tdInit;
        td.RegisterDeviceEvent = tdRegisterDeviceEvent;
        td.RegisterRawDeviceEvent = tdRegisterRawDeviceEvent;
        td.RegisterSensorEvent = tdRegisterSensorEvent;
        td.UnregisterCallback = tdUnregisterCallback;
        td.Close = tdClose;
        td.ReleaseString = tdReleaseString;
        td.TurnOn = tdTurnOn;
        td.TurnOff = tdTurnOff;
        td.Dim = tdDim;
        td.Methods = tdMethods;
        td.GetNumberOfDevices = tdGetNumberOfDevices;
        td.GetDeviceId = tdGetDeviceId;
        td.GetName = tdGetName;
        CTellstick* obj = new CTellstick(td, ID, repeats, repeatInterval);
        if (obj) {
            *tellstick = obj;
            return true;
        }
    }
    return false;
}

#elif _POSIX_VERSION >= 200112L // dynamic runtime loading
#include <dlfcn.h>

namespace {

bool g_symbolsLoaded = false;
TelldusFunctions g_functions;

template<typename Function>
bool loadSymbol(void* handle, Function& function, const char * name)
{
    dlerror(); // clear previous error
    *(void **)(&function) = dlsym(handle, name);
    const char *error = dlerror();
    if (error) {
        _log.Log(LOG_ERROR, "Tellstick: failed to load symbol \"%s\": %s", name, error);
    }
    return !error;
}

bool loadSymbols(void* handle, TelldusFunctions& td)
{
    return loadSymbol(handle, td.Init, "tdInit")
        && loadSymbol(handle, td.RegisterDeviceEvent, "tdRegisterDeviceEvent")
        && loadSymbol(handle, td.RegisterRawDeviceEvent, "tdRegisterRawDeviceEvent")
        && loadSymbol(handle, td.RegisterSensorEvent, "tdRegisterSensorEvent")
        && loadSymbol(handle, td.UnregisterCallback, "tdUnregisterCallback")
        && loadSymbol(handle, td.Close, "tdClose")
        && loadSymbol(handle, td.ReleaseString, "tdReleaseString")
        && loadSymbol(handle, td.TurnOn, "tdTurnOn")
        && loadSymbol(handle, td.TurnOff, "tdTurnOff")
        && loadSymbol(handle, td.Dim, "tdDim")
        && loadSymbol(handle, td.Methods, "tdMethods")
        && loadSymbol(handle, td.GetNumberOfDevices, "tdGetNumberOfDevices")
        && loadSymbol(handle, td.GetDeviceId, "tdGetDeviceId")
        && loadSymbol(handle, td.GetName, "tdGetName");
}

} // anonymous namespace

bool CTellstick::Create(CTellstick** tellstick, const int ID, int repeats, int repeatInterval)
{
    if (!g_symbolsLoaded) {
        void* handle = dlopen("libtelldus-core.so", RTLD_LAZY);
        if (handle) {
            if (loadSymbols(handle, g_functions)) {
                g_symbolsLoaded = true;
                _log.Log(LOG_NORM, "Tellstick: dynamically loaded telldus-core");
            }
            else {
                dlclose(handle);
            }
        }
        else {
            _log.Log(LOG_STATUS, "Tellstick: telldus-core not found");
        }
    }

    if (g_symbolsLoaded) {
        CTellstick* obj = new CTellstick(g_functions, ID, repeats, repeatInterval);
        if (obj) {
            *tellstick = obj;
            return true;
        }
    }
    return false;
}

#else // !WITH_TELLDUSCORE and no dlopen()/dlsym()

bool CTellstick::Create(CTellstick** tellstick, const int ID, int repeats, int repeatInterval)
{
    _log.Log(LOG_STATUS, "Tellstick: support not enabled");
    return false;
}

#endif // WITH_TELLDUSCORE
