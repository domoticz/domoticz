#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"

class OnkyoAVTCP : public CDomoticzHardwareBase, ASyncTCP
{
      public:
	OnkyoAVTCP(int ID, const std::string &IPAddress, unsigned short usIPPort);
	~OnkyoAVTCP() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	bool SendPacket(const char *pdata);

      private:
	bool SendPacket(const char *pCmd, const char *pArg);
	bool StartHardware() override;
	bool StopHardware() override;
	bool CustomCommand(uint64_t idx, const std::string &sCommand) override;
	void ReceiveMessage(const char *pData, int Len);
	void ReceiveSwitchMsg(const char *pData, int Len, bool muting, int ID);
	bool ReceiveXML(const char *pData, int Len);
	void EnsureSwitchDevice(int Unit, const char *options = nullptr);
	std::string BuildSelectorOptions(const std::string &names, const std::string &ids);
	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

	void ParseData(const unsigned char *pData, int Len);

      private:
	int m_retrycntr;
	unsigned char *m_pPartialPkt;
	int m_PPktLen;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::shared_ptr<std::thread> m_thread;
};
