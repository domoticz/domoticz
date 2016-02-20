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
	void ReportLightState(const std::string& deviceId,int unit,bool state);
	void ReportTemperature(const std::string& deviceId,float temperature);
	void ReportTemperatureHumidity(const std::string& deviceId,float temperature,float humidity);
	void ReportHumidity(const std::string& deviceId,float humidity);
	void ReportPressure(const std::string& deviceId,float pressure);
	void ReportCounter(const std::string& deviceId,int unit,unsigned long counter);
	void ReportVoltage(int unit,int voltage);
	void ReportIlluminescence(float illuminescence);
};
