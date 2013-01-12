// domoticz.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mainworker.h"

#include <stdio.h>     /* standard I/O functions                         */
#include <sys/types.h> /* various type definitions, like pid_t           */
#include <signal.h>    /* signal name macros, and the signal() prototype */ 
#include <iostream>
#include "CmdLine.h"
#include "appversion.h"

#if defined WIN32
#include "WindowsHelper.h"
#endif

const char *szHelp=
	"Usage: Domoticz -www port -verbose x\n"
	"\t-www port (for example -www 8080)\n"
	"\t-dbase file_path (for example D:\\domoticz.db)\n"
	"\t-verbose x (where x=0 is none, x=1 is debug)\n"
	"\t-startupdelay seconds (default=0)\n"
	"\t-nowwwpwd (in case you forgot the webserver username/password)\n";

std::string szStartupFolder;

MainWorker _mainworker;

void DQuitFunction()
{
	std::cout << "Closing application!..." << std::endl;
	fflush(stdout);
#if defined WIN32
	TrayMessage(NIM_DELETE,NULL);
#endif
	std::cout << "stopping worker...\n";
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
	RedirectIOToConsole();
#endif
	std::cout << "Domoticz V" << VERSION_STRING << " (c)2012-2013 GizMoCuz\n";

	szStartupFolder="";
#if !defined WIN32
	char szStartupPath[255];
	getExecutablePathName((char*)&szStartupPath,255);
	szStartupFolder=szStartupPath;
	if (szStartupFolder.find_last_of('/')!=std::string::npos)
		szStartupFolder=szStartupFolder.substr(0,szStartupFolder.find_last_of('/')+1);
	//std::cout << "Startup Path: " << szStartupFolder << std::endl;
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
		std::cout << szHelp;
		return 0;
	}

	if (cmdLine.HasSwitch("-startupdelay"))
	{
		if (cmdLine.GetArgumentCount("-startupdelay")!=1)
		{
			std::cout << "Please specify a startupdelay" << std::endl;
			return 0;
		}
		int DelaySeconds=atoi(cmdLine.GetSafeArgument("-startupdelay",0,"").c_str());
		std::cout << "Startup delay... waiting " << DelaySeconds << " seconds..." << std::endl;
		boost::this_thread::sleep(boost::posix_time::seconds(DelaySeconds));
	}

	if (cmdLine.HasSwitch("-www"))
	{
		if (cmdLine.GetArgumentCount("-www")!=1)
		{
			std::cout << "Please specify a port" << std::endl;
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
	if (cmdLine.HasSwitch("-dbase"))
	{
		if (cmdLine.GetArgumentCount("-dbase")!=1)
		{
			std::cout << "Please specify a Database Name" << std::endl;
			return 0;
		}
		dbasefile=cmdLine.GetSafeArgument("-dbase",0,"domoticz.db");
	}
	_mainworker.m_sql.SetDatabaseName(dbasefile);


	if (cmdLine.HasSwitch("-verbose"))
	{
		if (cmdLine.GetArgumentCount("-verbose")!=1)
		{
			std::cout << "Please specify a verbose level" << std::endl;
			return 0;
		}
		int Level=atoi(cmdLine.GetSafeArgument("-verbose",0,"").c_str());
		_mainworker.SetVerboseLevel((eVerboseLevel)Level);
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
	InitWindowsHelper(hInstance,hPrevInstance,nShowCmd,DQuitFunction,atoi(_mainworker.GetWebserverPort().c_str()));
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

