#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class OnkyoAVTCP : public CDomoticzHardwareBase, ASyncTCP
{
public:
	OnkyoAVTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~OnkyoAVTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()>	sDisconnected;
	bool SendPacket(const char *pdata);
private:
	bool isConnected() { return mIsConnected; };
	bool SendPacket(const char *pCmd, const char *pArg);
	bool StartHardware() override;
	bool StopHardware() override;
	bool CustomCommand(uint64_t idx, const std::string &sCommand) override;
	void ReceiveMessage(const char *pData, int Len);
	void ReceiveSwitchMsg(const char *pData, int Len, bool muting, int ID);
	bool ReceiveXML(const char *pData, int Len);
	void EnsureSwitchDevice(int Unit, const char *options = NULL);
	std::string BuildSelectorOptions(const std::string & names, const std::string & ids);
	void Do_Work();
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
	void ParseData(const unsigned char *pData, int Len);
private:
	int m_retrycntr;
	unsigned char *m_pPartialPkt;
	int m_PPktLen;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;
	std::shared_ptr<std::thread> m_thread;
	volatile bool m_stoprequested;
};
