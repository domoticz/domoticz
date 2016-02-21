#pragma once

#include "DomoticzHardware.h"
#include "../hardware/1Wire/1WireCommon.h"

class I_1WireSystem;
class C1Wire : public CDomoticzHardwareBase
{
public:
	explicit C1Wire(const int ID);
	virtual ~C1Wire();

	static bool Have1WireSystem();
	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	I_1WireSystem* m_system;
	std::map<std::string, bool> m_LastSwitchState;
	std::vector<_t1WireDevice> m_devices;

	static void LogSystem();
	void DetectSystem();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetDeviceDetails();

	// Messages to Domoticz
	void ReportLightState(const std::string& deviceId, const int unit, const bool state);
	void ReportTemperature(const std::string& deviceId, const float temperature);
	void ReportTemperatureHumidity(const std::string& deviceId, const float temperature, const float humidity);
	void ReportHumidity(const std::string& deviceId, const float humidity);
	void ReportPressure(const std::string& deviceId,const float pressure);
	void ReportCounter(const std::string& deviceId,const int unit,const unsigned long counter);
	void ReportVoltage(const std::string& deviceId, const int unit, const int voltage);
	void ReportIlluminance(const std::string& deviceId, const float illuminescence);
};
