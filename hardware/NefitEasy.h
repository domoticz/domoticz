#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CNefitEasy : public CDomoticzHardwareBase
{
      public:
	CNefitEasy(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~CNefitEasy() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool GetStatusDetails();
	bool GetOutdoorTemp();
	bool GetFlowTemp();
	bool GetPressure();
	bool GetDisplayCode();
	bool GetGasUsage();
	void SetUserMode(bool bSetUserModeClock);
	void SetHotWaterMode(bool bTurnOn);

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

	std::shared_ptr<std::thread> m_thread;

	uint32_t m_lastgasusage;
	P1Gas m_p1gas;
};
