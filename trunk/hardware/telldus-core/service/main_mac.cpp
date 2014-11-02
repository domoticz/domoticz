//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <signal.h>
#include "service/TelldusMain.h"
#include "service/Log.h"

TelldusMain tm;

void shutdownHandler(int onSignal) {
	Log::notice("Shutting down");
	tm.stop();
}

void sigpipeHandler(int onSignal) {
	Log::notice("SIGPIPE received");
}

int main(int argc, char **argv) {
	/* Install signal traps for proper shutdown */
	signal(SIGTERM, shutdownHandler);
	signal(SIGINT, shutdownHandler);
	signal(SIGPIPE, sigpipeHandler);

	Log::notice("telldusd started");
	tm.start();
	Log::notice("telldusd stopped gracefully");

	Log::destroy();
	return 0;
}
