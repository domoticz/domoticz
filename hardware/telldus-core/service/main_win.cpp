//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <windows.h>
#include <Dbt.h>

#include "service/TelldusWinService_win.h"
// #include <QCoreApplication>

int main(int argc, char **argv) {
	g_argc = argc;
	g_argv = argv;

	SERVICE_TABLE_ENTRY serviceTable[] = {
		{serviceName, TelldusWinService::serviceMain },
		{ 0, 0 }
	};

	StartServiceCtrlDispatcher( serviceTable );

	return 0;
}
