// domoticz.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mainworker.h"

#include <stdio.h>     /* standard I/O functions                         */
#include <sys/types.h> /* various type definitions, like pid_t           */
#include <signal.h>    /* signal name macros, and the signal() prototype */ 

#include "CmdLine.h"

const char *szAppTitle="Domoticz V1.01 (c)2012 GizMoCuz\n";

const char *szHelp=
	"Usage: Domoticz -www port -verbose x\n"
	"\t-www port (for example -www 8080)\n"
	"\t-verbose x (where x=0 is none, x=1 is debug)\n";


MainWorker _mainworker;

/* first, here is the signal handler */
void catch_int(int sig_num)
{
	/* re-set the signal handler again to catch_int, for next time */
	signal(SIGINT, catch_int);
	/* and print the message */
	std::cout << "Closing application!..." << std::endl;
	fflush(stdout);
	std::cout << "stopping worker...\n";
	_mainworker.Stop();
	exit(0); 
}

int main(int argc, char**argv)
{
	std::cout << szAppTitle;

	CCmdLine cmdLine;
	// parse argc,argv 
	cmdLine.SplitLine(argc, argv);

	if ((cmdLine.HasSwitch("-h"))||(cmdLine.HasSwitch("--help"))||(cmdLine.HasSwitch("/?")))
	{
		std::cout << szHelp;
		return 0;
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

	std::cout << "Webserver listning on port: " << _mainworker.GetWebserverPort() << std::endl;
	if (!_mainworker.Start())
		return 0;

	/* set the INT (Ctrl-C)
	signal handler to 'catch_int' */ 

	signal(SIGINT, catch_int); 

	/* now, lets get into an infinite loop of doing nothing. */
	for ( ;; )
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	return 0;
}

