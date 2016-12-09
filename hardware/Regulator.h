#pragma once

#include "DomoticzHardware.h"
#include "RegDevice.h"

class CRegulator : public CDomoticzHardwareBase
{
public:
	CRegulator(const int ID, const unsigned short refresh);
	~CRegulator(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	double m_dHysteresis;
	unsigned short m_refresh;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::vector<CRegDevice> m_regulatedDevices;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	bool GetTemperature(std::string sTempDevID, float &fvalue, time_t& lastUpdate);
	bool GetSetpoint(std::string sTempDevID, float &fvalue);
	bool GetSwitchState(std::string sSwitchDevID, bool &bvalue);

	void ReadDatabase();
	void Process();
};

