#pragma once

#include "DomoticzHardware.h"
#include <iosfwd>

class CLimitLess : public CDomoticzHardwareBase
{
	enum _eLimitlessBridgeType
	{
		LBTYPE_V4 = 0,
		LBTYPE_V5,
		LBTYPE_V6
	};
public:
	CLimitLess(const int ID, const int LedType, const int BridgeType, const std::string &IPAddress, const unsigned short usIPPort);
	~CLimitLess(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool AddSwitchIfNotExits(const unsigned char Unit, const std::string& devname);
	bool GetV6BridgeID();
	bool SendV6Command(const uint8_t *pCmd);
	bool IsDataAvailable(const SOCKET sock);
	_eLimitlessBridgeType m_BridgeType;
	unsigned char m_LEDType;

	uint8_t m_BridgeID_1;
	uint8_t m_BridgeID_2;
	uint8_t m_CommandCntr;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	SOCKET	m_RemoteSocket;
	sockaddr_in m_stRemoteDestAddr;

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
};

