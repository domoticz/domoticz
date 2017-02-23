//---------------------------------------------------------------------------
//
//	Notes for RELAY-NET-V5.7 LAN 8 channel relay and binary input card.
//
//	Hard reset: Hold reset button and power on to hard reset the relay card
//
//	After Hard Reset:
//	IpAddress 	= 192.168.1.166
//	Username	= admin
//	Password	= 12345678
//
//	Relay-Net-V5.7 card settings:
//
//	To use Inputs set scene settings to all "xxxxxxxx" for all inputs. This
//	give full control of the relays and inputs to domoticz.
//
//	Password does not matter when only TCP traffic is used.
//	When HTTP is used then username / password setting on the module and domoticz must match.
//	After init the is user:admin /password:12345678
//
//	Select power down save
//	Deselect Input output correlation
//	Deselect scene mode
//	Select touch input
//
//	Make sure TCP address and port match domoticz settings (example addr:192.168.2.241 port:17494)
//	Set HTTP port to 80 so you can access the module from your browser.
//	If you have multiple modules make sure mac address settings are unique.
//	Set Default Gateway, Primary & Secondary DNS server to 0.0.0.0 in most cases.
//
//	To test the card using Linux curl:
//	Turn  relay1 on:	curl --basic --user admin:12345678 --silent --data "saida1on=on"
//	Turn  relay1 off:	curl --basic --user admin:12345678 --silent --data "saida1off=off"
//	Pulse relay1 on:	curl --basic --user admin:12345678 --silent --data "saida1pluse=pluse"
//	(firmware pulse -> pluse)
//
//	Relays: saida1..saida8 ~ relay1..relay8
//			saida9 = ALL off, on or pluse
//
//	TCP command examples:
//	L1\r\n		Turn on relay 1
//	LA\r\n		Turn on all relays
//	D1\r\n		Turn off relay1
//	DA\r\n		Turn off all relays
//	P1\r\n		Pulse relay 1
//	R1\r\n		Read state relay1
//	DUMP\r\n	Read state of all relays and all inputs
//
//	setr00000001\r\n 	set all relays in one go
//	getr\r\n			get status of all relays in one go (result getr00000001)
//
//	Using scenes (we disable this by putting all xxxxxxxx)
//
//	INON=?\r\n
//	result:
//	INON1 = xxxxxxxx\r\n
//	INON2 = xxxxxxxx\r\n
//	INON3 = xxxxxxxx\r\n
//	INON4 = xxxxxxxx\r\n
//	INON5 = xxxxxxxx\r\n
//	INON6 = xxxxxxxx\r\n
//	INON7 = xxxxxxxx\r\n
//	INON8 = xxxxxxxx\r\n
//
//	INOFF=?\r\n
//	result:
//	INOFF1 = xxxxxxxx\r\n
//	INOFF2 = xxxxxxxx\r\n
//	INOFF3 = xxxxxxxx\r\n
//	INOFF4 = xxxxxxxx\r\n
//	INOFF5 = xxxxxxxx\r\n
//	INOFF6 = xxxxxxxx\r\n
//	INOFF7 = xxxxxxxx\r\n
//	INOFF8 = xxxxxxxx\r\n
//
//	INON1 = xxxxxxxx\r\n scene 1 ON part
//	result:
//	INON1 = xxxxxxxx\r\n
//
//	INOFF1 = xxxxxxxx\r\n scene 1 OFF part
//	result:
//	INOFF1 = xxxxxxxx\r\n
//
//	RESTART\r\n restart controller, takes 60 seconds before controller becomes responsive
//
//	Wifi ESP8266 Relay Module 10A Network Relay
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "RelayNet.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include <sys/socket.h>
#include <sstream>

extern CSQLHelper m_sql;

#define	RELAYNET_USE_HTTP 					false
#define RELAYNET_POLL_INPUTS				true
#define RELAYNET_POLL_RELAYS		 		true
#define RELAYNET_MIN_POLL_INTERVAL 			1
#define MAX_TCP_IDLE_SECONDS				10

RelayNet::RelayNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password, const bool pollInputs, const bool pollRelays, const int pollInterval, const int inputCount, const int relayCount) :
m_szIPAddress(IPAddress),
m_username(CURLEncode::URLEncode(username)),
m_password(CURLEncode::URLEncode(password))
{
	m_bOutputLog 		= false;
	m_username			= username;
	m_password			= password;
	m_HwdID				= ID;
	m_usIPPort			= usIPPort;
	m_stoprequested		= false;
	m_poll_inputs		= pollInputs;
	m_poll_relays		= pollRelays;
	m_input_count 		= inputCount;
	m_relay_count		= relayCount;
	m_poll_interval		= pollInterval;
	m_keep_connection	= (pollInterval < MAX_TCP_IDLE_SECONDS);
	m_connected			= false;

	if (inputCount == 0)
	{
		m_poll_inputs = false;
	}

	if (relayCount == 0)
	{
		m_poll_relays = false;
	}

	if (pollInterval < RELAYNET_MIN_POLL_INTERVAL)
	{
		m_poll_interval = RELAYNET_MIN_POLL_INTERVAL;
	}
}

RelayNet::~RelayNet(void)
{
}

void RelayNet::Init()
{
	BYTE	id1 = 0x03;
	BYTE	id2 = 0x0E;
	BYTE 	id3 = 0x0E;
	BYTE 	id4 = m_HwdID & 0xFF;

	/* 	Prepare packet for LIGHTING2 relay status packet  */
	memset(&Packet, 0, sizeof(RBUF));

	if (m_HwdID > 0xFF)
	{
		id3 = (m_HwdID >> 8) && 0xFF;
	}

	if (m_HwdID > 0xFFFF)
	{
		id3 = (m_HwdID >> 16) && 0xFF;
	}

	Packet.LIGHTING2.packetlength	= sizeof(Packet.LIGHTING2) - 1;
	Packet.LIGHTING2.packettype 	= pTypeLighting2;
	Packet.LIGHTING2.subtype 		= sTypeAC;
	Packet.LIGHTING2.id1 			= id1;
	Packet.LIGHTING2.id2 			= id2;
	Packet.LIGHTING2.id3 			= id3;
	Packet.LIGHTING2.id4 			= id4;
	Packet.LIGHTING2.unitcode 		= 0;
	Packet.LIGHTING2.cmnd 			= 0;
	Packet.LIGHTING2.level 			= 0;
	Packet.LIGHTING2.filler 		= 0;
	Packet.LIGHTING2.rssi 			= 12;

	SetupDevices();
}

bool RelayNet::StartHardware()
{
	/* Check if we can connect to relay module */
	if (RelaycardTcpConnect())
	{
		RelaycardTcpDisconnect();
	}
	else
	{
		_log.Log(LOG_STATUS, "RelayNet: Can not connect to %s", m_szIPAddress.c_str());
		return false;
	}

	Init();

	if (m_poll_inputs || m_poll_relays)
	{
		/* Start worker thread */
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RelayNet::Do_Work, this)));
		m_bIsStarted=true;
		_log.Log(LOG_STATUS, "RelayNet: %s %dsec Relaycard poller started", m_szIPAddress.c_str(), m_poll_interval);
	}

	return true;
}

bool RelayNet::StopHardware()
{
	if (m_connected)
	{
		RelaycardTcpDisconnect();
		m_connected = false;
	}

	if (m_thread != NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}

	m_bIsStarted = false;

	return true;
}

void RelayNet::Do_Work()
{

	/* Relay card inputs poll mode */
	int 	sec_counter = m_poll_interval;
	char	dump[512];

	if ((m_poll_inputs || m_poll_relays))
	{
		while (!m_stoprequested)
		{
			sleep_seconds(1);
			sec_counter++;

			if (sec_counter % 12 == 0)
			{
				m_LastHeartbeat=mytime(NULL);
			}

			if (sec_counter % m_poll_interval == 0)
			{
				/* poll interval has expired, update relayboard status */
				memset(&dump[0], 0, sizeof(dump));

				if (!m_connected)
				{
					m_connected = RelaycardTcpConnect();
				}

				if (m_connected)
				{
					if (TcpGetRelaycardDump(&dump[0], sizeof(dump)))
					{
						ProcessRelaycardDump(&dump[0], sizeof(dump));
					}
					else
					{
						RelaycardTcpDisconnect();
						m_connected = false;
					}
				}

				if ((m_connected) && (!m_keep_connection))
				{
					RelaycardTcpDisconnect();
					m_connected = false;
				}
			}
		}
	}

	_log.Log(LOG_STATUS, "RelayNet: %s %dsec Relaycard poller stopped", m_szIPAddress.c_str(), m_poll_interval);
}

bool RelayNet::RelaycardTcpConnect()
{
	bool 	bOk = false;
	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 500000;

	memset(&m_socket, 0, sizeof(m_socket));
	m_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (m_socket)
	{
		setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
		setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

		m_addr.sin_family 		= AF_INET;
		m_addr.sin_addr.s_addr	= inet_addr(m_szIPAddress.c_str());;
		m_addr.sin_port 		= htons(m_usIPPort);

		if (connect(m_socket, (struct sockaddr*)&m_addr, sizeof(m_addr)) >= 0)
		{
			bOk = true;
		}
	}

	return bOk;
}

void RelayNet::RelaycardTcpDisconnect()
{
	closesocket(m_socket);
	sleep_milliseconds(10);	// time to let assync close socket finish
}

bool RelayNet::TcpGetSetRelay(int RelayNumber, bool SetRelay, bool State)
{
	int					portno = m_usIPPort;
	unsigned long 		ip;
	int					n;
	struct sockaddr_in 	serv_addr;
	char 				sndbuf[4];
	char				recbuf[16];
	char				expbuf[16];
	std::string			expectedResponse;
	std::string			receivedResponse;
	std::size_t 		found;
	bool				newState = State;

	/* Determine TCP command to be send */
	if (SetRelay)
	{
		if (State)
		{
			sndbuf[0] = 'L'; 	//	Turn relay on & get status
		}
		else
		{
			sndbuf[0] = 'D'; 	//	Turn relay off & get status
		}
	}
	else
	{
		sndbuf[0] = 'R'; 		//	Get relay status
	}

	sndbuf[1] = 0x30 + RelayNumber;
	sndbuf[2] = '\r';
	sndbuf[3] = '\n';

	/* Send message */
	n = write(m_socket, &sndbuf[0], sizeof(sndbuf));

	if (n < 0)
	{
		perror("ERROR writing to socket");
	}

	/* Receive response */
	memset(&recbuf, 0, sizeof(recbuf));
	n = read(m_socket, &recbuf[0], sizeof(recbuf));

	if (n < 0)
	{
		perror("ERROR reading from socket");
	}

	/* Check response */
	snprintf(&expbuf[0], sizeof(expbuf), State ? "Relayon %d" : "Relayoff %d", RelayNumber);
	expectedResponse = expbuf;
	receivedResponse = recbuf;

	if(receivedResponse.find(expectedResponse) == std::string::npos)
	{
		newState = !State;
	}

	return newState;
}

bool RelayNet::SetRelayState(int RelayNumber, bool State)
{
	bool state = false;

	state = TcpGetSetRelay(RelayNumber, true, State);

	return (state==State);
}

bool RelayNet::GetRelayState(int RelayNumber)
{
	return TcpGetSetRelay(RelayNumber, false, false);
}

void RelayNet::UpdateDomoticzRelayState(int RelayNumber, bool State)
{
	/* Send the Lighting 2 message to Domoticz */
	Packet.LIGHTING2.unitcode = RelayNumber;
	Packet.LIGHTING2.cmnd = State;
	sDecodeRXMessage(this, (const unsigned char *)&Packet.LIGHTING2, "Relay", 255);
}

void RelayNet::SetupDevices()
{
	bool bOk = true;
	char dump[512];

	if (RelaycardTcpConnect())
	{
		m_connected = true;

		std::vector<std::vector<std::string> > result;
		char szIdx[10];
		sprintf(szIdx, "%X%02X%02X%02X",
			Packet.LIGHTING2.id1,
			Packet.LIGHTING2.id2,
			Packet.LIGHTING2.id3,
			Packet.LIGHTING2.id4);

		for (int relayNumber = 1; relayNumber <= m_relay_count; relayNumber++)
		{
			/* Query Domoticz database to lookup device */
			result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, relayNumber);

			if (result.empty())
			{
				_log.Log(LOG_STATUS, "RelayNet: Create %s/Relay%i", m_szIPAddress.c_str(), relayNumber);
			}

			UpdateDomoticzRelayState(relayNumber, RelayNet::GetRelayState(relayNumber) ? light2_sOn : light2_sOff);
		}

		if (m_poll_inputs)
		{
			for (int inputNumber = 1; inputNumber <= m_input_count; inputNumber++)
			{
				/* Query Domoticz database to lookup device */
				result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, 100+inputNumber);

				if (result.empty())
				{
					_log.Log(LOG_STATUS, "RelayNet: Create %s/Input%i", m_szIPAddress.c_str(), inputNumber);
				}
			}

			if (TcpGetRelaycardDump(&dump[0], sizeof(dump)))
			{
				ProcessRelaycardDump(&dump[0], sizeof(dump));
			}
		}

		RelaycardTcpDisconnect();
		m_connected = false;
	}
}

bool RelayNet::TcpGetRelaycardDump(char* Buffer, int Length)
{
	int					portno = m_usIPPort;
	unsigned long 		ip;
	int					n;
	struct sockaddr_in 	serv_addr;
	char 				sndbuf[6];
	bool				bOk = true;

	if (NULL == Buffer)
	{
		bOk = false;
	}

	/* Send request */
	if (bOk)
	{
		memcpy(&sndbuf[0], "DUMP\r\n", sizeof(sndbuf));
		n = write(m_socket, &sndbuf[0], sizeof(sndbuf));

		if (n < 0)
		{
			bOk = false;
		}
	}

	/* Receive response */
	if (bOk)
	{
		memset(Buffer, 0, Length);
		n = read(m_socket, Buffer, Length);

		if (n < 0)
		{
			bOk = false;
		}
	}

	return bOk;
}

bool RelayNet::WriteToHardwareTcp(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype == pTypeLighting2)
	{
		int relay = pSen->LIGHTING2.unitcode;

		if ((relay >= 1) && (relay <= m_relay_count))
		{
			if (pSen->LIGHTING2.cmnd == light2_sOff)
			{
				UpdateDomoticzRelayState(relay, SetRelayState(relay, false));
			}
			else
			{
				UpdateDomoticzRelayState(relay, SetRelayState(relay, true));
			}
		}
	}

	return true;
}

bool RelayNet::WriteToHardwareHttp(const char *pdata, const unsigned char length)
{
	//-----------------------------------------------------------------------
	//
	//	Notes:
	//
	//	Use opensll to manually create a base64 authorization string.
	//
	//	Example:
	//  echo -n "admin:ok" | openssl base64 -base64
	//	Output generated: WRtaW46b2s=
	//
	//	To use the string:
	//	std::string sAccessToken = "YWRtaW46b2s=";
	//
	//-----------------------------------------------------------------------

	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype == pTypeLighting2)
	{
		std::stringstream 			sRelayCommand;
		std::stringstream 			sLogin;
		std::string 				sAccessToken;
		std::stringstream			sURL;
		std::stringstream 			szPostData;
		std::vector<std::string>	ExtraHeaders;
		std::string 				sResult;
		int 						relay = pSen->LIGHTING2.unitcode;

		if ((relay >= 1) && (relay <= m_relay_count))
		{
			if (pSen->LIGHTING2.cmnd == light2_sOff)
			{
				sRelayCommand << "saida" << relay << "off=off";
			}
			else
			{
				sRelayCommand << "saida" << relay << "on=on";
			}

			/*	Setup URL, relay-command and username/password */
			std::string sPostData = sRelayCommand.str();
			sURL << "http://" << m_szIPAddress << "/relay_en.cgi";
			sLogin << m_username << ":" << m_password;

			/* Generate UnEncrypted base64 Basic Authorization for username/password and add result to ExtraHeaders */
			sAccessToken = base64_encode((const unsigned char *)(sLogin.str().c_str()), strlen(sLogin.str().c_str() ) );
			ExtraHeaders.push_back("Authorization: Basic " + sAccessToken);

			/* Send URL to relay module and check return status */
			if (!HTTPClient::POST(sURL.str(), sPostData, ExtraHeaders, sResult) )
			{
				_log.Log(LOG_ERROR, "RelayNet: [1] Error sending relay command to: %s", m_szIPAddress.c_str());
				return false;
			}

			/* Look for "saida" in response, if present all should be ok */
			if (sResult.find("saida") == std::string::npos)
			{
				_log.Log(LOG_ERROR, "RelayNet: [2] Error sending relay command to: %s", m_szIPAddress.c_str());
				return false;
			}

			return true;
		}
	}

	return false;
}

bool RelayNet::WriteToHardware(const char *pdata, const unsigned char length)
{
	bool bOk = true;

#if RELAYNET_USE_HTTP

	bOk = WriteToHardwareHttp(pdata, length);

#else

	bOk = WriteToHardwareTcp(pdata, length);

#endif

	return bOk;
}

bool RelayNet::UpdateDomoticzInput(int InputNumber, bool State)
{
	bool	bOk = true;

	if (State)
	{
		Packet.LIGHTING2.cmnd = light1_sOn;
	}
	else
	{
		Packet.LIGHTING2.cmnd = light1_sOff;
	}
	Packet.LIGHTING2.unitcode = 100 + InputNumber;
	Packet.LIGHTING2.seqnbr++;

	/* send packet to Domoticz */
	sDecodeRXMessage(this, (const unsigned char *)&Packet.LIGHTING2, "Input", 255);

	return bOk;
}

bool RelayNet::UpdateDomoticzOutput(int OutputNumber, bool State)
{
	bool	bOk = true;

	if (State)
	{
		Packet.LIGHTING2.cmnd = light1_sOn;
	}
	else
	{
		Packet.LIGHTING2.cmnd = light1_sOff;
	}
	Packet.LIGHTING2.unitcode = OutputNumber;
	Packet.LIGHTING2.seqnbr++;

	/* send packet to Domoticz */
	sDecodeRXMessage(this, (const unsigned char *)&Packet.LIGHTING2, "Output", 255);

	return bOk;
}


bool RelayNet::ProcessRelaycardDump(char* Dump, int Length)
{
	bool	bOk = true;
	bool	relaycardInput[m_relay_count];
	char	cTemp[16];
	std::string sDump;
	std::string sChkstr;

	sDump = Dump;

	if (m_poll_inputs)
	{
		for (int i=1; i <= m_input_count ; i++)
		{
			snprintf(&cTemp[0], sizeof(cTemp), "IH %d", i);
			sChkstr = cTemp;

			if(sDump.find(sChkstr) == std::string::npos)
			{
				UpdateDomoticzInput(i, false);
			}
			else
			{
				UpdateDomoticzInput(i, true);
			}
		}
	}

	if (m_poll_relays)
	{
		for (int i=1; i <= m_relay_count ; i++)
		{
			snprintf(&cTemp[0], sizeof(cTemp), "relayon %d", i);
			sChkstr = cTemp;

			if(sDump.find(sChkstr) == std::string::npos)
			{
				UpdateDomoticzOutput(i, false);
			}
			else
			{
				UpdateDomoticzOutput(i, true);
			}
		}
	}

	return bOk;
}
