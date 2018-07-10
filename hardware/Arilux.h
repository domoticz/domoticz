#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class Arilux : public CDomoticzHardwareBase
{
public:
	explicit Arilux(const int ID);
	~Arilux(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &ariluxBright, const std::string &ariluxHue);
private:
	bool SendTCPCommand(char ip[50],std::vector<unsigned char> &command);
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
private:
	_tColor m_color;
	bool m_isWhite;
	bool m_bDoRestart;
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
};
