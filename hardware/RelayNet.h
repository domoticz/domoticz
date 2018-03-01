#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

class RelayNet : public CDomoticzHardwareBase, ASyncTCP
{

public:

	RelayNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password, const bool pollInputs, const bool pollRelays, const int pollInterval, const int inputCount, const int relayCount);
	~RelayNet(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	bool isConnected() { return mIsConnected; };

public:
	// signals
	boost::signals2::signal<void()>	sDisconnected;

private:

	std::string 						m_szIPAddress;
	unsigned short 						m_usIPPort;
	std::string 						m_username;
	std::string 						m_password;
	volatile bool 						m_stoprequested;
	sockaddr_in 						m_addr;
	bool								m_poll_inputs;
	bool								m_poll_relays;
	bool								m_reconnect;
	bool								m_bDoRestart;
	bool								m_setup_devices;
	int									m_skip_relay_update;
	int									m_poll_interval;
	int									m_input_count;
	int									m_relay_count;
	int									m_retrycntr;
	boost::shared_ptr<boost::thread> 	m_thread;
	tRBUF 								m_Packet;

	bool StartHardware();
	bool StopHardware();

protected:

	void Do_Work();
	void Init();
	void SetupDevices();
	void TcpGetSetRelay(int RelayNumber, bool Set, bool State);
	void UpdateDomoticzInput(int InputNumber, bool State);
	void UpdateDomoticzRelay(int OutputNumber, bool State);
	void ProcessRelaycardDump(char* Dump);
	void SetRelayState(int RelayNumber, bool State);
	void TcpRequestRelaycardDump();
	bool WriteToHardwareTcp(const char *pdata);
	bool WriteToHardwareHttp(const char *pdata);
	void ParseData(const unsigned char *pData, int Len);
	void KeepConnectionAlive();

	/* ASyncTCP required methodes */
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);
};

