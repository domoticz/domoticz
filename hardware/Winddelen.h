#pragma once

#include "DomoticzHardware.h"

class CWinddelen : public CDomoticzHardwareBase
{
public:
	CWinddelen(const int ID, const std::string &IPAddress, const unsigned short usTotParts, const unsigned short usMillID);
	~CWinddelen(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_szMillName;
	unsigned short m_usMillID;
	unsigned short m_usTotParts;
	std::map<int, float> m_winddelen_per_mill;

	std::shared_ptr<std::thread> m_thread;
};

