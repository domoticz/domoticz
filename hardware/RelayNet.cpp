//---------------------------------------------------------------------------
//
//	Notes for RELAY-NET-V5.7 LAN 8 channel relay and binary input card.
//
//	Hard reset: Hold reset button and power until, after about 10 seconds, one
//				of the green LEDS starts to flash to hard reset the relay card
//
//	After Hard Reset:
//	IpAddress 	= 192.168.1.166
//	Username	= admin
//	Password	= 12345678
//
//	Connect to it from a browser, type HTTP://192.168.1.166 in the bar.
//	Make sure you computer has access to the subnet.
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
#include "hardwaretypes.h"
#include "ASyncTCP.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "../httpclient/HTTPClient.h"
#include <sstream>

//===========================================================================

#define	RELAYNET_USE_HTTP 					false
#define RELAYNET_POLL_INPUTS				true
#define RELAYNET_POLL_RELAYS		 		true
#define RELAYNET_MIN_POLL_INTERVAL 			1
#define MAX_INPUT_COUNT						64
#define MAX_RELAY_COUNT						64
#define KEEP_ALIVE_INTERVAL					10
#define RETRY_DELAY 						30

//===========================================================================

RelayNet::RelayNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password, const bool pollInputs, const bool pollRelays, const int pollInterval, const int inputCount, const int relayCount) :
m_szIPAddress(IPAddress),
m_username(CURLEncode::URLEncode(username)),
m_password(CURLEncode::URLEncode(password)),
m_stoprequested(false),
m_reconnect(false)
{
	m_stoprequested = false;
	m_setup_devices = true;
	m_bOutputLog = false;
	m_bDoRestart = false;
	m_bIsStarted = false;
	m_username = username;
	m_password = password;
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_poll_inputs = pollInputs;
	m_poll_relays = pollRelays;
	m_input_count = inputCount;
	m_relay_count = relayCount;
	m_poll_interval = pollInterval;
	m_skip_relay_update	= 0;
	m_retrycntr = 0;

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

//===========================================================================

RelayNet::~RelayNet(void)
{
}

//===========================================================================

bool RelayNet::StartHardware()
{
	bool bOk = false;;
	m_stoprequested = false;
	m_reconnect = false;
	m_bIsStarted = false;
	m_setup_devices = false;
	m_bIsStarted = false;
	m_stoprequested = false;
	m_bDoRestart = false;
	m_retrycntr = RETRY_DELAY; //force connect the next first time

	if (m_input_count || m_relay_count)
	{
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RelayNet::Do_Work, this)));
	}

	if (m_thread != NULL)
	{
		bOk = true;
		m_bIsStarted=true;
	}

	return bOk;
}

//===========================================================================

bool RelayNet::StopHardware()
{
	m_stoprequested = true;

	if (isConnected())
	{
		disconnect();
	}

	try 
	{
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
	}

	m_bIsStarted = false;

	_log.Log(LOG_STATUS, "RelayNet: Relay Module disconnected %s", m_szIPAddress.c_str());

	return true;
}

//===========================================================================

bool RelayNet::WriteToHardware(const char *pdata, const unsigned char length)
{
#if RELAYNET_USE_HTTP
	return WriteToHardwareHttp(pdata);
#else
	return WriteToHardwareTcp(pdata);
#endif
}

//===========================================================================

void RelayNet::Do_Work()
{
	bool bFirstTime = true;
	int sec_counter = 0;

	/*  Init  */
	Init();

	if (m_poll_inputs || m_poll_relays)
	{
		_log.Log(LOG_STATUS, "RelayNet: %d-second poller started (%s)", m_poll_interval, m_szIPAddress.c_str());
	}

	while (!m_stoprequested)
	{
		/*  One second sleep  */
		sleep_seconds(1);
		sec_counter++;

		/*  Heartbeat maintenance  */
		if (sec_counter  % 10 == 0)
		{
			m_LastHeartbeat = mytime(NULL);
		}

		/*  Connection maintenance  */
		if (bFirstTime)
		{
			bFirstTime = false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}

		/*  Prevent disconnect request by Relay Module  */
		if ((sec_counter % KEEP_ALIVE_INTERVAL == 0) &&
			((m_poll_interval > KEEP_ALIVE_INTERVAL) || (!m_poll_inputs && !m_poll_relays)))
		{
			KeepConnectionAlive();
		}

		/*  Update relay module status when poll interval has expired  */
		if ((m_poll_inputs || m_poll_relays) && (sec_counter % m_poll_interval == 0))
		{
			TcpRequestRelaycardDump();
		}
	}

	/*  Done  */
	if (m_poll_inputs || m_poll_relays)
	{
		_log.Log(LOG_STATUS, "RelayNet: %d-second poller stopped (%s)", m_poll_interval, m_szIPAddress.c_str());
	}
}

//===========================================================================

void RelayNet::Init()
{
	BYTE id1 = 0x03;
	BYTE id2 = 0x0E;
	BYTE id3 = 0x0E;
	BYTE id4 = m_HwdID & 0xFF;

	/* 	Prepare packet for LIGHTING2 relay status packet  */
	memset(&m_Packet, 0, sizeof(m_Packet));

	if (m_HwdID > 0xFF)
	{
		id3 = (m_HwdID >> 8) & 0xFF;
	}

	if (m_HwdID > 0xFFFF)
	{
		id3 = (m_HwdID >> 16) & 0xFF;
	}

	m_Packet.LIGHTING2.packetlength = sizeof(m_Packet.LIGHTING2) - 1;
	m_Packet.LIGHTING2.packettype = pTypeLighting2;
	m_Packet.LIGHTING2.subtype = sTypeAC;
	m_Packet.LIGHTING2.unitcode = 0;
	m_Packet.LIGHTING2.id1 = id1;
	m_Packet.LIGHTING2.id2 = id2;
	m_Packet.LIGHTING2.id3 = id3;
	m_Packet.LIGHTING2.id4 = id4;
	m_Packet.LIGHTING2.cmnd = 0;
	m_Packet.LIGHTING2.level = 0;
	m_Packet.LIGHTING2.filler = 0;
	m_Packet.LIGHTING2.rssi = 12;

	SetupDevices();
}

//===========================================================================

void RelayNet::SetupDevices()
{
	std::vector<std::vector<std::string> > result;
	char szIdx[10];

	sprintf(szIdx, "%X%02X%02X%02X",
		m_Packet.LIGHTING2.id1,
		m_Packet.LIGHTING2.id2,
		m_Packet.LIGHTING2.id3,
		m_Packet.LIGHTING2.id4);

	if (m_relay_count)
	{
		for (int relayNumber = 1; relayNumber <= m_relay_count; relayNumber++)
		{
			result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, relayNumber);

			if (result.empty())
			{
				_log.Log(LOG_STATUS, "RelayNet: Create %s/Relay%i", m_szIPAddress.c_str(), relayNumber);

				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
					"VALUES (%d, '%q', %d, %d, %d, %d, 0, 12, 255, '%q', 0, ' ')",
					m_HwdID, szIdx, relayNumber, pTypeLighting2, sTypeAC, int(STYPE_OnOff), "Relay");
			}
		}
	}

	if (m_input_count)
	{
		for (int inputNumber = 1; inputNumber <= m_input_count; inputNumber++)
		{
			result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, 100 + inputNumber);

			if (result.empty())
			{
				_log.Log(LOG_STATUS, "RelayNet: Create %s/Input%i", m_szIPAddress.c_str(), inputNumber);

				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
					"VALUES (%d,'%q',%d, %d, %d, %d, 0, 12, 255, '%q', 0, ' ')",
					m_HwdID, szIdx, 100+inputNumber, pTypeLighting2, sTypeAC, int(STYPE_Contact), "Input");
			}
		}
	}
}

//===========================================================================

void RelayNet::TcpRequestRelaycardDump()
{
	std::string	sRequest = "DUMP\r\n";

	if (isConnected())
	{
		write(sRequest);
	}
}

//===========================================================================

void RelayNet::KeepConnectionAlive()
{
	std::string	sRequest = "R1\r\n";

	if (isConnected())
	{
		write(sRequest);
	}
}

//===========================================================================

void RelayNet::TcpGetSetRelay(int RelayNumber, bool SetRelay, bool State)
{
	char sndbuf[4];

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

	sndbuf[1] = 0x30 + (char) RelayNumber;
	sndbuf[2] = '\r';
	sndbuf[3] = '\n';

	if (isConnected())
	{
		write((const unsigned char*)&sndbuf[0], (size_t) sizeof(sndbuf));
	}
}

void RelayNet::SetRelayState(int RelayNumber, bool State)
{
	TcpGetSetRelay(RelayNumber, true, State);

	if (m_poll_relays)
	{
		m_skip_relay_update++;
	}
}

//===========================================================================

bool RelayNet::WriteToHardwareTcp(const char *pdata)
{
	bool bOk = true;

	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype == pTypeLighting2)
	{
		int relay = pSen->LIGHTING2.unitcode;

		if ((relay >= 1) && (relay <= m_relay_count))
		{
			if (pSen->LIGHTING2.cmnd == light2_sOn)
			{
				SetRelayState(relay, true);
			}
			else
			{
				SetRelayState(relay, false);
			}
		}
		else
		{
			bOk = false;
		}
	}

	return bOk;
}

//===========================================================================

void RelayNet::UpdateDomoticzInput(int InputNumber, bool State)
{
	bool updateDatabase = false;
	std::vector<std::vector<std::string> > result;
	char szIdx[10];

	sprintf(szIdx, "%X%02X%02X%02X",
		m_Packet.LIGHTING2.id1,
		m_Packet.LIGHTING2.id2,
		m_Packet.LIGHTING2.id3,
		m_Packet.LIGHTING2.id4);

	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, 100 + InputNumber);

	if ((!result.empty()) && (result.size()>0))
	{
		std::vector<std::string> sd=result[0];
		bool dbState = true;

		if (atoi(sd[1].c_str()) == 0)
		{
			dbState = false;
		}

		if (dbState != State)
		{
			updateDatabase = true;
		}
	}

	if (updateDatabase)
	{
		if (State)
		{
			m_Packet.LIGHTING2.cmnd = light2_sOn;
			m_Packet.LIGHTING2.level = 100;
		}
		else
		{
			m_Packet.LIGHTING2.cmnd = light2_sOff;
			m_Packet.LIGHTING2.level = 0;
		}
		m_Packet.LIGHTING2.unitcode = 100 + (char) InputNumber;
		m_Packet.LIGHTING2.seqnbr++;

		/* send packet to Domoticz */
		sDecodeRXMessage(this, (const unsigned char *)&m_Packet.LIGHTING2, "Input", 255);
	}
}

//===========================================================================

void RelayNet::UpdateDomoticzRelay(int RelayNumber, bool State)
{
	bool updateDatabase = false;
	std::vector<std::vector<std::string> > result;
	char szIdx[10];

	sprintf(szIdx, "%X%02X%02X%02X",
		m_Packet.LIGHTING2.id1,
		m_Packet.LIGHTING2.id2,
		m_Packet.LIGHTING2.id3,
		m_Packet.LIGHTING2.id4);

	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szIdx, RelayNumber);

	if ((!result.empty()) && (result.size()>0))
	{
		std::vector<std::string> sd = result[0];
		bool dbState = true;

		if (atoi(sd[1].c_str()) == 0)
		{
			dbState = false;
		}

		if (dbState != State)
		{
			updateDatabase = true;
		}
	}

	if (updateDatabase)
	{
		if (State)
		{
			m_Packet.LIGHTING2.cmnd = light2_sOn;
			m_Packet.LIGHTING2.level = 100;
		}
		else
		{
			m_Packet.LIGHTING2.cmnd = light2_sOff;
			m_Packet.LIGHTING2.level = 0;
		}
		m_Packet.LIGHTING2.unitcode = (char) RelayNumber;
		m_Packet.LIGHTING2.seqnbr++;

		/* send packet to Domoticz */
		sDecodeRXMessage(this, (const unsigned char *)&m_Packet.LIGHTING2, "Relay", 255);
	}
}

//===========================================================================

void RelayNet::ProcessRelaycardDump(char* Dump)
{
	char	cTemp[16];
	std::string sDump;
	std::string sChkstr;

	sDump = Dump;
	boost::to_upper(sDump);

	if (!m_skip_relay_update && m_relay_count && (m_poll_relays || m_setup_devices))
	{
		for (int i=1; i <= m_relay_count ; i++)
		{
			snprintf(&cTemp[0], sizeof(cTemp), "RELAYON %d", i);
			sChkstr = cTemp;

			if(sDump.find(sChkstr) != std::string::npos)
			{
				UpdateDomoticzRelay(i, true);
			}

			snprintf(&cTemp[0], sizeof(cTemp), "RELAYOFF %d", i);
			sChkstr = cTemp;

			if (sDump.find(sChkstr) != std::string::npos)
			{
				UpdateDomoticzRelay(i, false);
			}
		}
	}

	if (m_input_count && (m_poll_inputs || m_setup_devices))
	{
		for (int i = 1; i <= m_input_count; i++)
		{
			snprintf(&cTemp[0], sizeof(cTemp), "IH %d", i);
			sChkstr = cTemp;

			if (sDump.find(sChkstr) != std::string::npos)
			{
				UpdateDomoticzInput(i, true);
			}

			snprintf(&cTemp[0], sizeof(cTemp), "IL %d", i);
			sChkstr = cTemp;

			if (sDump.find(sChkstr) != std::string::npos)
			{
				UpdateDomoticzInput(i, false);
			}
		}
	}

	/* housekeeping */
	m_setup_devices = false;

	if (m_skip_relay_update)
	{
		m_skip_relay_update--;
	}
}

//===========================================================================

void RelayNet::ParseData(const unsigned char *pData, int Len)
{
	char relayCardDump[512];

	if ((Len > 64) && (Len <= sizeof(relayCardDump)))
	{
		/* Its a RelayCard dump message */
		memset(&relayCardDump[0], 0, Len);
		memcpy(&relayCardDump[0], pData, Len);
		ProcessRelaycardDump(&relayCardDump[0]);
	}
}

//===========================================================================
//
//	Alternate way of turning relays on/off using HTTP. 
//	Currently not used. 
//
bool RelayNet::WriteToHardwareHttp(const char *pdata)
{
	//-----------------------------------------------------------------------
	//
	//	Notes:
	//
	//	Use OpenSSL to manually create a base64 authorization string.
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
			sAccessToken = base64_encode((const unsigned char *)(sLogin.str().c_str()), strlen(sLogin.str().c_str()));
			ExtraHeaders.push_back("Authorization: Basic " + sAccessToken);

			/* Send URL to relay module and check return status */
			if (!HTTPClient::POST(sURL.str(), sPostData, ExtraHeaders, sResult))
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

//===========================================================================
//
//							ASyncTCP support
//
void RelayNet::OnConnect()
{
	_log.Log(LOG_STATUS, "RelayNet: Connected to Relay Module %s", m_szIPAddress.c_str());
	m_reconnect = false;
	m_bIsStarted = true;
}

//===========================================================================

void RelayNet::OnDisconnect()
{
	_log.Log(LOG_STATUS, "RelayNet: Relay Module disconnected %s, reconnect", m_szIPAddress.c_str());

	if (!m_stoprequested)
	{
		m_reconnect = true;
	}
}

//===========================================================================

void RelayNet::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_stoprequested)
	{
		ParseData(pData, length);
	}
}

//===========================================================================

void RelayNet::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "RelayNet: Error: %s", e.what());
}

//===========================================================================

void RelayNet::OnError(const boost::system::error_code& error)
{
	if ((error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out))
	{
		_log.Log(LOG_ERROR, "RelayNet: OnError: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if ((error == boost::asio::error::eof) || (error == boost::asio::error::connection_reset))
	{
		_log.Log(LOG_STATUS, "RelayNet: OnError: Connection reset!");
	}
	else
	{
		_log.Log(LOG_ERROR, "RelayNet: OnError: %s", error.message().c_str());
	}
}

//===========================================================================
