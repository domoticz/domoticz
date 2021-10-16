#pragma once

#include <set>
#include "DomoticzHardware.h"
#include "../hardware/1Wire/1WireCommon.h"

class I_1WireSystem;
class C1Wire : public CDomoticzHardwareBase
{
      public:
	explicit C1Wire(int ID, int sensorThreadPeriod, int switchThreadPeriod, const std::string &path);
	~C1Wire() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void DetectSystem();
	bool StartHardware() override;
	bool StopHardware() override;
	void SensorThread();
	void SwitchThread();
	void BuildSensorList();
	void BuildSwitchList();
	void PollSwitches();

	// Messages to Domoticz
	void ReportLightState(const std::string &deviceId, uint8_t unit, bool state);
	void ReportWiper(const std::string &deviceId, int wiper);
	void ReportTemperature(const std::string &deviceId, float temperature);
	void ReportTemperatureHumidity(const std::string &deviceId, float temperature, float humidity);
	void ReportHumidity(const std::string &deviceId, float humidity);
	void ReportPressure(const std::string &deviceId, float pressure);
	void ReportCounter(const std::string &deviceId, int unit, unsigned long counter);
	void ReportVoltage(const std::string &deviceId, int unit, int voltage);
	void ReportIlluminance(const std::string &deviceId, float illuminescence);

      private:
	std::shared_ptr<std::thread> m_threadSensors;
	std::shared_ptr<std::thread> m_threadSwitches;
	StoppableTask m_TaskSwitches;
	I_1WireSystem *m_system;
	std::map<std::string, bool> m_LastSwitchState;
	std::set<_t1WireDevice> m_sensors;
	std::set<_t1WireDevice> m_switches;

	int m_sensorThreadPeriod; // milliseconds
	int m_switchThreadPeriod; // milliseconds
	const std::string &m_path;
	bool m_bSensorFirstTime;
	bool m_bSwitchFirstTime;
};
