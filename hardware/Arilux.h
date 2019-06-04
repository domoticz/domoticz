#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class Arilux : public CDomoticzHardwareBase
{
public:
	explicit Arilux(const int ID);
	~Arilux(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void InsertUpdateSwitch(const std::string name, const int switchType, const std::string location);
private:
	bool SendTCPCommand(uint32_t ip,std::vector<unsigned char> &command);
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
private:
	_tColor m_color;
	std::shared_ptr<std::thread> m_thread;
};
