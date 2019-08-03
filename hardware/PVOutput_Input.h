#pragma once

#include "DomoticzHardware.h"

class CPVOutputInput : public CDomoticzHardwareBase
{
public:
	CPVOutputInput(const int ID, const std::string &SID, const std::string &Key);
	~CPVOutputInput(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
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
};

