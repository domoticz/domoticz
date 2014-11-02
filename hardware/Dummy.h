#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CDummy : public CDomoticzHardwareBase
{
public:
	CDummy(const int ID);
	~CDummy(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
};

