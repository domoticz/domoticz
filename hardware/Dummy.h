#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CDummy : public CDomoticzHardwareBase
{
public:
	explicit CDummy(const int ID);
	~CDummy(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
};

