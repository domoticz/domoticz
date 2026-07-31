#pragma once
#if defined WIN32

#include <string>

// Running Domoticz as a Windows service.
//
// Windows will not run an ordinary program as a service: a service has to
// connect to the Service Control Manager, report its status and respond to
// stop requests. That is what this module adds, and it is why the installer
// no longer needs a wrapper such as NSSM.
//
// Three command line switches drive it:
//
//   -installservice [arguments]  register the service, then start it
//   -uninstallservice            stop and remove the service
//   -service                     run as a service; used by the SCM, not by hand
//
// Anything after -installservice is stored as the arguments the service will
// be started with, so the installer can pass on the user's choices.

namespace WindowsService
{
	// Names as they appear in services.msc and in sc.exe.
	extern const char *SERVICE_NAME;
	extern const char *SERVICE_DISPLAY_NAME;

	// Registers the service to start automatically and starts it right away.
	// szArguments is passed on to Domoticz when the service starts.
	// Returns a process exit code: 0 on success.
	int Install(const std::string &szArguments);

	// Stops the service if it is running and removes it. Returns 0 when the
	// service is gone afterwards, whether or not it existed to begin with.
	int Uninstall();

	// Hands control to the Service Control Manager. Blocks until the service
	// stops. Returns false when this process was not started by the SCM, in
	// which case the caller should carry on as a normal application.
	//
	// pfnRun is the body of the application; it must return when
	// g_bStopApplication becomes true.
	bool RunAsService(int (*pfnRun)());

	// True while this process is running under the Service Control Manager.
	// The tray icon, the message loop and any dialog are pointless in that
	// case: a service has no desktop to show them on.
	bool IsRunningAsService();

	// Tells the SCM we are still busy shutting down. Windows kills a service
	// that stays silent for too long, and stopping Domoticz can take a while.
	void ReportStopProgress();
}

#endif // WIN32
