#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class OnkyoAVTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	OnkyoAVTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~OnkyoAVTCP(void);
	bool isConnected(){ return mIsConnected; };
	bool WriteToHardware(const char *pdata, const unsigned char length);
	bool SendPacket(const char *pdata);
	bool SendPacket(const char *pCmd, const char *pArg);

public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;
private:
	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	bool CustomCommand(uint64_t idx, const std::string &sCommand);
	unsigned char *m_pPartialPkt;
	int m_PPktLen;
	void ReceiveMessage(const char *pData, int Len);
	void ReceiveSwitchMsg(const char *pData, int Len, bool muting, int ID);
	bool ReceiveXML(const char *pData, int Len);
	void EnsureSwitchDevice(int Unit, const char *options = NULL);
	std::string BuildSelectorOptions(const std::string & names, const std::string & ids);

 protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	void ParseData(const unsigned char *pData, int Len);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
};

