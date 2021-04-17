#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
} // namespace Json

class CEcoCompteur : public CDomoticzHardwareBase
{
      public:
	CEcoCompteur(int ID, const std::string &url, unsigned short port);
	~CEcoCompteur() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

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
	std::shared_ptr<std::thread> m_thread;
};
