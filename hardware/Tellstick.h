#pragma once

#ifdef WITH_TELLDUSCORE

#include "DomoticzHardware.h"
#include <iostream>

class CTellstick : public CDomoticzHardwareBase
{
public:
	explicit CTellstick(const int ID);
	~CTellstick(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SendCommand(const int ID, const std::string &command);

	void deviceEvent(int deviceId, int method, const char *data);
	static void deviceEventCallback(int deviceId, int method, const char *data, int callbackId, void *context);
private:
	bool AddSwitchIfNotExits(const int id, const char* devname, bool isDimmer);
	void Init();
	bool StartHardware();
	bool StopHardware();
};

#endif //WITH_TELLSTICK
