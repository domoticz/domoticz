#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>


class Yeelight : public CDomoticzHardwareBase
{
public:
	Yeelight(const int ID);
	~Yeelight(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue);
public:
	//signals
	boost::signals2::signal<void()> sDisconnected;
private:
	bool StartHardware();
	bool StopHardware();

protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;
	bool DiscoverLights();
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};
