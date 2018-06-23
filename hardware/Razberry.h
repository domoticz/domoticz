#pragma once

#include "DomoticzHardware.h"

class CRazberry : public CDomoticzHardwareBase
{
public:
	CRazberry(const int ID, const std::string &ipaddress, const int port, const std::string &username, const std::string &password);
	~CRazberry(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
};



