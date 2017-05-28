#pragma once

#include "DomoticzHardware.h"

class Rtl433 : public CDomoticzHardwareBase
{
public:
	explicit Rtl433(const int ID);
	virtual ~Rtl433();

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_threadReceive;

	bool StartHardware();
	bool StopHardware();
	void ReceiveThread();
	static std::vector<std::string> ParseCSVLine(const char *input);
};
