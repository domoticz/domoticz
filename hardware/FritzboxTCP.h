#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class FritzboxTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	FritzboxTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~FritzboxTCP(void);
	bool isConnected(){ return mIsConnected; };
	bool WriteToHardware(const char *pdata, const unsigned char length);
public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	void UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void WriteInt(const std::string &sendStr);


	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	int m_bufferpos;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

