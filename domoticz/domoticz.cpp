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
	"Usage: Domoticz connection settings\n"
	"\tconnection settings could be one of the below:\n"
	"\t\t-serial Port (for example /dev/ttyUSB0 or COM1)\n"
	"\t\t-ip Address Port (for example 192.168.0.1 10001)\n"
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

	bool bIsSerial=false;
	bool bIsTCP=false;
	std::string szSerialPort;
	std::string szIPAddress;
	unsigned short usIPPort=-1;

	if (argc < 3)
	{
		std::cout << szHelp;
		return 0;
	}

	CCmdLine cmdLine;
	// parse argc,argv 
	if (cmdLine.SplitLine(argc, argv) <= 0)
	{
		std::cout << szHelp;
		return 0;
	}
	if (cmdLine.HasSwitch("-ip"))
	{
		if (cmdLine.GetArgumentCount("-ip")!=2)
		{
			std::cout << "Please specify address/port" << std::endl;
			return 0;
		}
		szIPAddress=cmdLine.GetSafeArgument("-ip",0,"");
		usIPPort=atoi(cmdLine.GetSafeArgument("-ip",1,"").c_str());
		bIsTCP=true;
	}
	else if (cmdLine.HasSwitch("-serial"))
	{
		if (cmdLine.GetArgumentCount("-serial")!=1)
		{
			std::cout << "Please specify a serial port" << std::endl;
			return 0;
		}
		szSerialPort=cmdLine.GetSafeArgument("-serial",0,"");
		bIsSerial=true;
	}

	if ((!bIsSerial)&&(!bIsTCP))
	{
		std::cout << "unknown input type specified!\n";
		return 0;
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

