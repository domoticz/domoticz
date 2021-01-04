#pragma once

#include "DomoticzHardware.h"

class CWinddelen : public CDomoticzHardwareBase
{
      public:
	CWinddelen(int ID, const std::string &IPAddress, unsigned short usTotParts, unsigned short usMillID);
	~CWinddelen() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

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
