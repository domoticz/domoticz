#pragma once


#include <iosfwd>
#include <set>
#include "DomoticzHardware.h"
#include "../hardware/1Wire/1WireCommon.h"

class I_1WireSystem;
class C1Wire : public CDomoticzHardwareBase
{
public:
	explicit C1Wire(const int ID, const int sensorThreadPeriod, const int switchThreadPeriod, const std::string& path);
	virtual ~C1Wire();

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_threadSensors;
	boost::shared_ptr<boost::thread> m_threadSwitches;
	I_1WireSystem* m_system;
	std::map<std::string, bool> m_LastSwitchState;
	std::set<_t1WireDevice> m_sensors;
	std::set<_t1WireDevice> m_switches;

	int m_sensorThreadPeriod; // milliseconds
	int m_switchThreadPeriod; // milliseconds
	const std::string &m_path;
	bool m_bSensorFirstTime;
	bool m_bSwitchFirstTime;

	void DetectSystem();
	bool StartHardware();
	bool StopHardware();
	void SensorThread();
	void SwitchThread();
	void BuildSensorList();
	void BuildSwitchList();
	void PollSwitches();

	// Messages to Domoticz
	void ReportLightState(const std::string& deviceId, const int unit, const bool state);
	void ReportWiper(const std::string& deviceId, const int wiper);
	void ReportTemperature(const std::string& deviceId, const float temperature);
	void ReportTemperatureHumidity(const std::string& deviceId, const float temperature, const float humidity);
	void ReportHumidity(const std::string& deviceId, const float humidity);
	void ReportPressure(const std::string& deviceId,const float pressure);
	void ReportCounter(const std::string& deviceId,const int unit,const unsigned long counter);
	void ReportVoltage(const std::string& deviceId, const int unit, const int voltage);
	void ReportIlluminance(const std::string& deviceId, const float illuminescence);
};
