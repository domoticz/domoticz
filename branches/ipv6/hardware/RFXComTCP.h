#pragma once

#include <deque>
#include <iostream>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class RFXComTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	RFXComTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~RFXComTCP(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);
	bool onInternalMessage(const unsigned char *pBuffer, const size_t Len);
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	bool m_bReceiverStarted;
};

