#pragma once

#include "ASyncTCP.h"
#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

class RelayNet : public CDomoticzHardwareBase, ASyncTCP
{

      public:
	RelayNet(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &username, const std::string &password, bool pollInputs, bool pollRelays, int pollInterval,
		 int inputCount, int relayCount);
	~RelayNet() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void Init();
	void SetupDevices();
	void TcpGetSetRelay(int RelayNumber, bool Set, bool State);
	void UpdateDomoticzInput(int InputNumber, bool State);
	void UpdateDomoticzRelay(int OutputNumber, bool State);
	void ProcessRelaycardDump(char *Dump);
	void SetRelayState(int RelayNumber, bool State);
	void TcpRequestRelaycardDump();
	bool WriteToHardwareTcp(const char *pdata);
	bool WriteToHardwareHttp(const char *pdata);
	void ParseData(const unsigned char *pData, int Len);
	void KeepConnectionAlive();

	/* ASyncTCP required methodes */
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code &error) override;

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_username;
	std::string m_password;
	sockaddr_in m_addr;
	bool m_poll_inputs;
	bool m_poll_relays;
	bool m_setup_devices;
	int m_skip_relay_update;
	int m_poll_interval;
	int m_input_count;
	int m_relay_count;
	int m_retrycntr;
	std::shared_ptr<std::thread> m_thread;
	tRBUF m_Packet;
};
