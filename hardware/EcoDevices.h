#pragma once

#include <iostream>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "TeleinfoBase.h"

class CEcoDevices : public CTeleinfoBase
{
	public:
		CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
		~CEcoDevices();
		bool WriteToHardware(const char *pdata, const unsigned char length);
	private:
		bool Login();
		void Logout();

		std::string m_szIPAddress;
		unsigned short m_usIPPort;
		bool m_stoprequested;
		boost::shared_ptr<boost::thread> m_thread;

		typedef struct _tStatus
		{
			uint8_t     len;
			std::string hostname;// EcoDevices configured hostname
			std::string version; // EcoDevices firmware version
			uint32_t    flow1;	 // current flow l/mn counter 1
			uint32_t    flow2;	 // current flow l/mn counter 2
			uint32_t    index1;	 // index counter 1, liters
			uint32_t    index2;	 // index counter 2, liters
			std::string  t1_ptec;// Subcription on input Teleinfo 1
			std::string t2_ptec; // subscription on input Teleifo 2
			uint32_t    pflow1;	 // previous current flow counter 1
			uint32_t    pflow2;	 // previous current flow counter 2
			uint32_t    pindex1; // previous index counter 1
			uint32_t    pindex2; // previous index counter 2
			time_t      time1;	 // time counter 1 sent
			time_t      time2;	 // time counter 2 sent
			_tStatus()
			{
				len = sizeof(_tStatus) - 1;
				pflow1 = 0;
				pflow2 = 0;
				pindex1 = 0;
				pindex2 = 0;
				time1 = 0;
				time2 = 0;
			}
		} Status;

		Status m_status;

		Teleinfo m_teleinfo1, m_teleinfo2;

		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();
		void DecodeXML2Teleinfo(const std::string &sResult, Teleinfo &teleinfo);
		void GetMeterDetails();
};
