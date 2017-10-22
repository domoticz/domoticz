#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class Arilux : public CDomoticzHardwareBase
{
public:
	explicit Arilux(const int ID);
	~Arilux(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &ariluxBright, const std::string &ariluxHue);	
	boost::signals2::signal<void()> sDisconnected;
private:
	bool SendTCPCommand(char ip[50],std::vector<unsigned char> &command);
	bool StartHardware();
	bool StopHardware();
	float cHue;
	bool isWhite;
	int brightness;
protected:
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	
};
