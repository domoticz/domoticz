#pragma once

#include "DomoticzHardware.h"

class CPVOutputInput : public CDomoticzHardwareBase
{
      public:
	CPVOutputInput(int ID, const std::string &SID, const std::string &Key);
	~CPVOutputInput() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();

      private:
	std::string m_SID;
	std::string m_KEY;
	std::shared_ptr<std::thread> m_thread;
	bool m_bHadConsumption = false;
};
