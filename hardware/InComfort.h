#pragma once

// Intergas InComfort Domoticz driver v1.0.1.
// Tested with LAN2RF gateway WebInterface 1.1, Firmware IC3-ICN-V1.12
// by Erik Tamminga (erik@etamminga.nl)
//
// 2017-01-17: v1.0.1 etamminga
// - IO values did not get send to the correct sensor
// - Slow changing values were not updated. Forcing an update every 15 minutes.

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CInComfort : public CDomoticzHardwareBase
{
      public:
	CInComfort(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~CInComfort() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	void SetProgramState(int newState);

      private:
	bool Login();
	void Logout();
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetHeaterDetails();
	std::string GetHTTPData(const std::string &sURL);
	void ParseAndUpdateDevices(const std::string &jsonData);
	std::string SetRoom1SetTemperature(float tempSetpoint);

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::shared_ptr<std::thread> m_thread;

	time_t m_LastUpdateFrequentChangingValues;
	time_t m_LastUpdateSlowChangingValues;
	float m_LastRoom1Temperature;
	float m_LastRoom2Temperature;
	float m_LastRoom1SetTemperature;
	float m_LastRoom1OverrideTemperature;
	float m_LastRoom2SetTemperature;
	float m_LastRoom2OverrideTemperature;

	float m_LastCentralHeatingTemperature;
	float m_LastCentralHeatingPressure;
	float m_LastTapWaterTemperature;

	std::string m_LastStatusText;
	int m_LastIO;
};
