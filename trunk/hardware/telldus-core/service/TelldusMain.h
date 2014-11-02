//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_SERVICE_TELLDUSMAIN_H_
#define TELLDUS_CORE_SERVICE_TELLDUSMAIN_H_

class TelldusMain {
public:
	TelldusMain(void);
	~TelldusMain(void);

	void start();
	void stop();

	// Thread safe!
	void deviceInsertedOrRemoved(int vid, int pid, bool inserted);
	void resume();
	void suspend();

private:
	class PrivateData;
	PrivateData *d;
};

#endif  // TELLDUS_CORE_SERVICE_TELLDUSMAIN_H_
