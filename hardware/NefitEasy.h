#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>
#include "hardwaretypes.h"

class CNefitEasy : public CDomoticzHardwareBase
{
public:
	CNefitEasy(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~CNefitEasy(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSetpoint(const int idx, const float temp);
private:
	std::string m_AccessKey;
	std::string m_SerialNumber;
	std::string m_Password;

	std::string m_ConnectionPassword;
	std::string m_jid;
	std::string m_from;
	std::string m_to;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	std::string m_LastDisplayCode;
	std::string m_LastBoilerStatus;
	bool m_bClockMode;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	bool GetStatusDetails();
	bool GetOutdoorTemp();
	bool GetFlowTemp();
	bool GetPressure();
	bool GetDisplayCode();
	bool GetGasUsage();

	uint32_t m_lastgasusage;
	P1Gas	m_p1gas;

	void SetUserMode(bool bSetUserModeClock);
	void SetHotWaterMode(bool bTurnOn);

	//XMPP stuff
	bool ConnectToXMPP(const std::string &IPAddress, const int PortNumber);
	void Logout();
	bool m_bDoLogin;
};

