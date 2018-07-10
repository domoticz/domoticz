#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CEcoCompteur : public CDomoticzHardwareBase
{
public:
	CEcoCompteur(const int ID, const std::string& url, const unsigned short port);
	~CEcoCompteur(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetScript();
private:
	std::string m_url;
	unsigned short m_port;
	unsigned short m_refresh;

	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

