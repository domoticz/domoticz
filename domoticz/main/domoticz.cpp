// domoticz.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mainworker.h"

#include <stdio.h>     /* standard I/O functions                         */
#include <sys/types.h> /* various type definitions, like pid_t           */
#include <signal.h>    /* signal name macros, and the signal() prototype */ 
#include <iostream>
#include "CmdLine.h"
#include "Logger.h"
#include "appversion.h"

#if defined WIN32
	#include "WindowsHelper.h"
	#include <Shlobj.h>
#endif

const char *szHelp=
	"Usage: Domoticz -www port -verbose x\n"
	"\t-www port (for example -www 8080)\n"
#if defined WIN32
	"\t-dbase file_path (for example D:\\domoticz.db)\n"
#else
	"\t-dbase file_path (for example /opt/domoticz/domoticz.db)\n"
#endif
	"\t-verbose x (where x=0 is none, x=1 is debug)\n"
	"\t-startupdelay seconds (default=0)\n"
	"\t-nowwwpwd (in case you forgot the web server username/password)\n"
#if defined WIN32
	"\t-nobrowser (do not start web browser (Windows Only)\n"
#endif
#if defined WIN32
	"\t-logfile file_path (for example D:\\domoticz.log)\n"
#else
	"\t-logfile file_path (for example /var/log/domoticz.log)\n"
#endif
	"";

std::string szStartupFolder;
bool bIsRaspberryPi=false;
bool bHave1Wire=false;

MainWorker _mainworker;
CLogger _log;

void DQuitFunction()
{
	_log.Log(LOG_NORM,"Closing application!...");
	fflush(stdout);
#if defined WIN32
	TrayMessage(NIM_DELETE,NULL);
#endif
	_log.Log(LOG_NORM,"stopping worker...");
	_mainworker.Stop();
}

void catch_intterm(int sig_num)
{
	DQuitFunction();
	exit(EXIT_SUCCESS); 
}

#if defined(_WIN32)
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		return GetModuleFileNameA(NULL, pathName, (DWORD)pathNameCapacity);
	}
#elif defined(__linux__) /* elif of: #if defined(_WIN32) */
#include <unistd.h>
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		size_t pathNameSize = readlink("/proc/self/exe", pathName, pathNameCapacity - 1);
		pathName[pathNameSize] = '\0';
		return pathNameSize;
	}
#elif defined(__APPLE__) /* elif of: #elif defined(__linux__) */
	#include <mach-o/dyld.h>
	static size_t getExecutablePathName(char* pathName, size_t pathNameCapacity)
	{
		uint32_t pathNameSize = 0;

		_NSGetExecutablePath(NULL, &pathNameSize);

		if (pathNameSize > pathNameCapacity)
			pathNameSize = pathNameCapacity;

		if (!_NSGetExecutablePath(pathName, &pathNameSize))
		{
			char real[PATH_MAX];

			if (realpath(pathName, real) != NULL)
			{
				pathNameSize = strlen(real);
				strncpy(pathName, real, pathNameSize);
			}

			return pathNameSize;
		}

		return 0;
	}
#else /* else of: #elif defined(__APPLE__) */
	#error provide your own getExecutablePathName implementation
#endif /* end of: #if defined(_WIN32) */

#if defined WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char**argv)
#endif
{
#if defined WIN32
	bool bStartWebBrowser=true;
	RedirectIOToConsole();
#endif
	_log.Log(LOG_NORM,"Domoticz V%s%d (c)2012-2013 GizMoCuz",VERSION_STRING,SVNVERSION);

	szStartupFolder="";
#if !defined WIN32
	//Check if we are running on a RaspberryPi
	std::string sLine = "";
	std::ifstream infile;

	infile.open("/proc/cpuinfo");
	if (infile.is_open())
	{
		while (!infile.eof())
		{
			getline(infile, sLine);
			if (sLine.find("BCM2708")!=std::string::npos)
			{
				_log.Log(LOG_NORM,"System: Raspberry Pi");
				bIsRaspberryPi=true;
				break;
			}
		}
		infile.close();
	}
	if (bIsRaspberryPi)
	{
		std::ifstream infile1wire;
		std::string wire1catfile="/sys/bus/w1/devices";
		wire1catfile+="/w1_bus_master1/w1_master_slaves";
		infile1wire.open(wire1catfile.c_str());
		if (infile1wire.is_open())
		{
			bHave1Wire=true;
			infile1wire.close();
		}
	}
	char szStartupPath[255];
	getExecutablePathName((char*)&szStartupPath,255);
	szStartupFolder=szStartupPath;
	if (szStartupFolder.find_last_of('/')!=std::string::npos)
		szStartupFolder=szStartupFolder.substr(0,szStartupFolder.find_last_of('/')+1);
	_log.Log(LOG_NORM,"Startup Path: %s", szStartupFolder.c_str());
#endif

	CCmdLine cmdLine;

	// parse argc,argv 
#if defined WIN32
	cmdLine.SplitLine(__argc, __argv);
#else
	cmdLine.SplitLine(argc, argv);
#endif

	if ((cmdLine.HasSwitch("-h"))||(cmdLine.HasSwitch("--help"))||(cmdLine.HasSwitch("/?")))
	{
		_log.Log(LOG_NORM,szHelp);
		return 0;
	}

	if (cmdLine.HasSwitch("-startupdelay"))
	{
		if (cmdLine.GetArgumentCount("-startupdelay")!=1)
		{
			_log.Log(LOG_ERROR,"Please specify a startupdelay");
			return 0;
		}
		int DelaySeconds=atoi(cmdLine.GetSafeArgument("-startupdelay",0,"").c_str());
		_log.Log(LOG_NORM,"Startup delay... waiting %d seconds...",DelaySeconds);
		boost::this_thread::sleep(boost::posix_time::seconds(DelaySeconds));
	}

	if (cmdLine.HasSwitch("-www"))
	{
		if (cmdLine.GetArgumentCount("-www")!=1)
		{
			_log.Log(LOG_ERROR,"Please specify a port");
			return 0;
		}
		std::string wwwport=cmdLine.GetSafeArgument("-www",0,"8080");
		_mainworker.SetWebserverPort(wwwport);
	}
	if (cmdLine.HasSwitch("-nowwwpwd"))
	{
		_mainworker.m_bIgnoreUsernamePassword=true;
	}

	std::string dbasefile=szStartupFolder + "domoticz.db";
#ifdef WIN32
	if (!IsUserAnAdmin())
	{
		char szPath[MAX_PATH];
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath);
		if (SUCCEEDED(hr))
		{
			std::string sPath=szPath;
			sPath+="\\Domoticz";

			DWORD dwAttr = GetFileAttributes(sPath.c_str());
			BOOL bDirExists=(dwAttr != 0xffffffff && (dwAttr & FILE_ATTRIBUTE_DIRECTORY));
			if (!bDirExists)
			{
				BOOL bRet=CreateDirectory(sPath.c_str(),NULL);
				if (bRet==FALSE) {
					MessageBox(0,"Error creating Domoticz directory in program data folder (%ProgramData%)!!","Error:",MB_OK);
				}
			}
			sPath+="\\domoticz.db";
			dbasefile=sPath;
		}
	}
#endif

	if (cmdLine.HasSwitch("-dbase"))
	{
		if (cmdLine.GetArgumentCount("-dbase")!=1)
		{
			_log.Log(LOG_ERROR,"Please specify a Database Name");
			return 0;
		}
		dbasefile=cmdLine.GetSafeArgument("-dbase",0,"domoticz.db");
	}
	_mainworker.m_sql.SetDatabaseName(dbasefile);


	if (cmdLine.HasSwitch("-verbose"))
	{
		if (cmdLine.GetArgumentCount("-verbose")!=1)
		{
			_log.Log(LOG_ERROR,"Please specify a verbose level");
			return 0;
		}
		int Level=atoi(cmdLine.GetSafeArgument("-verbose",0,"").c_str());
		_mainworker.SetVerboseLevel((eVerboseLevel)Level);
	}
#if defined WIN32
	if (cmdLine.HasSwitch("-nobrowser"))
	{
		bStartWebBrowser=false;
	}
#endif
	if (cmdLine.HasSwitch("-logfile"))
	{
		if (cmdLine.GetArgumentCount("-logfile")!=1)
		{
			_log.Log(LOG_ERROR,"Please specify an output log file");
			return 0;
		}
		std::string logfile=cmdLine.GetSafeArgument("-logfile",0,"domoticz.log");
		_log.SetOutputFile(logfile.c_str());
	}
	
	if (!_mainworker.Start())
	{
		return 0;
	}

	signal(SIGINT, catch_intterm); 
	signal(SIGTERM,catch_intterm);
	
	/* now, lets get into an infinite loop of doing nothing. */

#if defined WIN32
#ifndef _DEBUG
	RedirectIOToConsole();	//hide console
#endif
	InitWindowsHelper(hInstance,hPrevInstance,nShowCmd,DQuitFunction,atoi(_mainworker.GetWebserverPort().c_str()),bStartWebBrowser);
	MSG Msg;
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
#else
	for ( ;; )
		boost::this_thread::sleep(boost::posix_time::seconds(1));
#endif
	return 0;
}

