#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "TeleinfoBase.h"

class CEcoDevices : public CTeleinfoBase
{
	struct EcoStatus
	{
		uint8_t len;
		std::string hostname; // EcoDevices configured hostname
		std::string version;  // EcoDevices firmware version
		uint32_t flow1;	      // current flow l/mn counter 1
		uint32_t flow2;	      // current flow l/mn counter 2
		uint32_t index1;      // index counter 1, liters
		uint32_t index2;      // index counter 2, liters
		std::string t1_ptec;  // Subcription on input Teleinfo 1
		std::string t2_ptec;  // subscription on input Teleifo 2
		uint32_t pflow1;      // previous current flow counter 1
		uint32_t pflow2;      // previous current flow counter 2
		uint32_t pindex1;     // previous index counter 1
		uint32_t pindex2;     // previous index counter 2
		time_t time1;	      // time counter 1 sent
		time_t time2;	      // time counter 2 sent
		uint32_t voltage;     // voltage, for model RT2 only
		EcoStatus()
		{
			len = sizeof(EcoStatus) - 1;
			flow1 = 0;
			flow2 = 0;
			index1 = 0;
			index2 = 0;
			pflow1 = 0;
			pflow2 = 0;
			pindex1 = 0;
			pindex2 = 0;
			time1 = 0;
			time2 = 0;
			voltage = 0;
		}
	};

      public:
	CEcoDevices(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password, int datatimeout, int model, int ratelimit);
	~CEcoDevices() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool Login();
	void Logout();
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void DecodeXML2Teleinfo(const std::string &sResult, Teleinfo &teleinfo);
	void GetMeterDetails();
	void GetMeterRT2Details();

      private:
	std::string m_szIPAddress, m_username, m_password;
	std::stringstream m_ssURL;
	unsigned short m_usIPPort;
	bool m_bFirstRun;
	int m_iModel;
	std::shared_ptr<std::thread> m_thread;

	EcoStatus m_status;

	Teleinfo m_teleinfo1, m_teleinfo2;
};
