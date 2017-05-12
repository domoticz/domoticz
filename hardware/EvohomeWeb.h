/**
 * Skeleton class for Integrated Evohome client for Domoticz
 *
 *  Copyright 2017 - gordonb3 https://github.com/gordonb3/evohomeclient
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <https://github.com/gordonb3/evohomeclient/blob/master/LICENSE>
 *
 *
 *
 *
 */

#pragma once

#include "EvohomeBase.h"


class CEvohomeWeb : public CEvohomeBase
{
public:
	CEvohomeWeb(const int ID);
	~CEvohomeWeb(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	// evohome web commands
	void GetStatus();
	void SetSystemMode();
	void SetTemperature();
	void SetDHWState();
};

