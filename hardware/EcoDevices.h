#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CEcoDevices : public CDomoticzHardwareBase
{
	public:
		CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
		~CEcoDevices(void);
		bool WriteToHardware(const char *pdata, const unsigned char length);
	private:
		bool Login();
		void Logout();

		std::string m_szIPAddress;
		unsigned short m_usIPPort;
		bool m_stoprequested;
		boost::shared_ptr<boost::thread> m_thread;

		P1Power m_p1power1, m_p1power2;
		P1Gas   m_p1water, m_p1gas;
		time_t m_lastSendData;
		unsigned long m_lastusage1, m_lastusage2, m_lastwaterusage, m_lastgasusage;

		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();
		void GetMeterDetails();
};
