#include "stdafx.h"
#if defined WIN32

#include "WindowsService.h"
#include "Logger.h"
#include <atomic>
#include <windows.h>

extern std::atomic<bool> g_bStopApplication;

namespace WindowsService
{
	const char *SERVICE_NAME = "Domoticz";
	const char *SERVICE_DISPLAY_NAME = "Domoticz";
	static const char *SERVICE_DESCRIPTION = "Domoticz Home Automation System";

	// Services that Domoticz needs before it can do anything useful. Same list
	// the installer used to hand to the wrapper.
	static const char *SERVICE_DEPENDENCIES = "RpcSS\0LanmanWorkstation\0";

	static SERVICE_STATUS m_ServiceStatus = {};
	static SERVICE_STATUS_HANDLE m_hStatus = nullptr;
	static int (*m_pfnRun)() = nullptr;
	static int m_iExitCode = 0;
	static bool m_bIsService = false;
	static DWORD m_dwCheckPoint = 1;

	// ----------------------------------------------------------------------
	// Reporting to the Service Control Manager
	// ----------------------------------------------------------------------
	static void ReportStatus(DWORD dwState, DWORD dwExitCode, DWORD dwWaitHint)
	{
		if (m_hStatus == nullptr)
			return;

		m_ServiceStatus.dwCurrentState = dwState;
		m_ServiceStatus.dwWin32ExitCode = dwExitCode;
		m_ServiceStatus.dwWaitHint = dwWaitHint;

		// Stop is the only control we accept, and only once we are running.
		m_ServiceStatus.dwControlsAccepted = (dwState == SERVICE_START_PENDING)
			? 0
			: SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

		if (dwState == SERVICE_RUNNING || dwState == SERVICE_STOPPED)
			m_ServiceStatus.dwCheckPoint = 0;
		else
			m_ServiceStatus.dwCheckPoint = m_dwCheckPoint++;

		SetServiceStatus(m_hStatus, &m_ServiceStatus);
	}

	void ReportStopProgress()
	{
		if (m_bIsService)
			ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 30000);
	}

	bool IsRunningAsService()
	{
		return m_bIsService;
	}

	static std::string GetOwnPath()
	{
		char szPath[MAX_PATH] = {};
		if (GetModuleFileNameA(nullptr, szPath, MAX_PATH) == 0)
			return "";
		return szPath;
	}

	// A service starts in %SystemRoot%\System32, not next to its executable.
	// Domoticz finds its own folder through GetModuleFileName so it does not
	// depend on this, but anything relative on the command line would resolve
	// somewhere unexpected. The wrapper used to set this too.
	static void SetWorkingDirectoryToOwnFolder()
	{
		std::string szPath = GetOwnPath();
		size_t iSlash = szPath.find_last_of('\\');
		if (iSlash == std::string::npos)
			return;
		std::string szFolder = szPath.substr(0, iSlash);
		if (!SetCurrentDirectoryA(szFolder.c_str()))
			_log.Log(LOG_ERROR, "Service: could not switch to '%s' (error %lu)", szFolder.c_str(), GetLastError());
	}

	// ----------------------------------------------------------------------
	// Callbacks for the Service Control Manager
	// ----------------------------------------------------------------------
	static void WINAPI ServiceControlHandler(DWORD dwControl)
	{
		switch (dwControl)
		{
			case SERVICE_CONTROL_STOP:
			case SERVICE_CONTROL_SHUTDOWN:
				// Ask for room: stopping the workers is not instant.
				ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 30000);
				g_bStopApplication = true;
				break;

			case SERVICE_CONTROL_INTERROGATE:
				ReportStatus(m_ServiceStatus.dwCurrentState, NO_ERROR, 0);
				break;

			default:
				break;
		}
	}

	static void WINAPI ServiceMain(DWORD /*argc*/, LPSTR * /*argv*/)
	{
		m_hStatus = RegisterServiceCtrlHandlerA(SERVICE_NAME, ServiceControlHandler);
		if (m_hStatus == nullptr)
		{
			_log.Log(LOG_ERROR, "Service: could not register the control handler (error %lu)", GetLastError());
			return;
		}

		m_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		m_ServiceStatus.dwServiceSpecificExitCode = 0;

		// Starting Domoticz means opening the database and every configured
		// piece of hardware, so ask for a generous window.
		ReportStatus(SERVICE_START_PENDING, NO_ERROR, 60000);

		SetWorkingDirectoryToOwnFolder();

		ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

		m_iExitCode = m_pfnRun();

		ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}

	bool RunAsService(int (*pfnRun)())
	{
		m_pfnRun = pfnRun;
		m_bIsService = true;

		SERVICE_TABLE_ENTRYA serviceTable[] = {
			{ const_cast<LPSTR>(SERVICE_NAME), ServiceMain },
			{ nullptr, nullptr }
		};

		if (StartServiceCtrlDispatcherA(serviceTable))
			return true;

		// Started by hand rather than by the SCM: let the caller run normally.
		m_bIsService = false;
		if (GetLastError() != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
			_log.Log(LOG_ERROR, "Service: could not connect to the service controller (error %lu)", GetLastError());
		return false;
	}

	// ----------------------------------------------------------------------
	// Installing and removing
	// ----------------------------------------------------------------------
	int Install(const std::string &szArguments)
	{
		std::string szPath = GetOwnPath();
		if (szPath.empty())
		{
			_log.Log(LOG_ERROR, "Service: could not determine our own location (error %lu)", GetLastError());
			return 1;
		}

		// The path is quoted because it usually contains spaces; -service tells
		// the copy the SCM starts to hand control to ServiceMain.
		std::string szCommand = "\"" + szPath + "\" -service";
		if (!szArguments.empty())
			szCommand += " " + szArguments;

		// Logged before anything can fail, so a failed install still shows what
		// it was going to register.
		_log.Log(LOG_STATUS, "Service: registering '%s' as: %s", SERVICE_NAME, szCommand.c_str());

		SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
		if (hManager == nullptr)
		{
			_log.Log(LOG_ERROR, "Service: could not open the service manager (error %lu). Administrator rights are required.", GetLastError());
			return 1;
		}

		SC_HANDLE hService = CreateServiceA(
			hManager,
			SERVICE_NAME,
			SERVICE_DISPLAY_NAME,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_NORMAL,
			szCommand.c_str(),
			nullptr,                    // no load ordering group
			nullptr,                    // no tag identifier
			SERVICE_DEPENDENCIES,
			nullptr,                    // LocalSystem
			nullptr);                   // no password

		if (hService == nullptr)
		{
			DWORD dwError = GetLastError();
			CloseServiceHandle(hManager);
			if (dwError == ERROR_SERVICE_EXISTS)
			{
				_log.Log(LOG_ERROR, "Service: '%s' already exists. Remove it first with -uninstallservice.", SERVICE_NAME);
				return 1;
			}
			_log.Log(LOG_ERROR, "Service: could not create '%s' (error %lu)", SERVICE_NAME, dwError);
			return 1;
		}

		SERVICE_DESCRIPTIONA description = {};
		description.lpDescription = const_cast<LPSTR>(SERVICE_DESCRIPTION);
		ChangeServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, &description);

		// Restart twice on an unexpected exit, then leave it alone. This is
		// what the wrapper used to provide.
		SC_ACTION actions[3] = {};
		actions[0].Type = SC_ACTION_RESTART;
		actions[0].Delay = 60000;
		actions[1].Type = SC_ACTION_RESTART;
		actions[1].Delay = 60000;
		actions[2].Type = SC_ACTION_NONE;
		actions[2].Delay = 0;

		SERVICE_FAILURE_ACTIONSA failureActions = {};
		failureActions.dwResetPeriod = 86400;   // forget failures after a day
		failureActions.cActions = 3;
		failureActions.lpsaActions = actions;
		ChangeServiceConfig2A(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions);

		_log.Log(LOG_STATUS, "Service: '%s' installed", SERVICE_NAME);

		if (!StartServiceA(hService, 0, nullptr))
			_log.Log(LOG_ERROR, "Service: installed, but could not be started (error %lu)", GetLastError());
		else
			_log.Log(LOG_STATUS, "Service: '%s' started", SERVICE_NAME);

		CloseServiceHandle(hService);
		CloseServiceHandle(hManager);
		return 0;
	}

	int Uninstall()
	{
		SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (hManager == nullptr)
		{
			_log.Log(LOG_ERROR, "Service: could not open the service manager (error %lu). Administrator rights are required.", GetLastError());
			return 1;
		}

		SC_HANDLE hService = OpenServiceA(hManager, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
		if (hService == nullptr)
		{
			DWORD dwError = GetLastError();
			CloseServiceHandle(hManager);
			if (dwError == ERROR_SERVICE_DOES_NOT_EXIST)
			{
				// Nothing to do, which is a success as far as an uninstaller
				// is concerned.
				return 0;
			}
			_log.Log(LOG_ERROR, "Service: could not open '%s' (error %lu)", SERVICE_NAME, dwError);
			return 1;
		}

		SERVICE_STATUS status = {};
		if (ControlService(hService, SERVICE_CONTROL_STOP, &status))
		{
			// Give it a minute to wind down before pulling it out.
			for (int i = 0; i < 60; i++)
			{
				Sleep(1000);
				if (!QueryServiceStatus(hService, &status))
					break;
				if (status.dwCurrentState == SERVICE_STOPPED)
					break;
			}
			if (status.dwCurrentState != SERVICE_STOPPED)
				_log.Log(LOG_ERROR, "Service: '%s' did not stop in time, removing it anyway", SERVICE_NAME);
		}

		int iResult = 0;
		if (!DeleteService(hService))
		{
			DWORD dwError = GetLastError();
			if (dwError != ERROR_SERVICE_MARKED_FOR_DELETE)
			{
				_log.Log(LOG_ERROR, "Service: could not remove '%s' (error %lu)", SERVICE_NAME, dwError);
				iResult = 1;
			}
		}
		else
			_log.Log(LOG_STATUS, "Service: '%s' removed", SERVICE_NAME);

		CloseServiceHandle(hService);
		CloseServiceHandle(hManager);
		return iResult;
	}

} // namespace WindowsService

#endif // WIN32
