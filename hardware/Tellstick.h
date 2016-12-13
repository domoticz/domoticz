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

	void sensorEvent(int deviceId, const char *protocol, const char *model, int dataType, const char *value);
	static void sensorEventCallback(const char *protocol, const char *model, int id, int dataType, const char *value,
					int timestamp, int callbackId, void *context);

private:
	bool AddSwitchIfNotExits(const int id, const char* devname, bool isDimmer);
	void Init();
	bool StartHardware();
	bool StopHardware();

	int deviceEventId;
	int sensorEventId;

};

#endif //WITH_TELLSTICK
