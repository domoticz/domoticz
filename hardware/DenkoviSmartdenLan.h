#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CDenkoviSmartdenLan : public CDomoticzHardwareBase
{
public:
	CDenkoviSmartdenLan(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password);
	~CDenkoviSmartdenLan(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
	void UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);
};

