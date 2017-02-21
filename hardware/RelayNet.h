#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"
#include <iostream>

class RelayNet : public CDomoticzHardwareBase
{

public:

	RelayNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password, const bool pollInputs, const bool pollRelays, const int pollInterval, const int inputCount, const int relayCount);
	~RelayNet(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);
	bool WriteToHardwareTcp(const char *pdata, const unsigned char length);
	bool WriteToHardwareHttp(const char *pdata, const unsigned char length);

private:

	std::string 						m_szIPAddress;
	unsigned short 						m_usIPPort;
	std::string 						m_username;
	std::string 						m_password;
	volatile bool 						m_stoprequested;
	sockaddr_in 						m_addr;
	bool								m_poll_inputs;
	bool								m_poll_relays;
	bool								m_connected;
	bool								m_keep_connection;
	int									m_poll_interval;
	int									m_input_count;
	int									m_relay_count;
	boost::shared_ptr<boost::thread> 	m_thread;
	tRBUF 								Packet;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void SetupDevices();
	bool RelaycardTcpConnect();
	void RelaycardTcpDisconnect();
	bool TcpGetSetRelay(int RelayNumber, bool Set, bool State);
	void UpdateDomoticzRelayState(int RelayNumber, bool State);
	bool UpdateDomoticzInput(int InputNumber, bool State);
	bool UpdateDomoticzOutput(int OutputNumber, bool State);
	bool ProcessRelaycardDump(char* Dump, int Length);
	bool GetRelayState(int RelayNumber);
	bool SetRelayState(int RelayNumber, bool State);
	bool TcpGetRelaycardDump(char* Buffer, int Length);

protected:

	int m_socket;
};

