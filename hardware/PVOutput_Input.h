#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CPVOutputInput : public CDomoticzHardwareBase
{
public:
	CPVOutputInput(const int ID, const std::string &SID, const std::string &Key);
	~CPVOutputInput(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_SID;
	std::string m_KEY;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
};

