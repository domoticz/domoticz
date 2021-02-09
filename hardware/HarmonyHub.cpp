/******************************************************************************
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========
Original source code from: http://sourceforge.net/projects/harmonyhubcontrol/
Integration in Domoticz done by: Jan ten Hove
Cleanup and changes: GizMoCuz
History:
 16 August 2016:
 - Fixed: Logitech Harmony, Ping request now working as well for firmware 4.10.30 (Herman)
 19 November 2016:
 - Removed: Need to login remotely with username/password
 11 April 2018:
 - Refactored: address several issues with working with firmware 4.14.123 (gordonb3)
 19 June 2018:
 - Complete overhaul: Abandon original methods from harmonyhubcontrol (gordonb3)
    =>	use a centralized (polled) socket reader function rather than wait for a direct
	response to a socket write operation.
 25 June 2018:
 - Make debug lines available to the logging system
*/

//#define _DEBUG


#include "stdafx.h"
#include "HarmonyHub.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../webserver/Base64.h"
#include "csocket.h"
#include "../main/json_helper.h"


#define CONNECTION_ID				"446f6d6f-7469-637a-2d48-61726d6f6e79" // `Domoticz-Harmony` in ASCII
#define GET_CONFIG_COMMAND			"get_config"
#define START_ACTIVITY_COMMAND			"start_activity"
#define GET_CURRENT_ACTIVITY_COMMAND		"get_current_activity_id"
#define HARMONY_PING_INTERVAL_SECONDS		30	// must be smaller than 40 seconds or Hub will silently hang up on us
#define HARMONY_SEND_ACK_SECONDS		10	// must be smaller than 20 seconds...
#define HARMONY_PING_RETRY_SECONDS		 5
#define HARMONY_RETRY_LOGIN_SECONDS		60
#define HEARTBEAT_SECONDS			12


// Note:
// HarmonyHub is on Wifi and can thus send frames with a maximum payload length of 2324 bytes
// Normal implementations will however obey the 1500 bytes MTU from the wired networks that
// they are attached to and this may be limited even further if the router uses mechanisms like
// PPTP for connecting the (Wireless) LAN to the internet.
#define SOCKET_BUFFER_SIZE  1500


#ifdef _DEBUG
	#include <iostream>
#endif

CHarmonyHub::CHarmonyHub(const int ID, const std::string &IPAddress, const unsigned int port):
m_szHarmonyAddress(IPAddress)
{
	m_usHarmonyPort = (unsigned short)port;
	m_HwdID=ID;
	Init();
}

CHarmonyHub::~CHarmonyHub()
{
	StopHardware();
}

void CHarmonyHub::Init()
{
	m_connection = nullptr;
	m_connectionstatus = DISCONNECTED;
	m_bNeedMoreData = false;
	m_bWantAnswer = false;
	m_bNeedEcho = false;
	m_bReceivedMessage = false;
	m_bIsChangingActivity = false;
	m_bShowConnectError = true;
	m_szCurActivityID = "";
	m_szHubSwVersion = "";
	m_szAuthorizationToken = "";
	m_mapActivities.clear();
}


void CHarmonyHub::Logout()
{
	if (m_connectionstatus != DISCONNECTED)
	{
#ifdef _DEBUG
		std::cerr << "logout requested: close stream\n";
#endif
		CloseStream(m_connection);
		int i = 50;
		while ((i > 0) && (m_connectionstatus != DISCONNECTED))
		{
			sleep_milliseconds(40);
			CheckForHarmonyData();
			i--;
		}
		if (m_connectionstatus != DISCONNECTED)
		{
			// something went wrong
#ifdef _DEBUG
			std::cerr << "stream did not sucessfully close: killing TCP socket\n";
#endif
			ResetCommunicationSocket();
		}
	}
	Init();
}


bool CHarmonyHub::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}


bool CHarmonyHub::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	m_bIsChangingActivity = false;
	m_szHubSwVersion = "";
	return true;
}


bool CHarmonyHub::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF*>(pdata);

	if (this->m_bIsChangingActivity)
	{
		_log.Log(LOG_ERROR, "Harmony Hub: Command cannot be sent. Hub is changing activity");
		return false;
	}

	if (this->m_connectionstatus == DISCONNECTED)
	{
		if (this->m_szAuthorizationToken.empty() &&
		     (pCmd->LIGHTING2.id1 == 0xFF) && (pCmd->LIGHTING2.id2 == 0xFF) &&
		     (pCmd->LIGHTING2.id3 == 0xFF) && (pCmd->LIGHTING2.id4 == 0xFF) &&
		     (pCmd->LIGHTING2.cmnd == 0)
		)
		{
			// "secret" undefined state request to silence connection error reporting
			if (this->m_bShowConnectError)
				_log.Log(LOG_STATUS, "Harmony Hub: disable connection error logging");
			this->m_bShowConnectError = false;
			return false;
		}

		_log.Log(LOG_STATUS, "Harmony Hub: Received a switch command but we are not connected - attempting connect now");
		this->m_bLoginNow = true;
		int retrycount = 0;
		while ( (retrycount < 10) && (!IsStopRequested(500)) )
		{
			// give worker thread up to 5 seconds time to establish the connection
			if ((this->m_connectionstatus == BOUND) && (!m_szCurActivityID.empty()))
				break;
			retrycount++;
		}
		if (IsStopRequested(0))
			return true;

		if (this->m_connectionstatus == DISCONNECTED)
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Connect failed: cannot send the switch command");
			return false;
		}
	}

	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		int lookUpId = (int)(pCmd->LIGHTING2.id1 << 24) |  (int)(pCmd->LIGHTING2.id2 << 16) | (int)(pCmd->LIGHTING2.id3 << 8) | (int)(pCmd->LIGHTING2.id4) ;
		std::string realID = std::to_string(lookUpId);

		if (pCmd->LIGHTING2.cmnd == 0)
		{
			if (m_szCurActivityID != realID)
			{
				return false;
			}
			if (realID == "-1") // powering off the PowerOff activity leads to an undefined state in the frontend
			{
				// send it anyway, user may be trying to correct a wrong state of the Hub
				SubmitCommand(START_ACTIVITY_COMMAND, "-1");
				// but don't allow the frontend to update the button state to the off position
				return false;
			}
			if (SubmitCommand(START_ACTIVITY_COMMAND, "-1") <= 0)
			{
				_log.Log(LOG_ERROR, "Harmony Hub: Error sending the power-off command");
				return false;
			}
		}
		else if (SubmitCommand(START_ACTIVITY_COMMAND, realID) <= 0)
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Error sending the switch command");
			return false;
		}
	}
	return true;
}


void CHarmonyHub::Do_Work()
{
	_log.Log(LOG_STATUS,"Harmony Hub: Worker thread started...");

	unsigned int pcounter = 0;		// ping interval counter
	unsigned int tcounter = 0;		// 1/25 seconds

	char lcounter = 0;			// login counter
	char fcounter = 0;			// failed login attempts
	char hcounter = HEARTBEAT_SECONDS;	// heartbeat interval counter


	m_bLoginNow = true;

	while (!IsStopRequested(0))
	{
		if (m_connectionstatus == BOUND)
		{
			if (!m_bWantAnswer)
			{
				if (m_szAuthorizationToken.empty())
				{
					// we are not yet authenticated -> send pair request
					SendPairRequest(m_connection);
					m_bShowConnectError = false;
#ifdef _DEBUG
					std::cerr << "disable show connect error\n";
#endif
				}
				else
				{
					if (m_mapActivities.empty())
					{
						// instruct Hub to send us its config so we can retrieve the list of activities
						SubmitCommand(GET_CONFIG_COMMAND);
					}
					else if (m_szCurActivityID.empty())
					{
						fcounter = 0;
						_log.Log(LOG_STATUS, "Harmony Hub: Connected to Hub.");
						SubmitCommand(GET_CURRENT_ACTIVITY_COMMAND);
					}
				}
			}

			if (!tcounter) // slot this to full seconds only to prevent racing
			{
				// Hub requires us to actively keep our connection alive by periodically pinging it
				if ((pcounter % HARMONY_PING_INTERVAL_SECONDS) == 0)
				{
#ifdef _DEBUG
					std::cerr << "send ping\n";
#endif
					if (m_bNeedEcho || SendPing() < 0)
					{
						// Hub dropped our connection
						_log.Log(LOG_ERROR, "Harmony Hub: Error pinging server.. Resetting connection.");
						ResetCommunicationSocket();
						pcounter = HARMONY_RETRY_LOGIN_SECONDS - 5; // wait 5 seconds before attempting login again
					}
				}

				else if (m_bNeedEcho && ((pcounter % HARMONY_PING_RETRY_SECONDS) == 0))
				{
					// timeout
#ifdef _DEBUG
					std::cerr << "retry ping\n";
#endif
					if (SendPing() < 0)
					{
						// Hub dropped our connection
						_log.Log(LOG_ERROR, "Harmony Hub: Error pinging server.. Resetting connection.");
						ResetCommunicationSocket();
						pcounter = HARMONY_RETRY_LOGIN_SECONDS - 5; // wait 5 seconds before attempting login again
					}
				}
			}
		}
		else if (m_connectionstatus == DISCONNECTED)
		{
			if (!tcounter) // slot this to full seconds only to prevent racing
			{
				if (!m_bLoginNow)
				{
					if ((pcounter % HARMONY_RETRY_LOGIN_SECONDS) == 0)
					{
						lcounter++;
						if (lcounter > fcounter)
						{
							m_bLoginNow = true;
							if (fcounter > 0)
								_log.Log(LOG_NORM, "Harmony Hub: Reattempt login.");
						}
					}
				}
				if (m_bLoginNow)
				{
					m_bLoginNow = false;
					m_bWantAnswer = false;
					m_bNeedEcho = false;
					if (fcounter < 5)
						fcounter++;
					lcounter = 0;
					pcounter = 0;
					m_szCurActivityID = "";
					if (!SetupCommunicationSocket())
					{
						if ((m_connectionstatus == DISCONNECTED) && (hcounter > 3))
						{
							// adjust heartbeat counter for connect timeout
							hcounter -= 3;
						}
					}
				}
			}
		}
		else
		{
			// m_connectionstatus == CONNECTED || INITIALIZED || AUTHENTICATED
			if (!tcounter)
			{
				if ((pcounter % HARMONY_RETRY_LOGIN_SECONDS) > 1)
				{
					// timeout
					_log.Log(LOG_ERROR, "Harmony Hub: setup command socket timed out");
					ResetCommunicationSocket();
				}
			}
		}

		// check socket ready and if it is then read Harmony data
		CheckForHarmonyData();

		if (m_bReceivedMessage)
		{
			if (pcounter < (HARMONY_PING_INTERVAL_SECONDS - HARMONY_SEND_ACK_SECONDS))
			{
				// fast forward our ping counter
				pcounter = HARMONY_PING_INTERVAL_SECONDS - HARMONY_SEND_ACK_SECONDS;
#ifdef _DEBUG
				std::cerr << "fast forward ping counter to " << pcounter << " seconds\n";
#endif
			}
			m_bReceivedMessage = false;
		}

		if (m_bWantAnswer || tcounter)
		{
			// use a 40ms poll interval
			sleep_milliseconds(40);
			tcounter++;
			if ((tcounter % 25) == 0)
			{
				tcounter = 0;
				pcounter++;
				hcounter--;
			}
		}
		else
		{
			// use a 1s poll interval
			if (IsStopRequested(1000))
				break;
			pcounter++;
			hcounter--;
		}

		if (!hcounter)
		{
			// update heartbeat
			hcounter = HEARTBEAT_SECONDS;
			m_LastHeartbeat = mytime(nullptr);
		}
	}
	Logout();

	_log.Log(LOG_STATUS,"Harmony Hub: Worker stopped...");
}


/************************************************************************
*									*
* Update a single activity switch if exist				*
*									*
************************************************************************/
void CHarmonyHub::CheckSetActivity(const std::string &activityID, const bool on)
{
	// get the device id from the db (if already inserted)
	int actId=atoi(activityID.c_str());
	std::stringstream hexId ;
	hexId << std::setw(7)  << std::hex << std::setfill('0') << std::uppercase << (int)( actId) ;
	std::string actHex = hexId.str();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, actHex.c_str());
	if (!result.empty())
	{
		UpdateSwitch((uint8_t)(atoi(result[0][1].c_str())), activityID.c_str(),on,result[0][0]);
	}
}


/************************************************************************
*									*
* Insert/Update a single activity switch (unconditional)		*
*									*
************************************************************************/
void CHarmonyHub::UpdateSwitch(unsigned char /*idx*/, const char *realID, const bool bOn, const std::string &defaultname)
{
	std::stringstream hexId ;
	hexId << std::setw(7) << std::setfill('0') << std::hex << std::uppercase << (int)( atoi(realID) );
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, hexId.str().c_str());
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == light2_sOff))
			return;
		if ((bOn && (nvalue != light2_sOff)))
			return;
	}
	int i_Id = atoi( realID);
	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;

	lcmd.LIGHTING2.id1 = (i_Id>> 24) & 0xFF;
	lcmd.LIGHTING2.id2 = (i_Id>> 16) & 0xFF;
	lcmd.LIGHTING2.id3 = (i_Id>> 8) & 0xFF;
	lcmd.LIGHTING2.id4 = (i_Id) & 0xFF;
	lcmd.LIGHTING2.unitcode = 1;
	uint8_t level = 15;
	if (!bOn)
	{
		level = 0;
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		level = 15;
		lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}


/************************************************************************
*									*
* Raw socket functions							*
*									*
************************************************************************/
bool CHarmonyHub::ConnectToHarmony(const std::string &szHarmonyIPAddress, const int HarmonyPortNumber, csocket* connection)
{
	if (szHarmonyIPAddress.length() == 0 || HarmonyPortNumber == 0 || HarmonyPortNumber > 65535)
		return false;

	connection->connect(szHarmonyIPAddress.c_str(), HarmonyPortNumber);

	return (connection->getState() == csocket::CONNECTED);
}


bool CHarmonyHub::SetupCommunicationSocket()
{
	if (m_connection)
		ResetCommunicationSocket();

	m_connection = new csocket();

	if(!ConnectToHarmony(m_szHarmonyAddress, m_usHarmonyPort, m_connection))
	{
		if (m_bShowConnectError)
			_log.Log(LOG_ERROR,"Harmony Hub: Cannot connect to Harmony Hub. Check IP/Port.");
		return false;
	}
	m_connectionstatus = CONNECTED;
	int ret = StartStream(m_connection);
	return (ret >= 0);
}


void CHarmonyHub::ResetCommunicationSocket()
{
	delete m_connection;
	m_connection = nullptr;
	m_connectionstatus = DISCONNECTED;

	m_bIsChangingActivity = false;
	m_bWantAnswer = false;
	m_szCurActivityID = "";
}


int CHarmonyHub::WriteToSocket(std::string *szReq)
{
	int ret =  m_connection->write(szReq->c_str(), static_cast<unsigned int>(szReq->length()));
	if (ret > 0)
		m_bWantAnswer = true;
	return ret;
}


std::string CHarmonyHub::ReadFromSocket(csocket *connection)
{
	return ReadFromSocket(connection, 0);
}
std::string CHarmonyHub::ReadFromSocket(csocket *connection, float waitTime)
{
	if (connection == nullptr)
	{
		return "</stream:stream>";
	}

	std::string szData;

	bool bIsDataReadable = true;
	connection->canRead(&bIsDataReadable, waitTime);
	if (bIsDataReadable)
	{
		char databuffer[SOCKET_BUFFER_SIZE];
		int bytesReceived = connection->read(databuffer, SOCKET_BUFFER_SIZE, false);
		if (bytesReceived > 0)
		{
			szData = std::string(databuffer, bytesReceived);
		}
	}
	return szData;
}


/************************************************************************
*									*
* XMPP Stream initialization						*
*									*
************************************************************************/
int CHarmonyHub::StartStream(csocket *connection)
{
	if (connection == nullptr)
		return -1;
	std::string szReq = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	return WriteToSocket(&szReq);
}


int CHarmonyHub::SendAuth(csocket *connection, const std::string &szUserName, const std::string &szPassword)
{
	if (connection == nullptr)
		return -1;
	std::string szAuth = R"(<auth xmlns="urn:ietf:params:xml:ns:xmpp-sasl" mechanism="PLAIN">)";
	std::string szCred = "\0";
	szCred.append(szUserName);
	szCred.append("\0");
	szCred.append(szPassword);
	szAuth.append(base64_encode(szCred));
	szAuth.append("</auth>");
	return WriteToSocket(&szAuth);
}


int CHarmonyHub::SendPairRequest(csocket *connection)
{
	if (connection == nullptr)
		return -1;
	std::string szReq = R"(<iq type="get" id=")";
	szReq.append(CONNECTION_ID);
	szReq.append(R"("><oa xmlns="connect.logitech.com" mime="vnd.logitech.connect/vnd.logitech.pair">method=pair)");
	szReq.append(":name=foo#iOS6.0.1#iPhone</oa></iq>");
	return WriteToSocket(&szReq);
}


int CHarmonyHub::CloseStream(csocket *connection)
{
	if (connection == nullptr)
		return -1;
	std::string szReq = "</stream:stream>";
	return WriteToSocket(&szReq);
}


/************************************************************************
*									*
* Ping function								*
*									*
************************************************************************/
int CHarmonyHub::SendPing()
{
	if (m_connection == nullptr || m_szAuthorizationToken.length() == 0)
		return -1;

	std::string szReq = R"(<iq type="get" id=")";
	szReq.append(CONNECTION_ID);
	szReq.append(R"("><oa xmlns="connect.logitech.com" mime="vnd.logitech.connect/vnd.logitech.ping">token=)");
	szReq.append(m_szAuthorizationToken);
	szReq.append(":name=foo#iOS6.0.1#iPhone</oa></iq>");

	m_bNeedEcho = true;
	return WriteToSocket(&szReq);
}


/************************************************************************
*									*
* Submit command function						*
*									*
************************************************************************/
int CHarmonyHub::SubmitCommand(const std::string &szCommand)
{
	return SubmitCommand(szCommand, "");
}
int CHarmonyHub::SubmitCommand(const std::string &szCommand, const std::string &szActivityId)
{
	if (m_connection == nullptr || m_szAuthorizationToken.empty())
	{
		// errorString = "SubmitCommand : nullptr csocket or empty authorization token provided";
		return false;
	}

	std::string szReq = R"(<iq type="get" id=")";
	szReq.append(CONNECTION_ID);
	szReq.append(R"("><oa xmlns="connect.logitech.com" mime="vnd.logitech.harmony/vnd.logitech.harmony.engine?)");

	// Issue the provided command
	if (szCommand == GET_CONFIG_COMMAND)
	{
		szReq.append("config\"></oa></iq>");
	}
	else if (szCommand == START_ACTIVITY_COMMAND)
	{
		szReq.append("startactivity\">activityId=");
		szReq.append(szActivityId);
		szReq.append(":timestamp=0</oa></iq>");
	}
	else
	{
		// default action: return query for the current activity
		szReq.append("getCurrentActivity\" /></iq>");
	}

	return WriteToSocket(&szReq);
}


/************************************************************************
*									*
* Communication handler							*
*   - reconstructs network frames into a single communication block	*
*     to be handled by the parser					*
*									*
************************************************************************/
bool CHarmonyHub::CheckForHarmonyData()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_connectionstatus == DISCONNECTED)
		return false;

	if (m_bNeedMoreData)
		m_szHarmonyData.append(ReadFromSocket(m_connection, 0));
	else
		m_szHarmonyData = ReadFromSocket(m_connection, 0);

	if (m_szHarmonyData.empty())
		return false;


	if (m_szHarmonyData.compare(0, 8, "<message") == 0)
	{
		// Hub is sending one or more messages
		if (IsTransmissionComplete(&m_szHarmonyData))
			ParseHarmonyTransmission(&m_szHarmonyData);
	}

	else if (m_szHarmonyData == "<iq/>")
	{
		// Hub just acknowledges receiving our query
#ifdef _DEBUG
		std::cerr << "received ACK\n";
#endif
	}

	else if (m_szHarmonyData.compare(0, 3, "<iq") == 0)
	{
		// Hub is answering our query
		if (IsTransmissionComplete(&m_szHarmonyData))
			ParseHarmonyTransmission(&m_szHarmonyData);
	}

	else if (m_szHarmonyData.compare(0, 5, "<?xml") == 0)
	{
		// Hub responds to our start stream request
		ProcessHarmonyConnect(&m_szHarmonyData);
	}

	else if (m_szHarmonyData.compare(0, 7, "<stream") == 0)
	{
		// Hub responds to our (re)start stream request
		ProcessHarmonyConnect(&m_szHarmonyData);
	}

	else if (m_szHarmonyData.compare(0, 8, "<success") == 0)
	{
		// Hub accepted our credentials
		ProcessHarmonyConnect(&m_szHarmonyData);
	}

	else if (m_szHarmonyData.compare(0, 8, "</stream") == 0)
	{
		// Hub is closing the connection
		ParseHarmonyTransmission(&m_szHarmonyData);
	}

	else
	{
		// unhandled stanza
	}
	return true;
}


/************************************************************************
*									*
* Helper function for the communication handler				*
*   - verifies that the communication block is complete so we can	*
*     process it or we need to wait for additional data to come in	*
*									*
************************************************************************/
bool CHarmonyHub::IsTransmissionComplete(std::string *szHarmonyData)
{
	size_t msgClose = szHarmonyData->find("</message>", szHarmonyData->length() - 10);
	if (msgClose != std::string::npos)
		return true;
	size_t resultClose = szHarmonyData->find("</iq>", szHarmonyData->length() - 5);
	if (resultClose != std::string::npos)
		return true;
	size_t streamClose = szHarmonyData->find("</stream:stream>", szHarmonyData->length() - 16);
	if (streamClose != std::string::npos)
		return true;

	m_bNeedMoreData = true;
	return false;
}


/************************************************************************
*									*
* Wrapper function for pairing with the Harmony Hub			*
*									*
************************************************************************/
void CHarmonyHub::ProcessHarmonyConnect(std::string *szHarmonyData)
{
	m_bWantAnswer = false;
	int sendStatus = 0;
	if ((m_connectionstatus == CONNECTED) && (szHarmonyData->find("<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>") != std::string::npos))
	{
		// stream initiated successfully
		m_connectionstatus = INITIALIZED;
#ifdef _DEBUG
		std::cerr << "connectionstatus = initialized\n";
#endif
		if (m_szAuthorizationToken.empty())
			sendStatus = SendAuth(m_connection, "guest", "gatorade");
		else
			sendStatus = SendAuth(m_connection, m_szAuthorizationToken, m_szAuthorizationToken);
	}

	else if ((m_connectionstatus == INITIALIZED) && (*szHarmonyData == "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>"))
	{
		// authentication successful - restart our stream to bind
#ifdef _DEBUG
		std::cerr << "connectionstatus = authenticated\n";
#endif
		m_connectionstatus = AUTHENTICATED;
		sendStatus = StartStream(m_connection);
	}
	else if ((m_connectionstatus == AUTHENTICATED) && (szHarmonyData->find("<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>") != std::string::npos))
	{
		// stream bind completed
		m_connectionstatus = BOUND;
#ifdef _DEBUG
		std::cerr << "connectionstatus = bound\n";
#endif
	}

	if (sendStatus < 0)
	{
		// error while sending commands to hub
		_log.Log(LOG_ERROR, "Harmony Hub: Cannot setup command socket to Harmony Hub");
		ResetCommunicationSocket();
	}
}


/************************************************************************
*									*
* Query response handler						*
*  - handles direct responses to queries we sent to Harmony Hub		*
*									*
************************************************************************/
void CHarmonyHub::ProcessQueryResponse(std::string *szQueryResponse)
{
	size_t pos = 0;
	m_bWantAnswer = false;

	pos = szQueryResponse->find("errorcode=");
	if (pos != std::string::npos)
	{
		std::string szErrorcode = szQueryResponse->substr(pos + 11, 3);
#ifdef _DEBUG
		if (szErrorcode == "100")
		{
			// This errorcode has been seen with `mime='harmony.engine?startActivity'`
			// We're currently not interested in those - see related comment section below

			std::cerr << "iq message status = continue\n";
			return;
		}
#endif
		if (szErrorcode != "200")
		{
#ifdef _DEBUG
			std::cerr << "iq message status = error\n";
#endif
			return;
		}
	}

#ifdef _DEBUG
	std::cerr << "iq message status = OK\n";
#endif


	// mime='vnd.logitech.connect/vnd.logitech.ping'
	if (szQueryResponse->find("vnd.logitech.ping") != std::string::npos)
	{
		// got ping return
#ifdef _DEBUG
		std::cerr << "got ping return\n";
#endif
		m_bNeedEcho = false;
	}

	// mime='vnd.logitech.harmony/vnd.logitech.harmony.engine?getCurrentActivity'
	else if (szQueryResponse->find("engine?getCurrentActivity") != std::string::npos)
	{
		pos = szQueryResponse->find("<![CDATA[result=");
		if (pos != std::string::npos)
		{
			std::string szJsonString = szQueryResponse->substr(pos + 16);
			pos = szJsonString.find("]]>");
			if (pos != std::string::npos)
			{
				std::string szCurrentActivity = szJsonString.substr(0, pos);
				if (_log.IsDebugLevelEnabled(DEBUG_HARDWARE))
				{
					_log.Debug(DEBUG_HARDWARE, "Harmony Hub: Current activity ID = %d (%s)", atoi(szCurrentActivity.c_str()), m_mapActivities[szCurrentActivity].c_str());
				}

				if (m_szCurActivityID.empty()) // initialize all switches
				{
					m_szCurActivityID = szCurrentActivity;
					for (const auto & itt : m_mapActivities)
					{
						UpdateSwitch(0, itt.first.c_str(), (m_szCurActivityID == itt.first), itt.second);
					}
				}
				else if (m_szCurActivityID != szCurrentActivity)
				{
					CheckSetActivity(m_szCurActivityID, false);
					m_szCurActivityID = szCurrentActivity;
					CheckSetActivity(m_szCurActivityID, true);
				}
			}
		}
	}

#ifdef _DEBUG
	// mime='vnd.logitech.harmony/vnd.logitech.harmony.engine?startactivity'
	else if (szQueryResponse->find("engine?startactivity") != std::string::npos)
	{
		// doesn't appear to hold any sensible data - also always returns errorcode='200' even if activity is incorrect.
	}

	// mime='harmony.engine?startActivity'
	else if (szQueryResponse->find("engine?startActivity") != std::string::npos)
	{
		// This chain of query type messages follow after startactivity, apparently to inform that commands are being
		// executed towards specific deviceId's, but not showing what these commands are.

		// Since the mime is different from what we sent to Harmony Hub this appears to be a query directed to us, but
		// it is unknown whether we should acknowledge and/or return some response. The Hub doesn't seem to mind that
		// we don't though.
	}
#endif

	// mime='vnd.logitech.harmony/vnd.logitech.harmony.engine?config'
	else if (szQueryResponse->find("engine?config") != std::string::npos)
	{

#ifdef _DEBUG
		std::cerr << "reading engine config\n";
#endif
		pos = szQueryResponse->find("![CDATA[{");
		if (pos == std::string::npos)
		{
#ifdef _DEBUG
			std::cerr << "error: response does not contain a CDATA section\n";
#endif
			return;
		}

		std::string szJsonString = szQueryResponse->substr(pos + 8);
		Json::Value j_result;

		bool ret = ParseJSon(szJsonString, j_result);
		if ((!ret) || (!j_result.isObject()))
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities)");
			return;
		}

		if (j_result["activity"].empty())
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities)");
			return;
		}

		try
		{
			int totActivities = (int)j_result["activity"].size();
			for (int ii = 0; ii < totActivities; ii++)
			{
				std::string aID = j_result["activity"][ii]["id"].asString();
				std::string aLabel = j_result["activity"][ii]["label"].asString();
				m_mapActivities[aID] = aLabel;
			}
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities, JSon activity)");
		}

		if (_log.IsDebugLevelEnabled(DEBUG_HARDWARE))
		{
			std::string resultString = "Harmony Hub: Activity list: {";

			std::map<std::string, std::string>::iterator it = m_mapActivities.begin();
			std::map<std::string, std::string>::iterator ite = m_mapActivities.end();
			for (; it != ite; ++it)
			{
				resultString.append("\"");
				resultString.append(it->second);
				resultString.append("\":\"");
				resultString.append(it->first);
				resultString.append("\",");
			}
			resultString=resultString.substr(0, resultString.size()-1);
			resultString.append("}");

			_log.Debug(DEBUG_HARDWARE, resultString);
		}
	}

	// mime='vnd.logitech.connect/vnd.logitech.pair'
	else if (szQueryResponse->find("vnd.logitech.pair") != std::string::npos)
	{
		pos = szQueryResponse->find("identity=");
		if (pos == std::string::npos)
		{
#ifdef _DEBUG
			std::cerr << "GetAuthorizationToken : Logitech Harmony response does not contain a session authorization token\n";
#endif
			return;
		}
		m_szAuthorizationToken = szQueryResponse->substr(pos + 9);
		pos = m_szAuthorizationToken.find(':');
		if (pos == std::string::npos)
		{
#ifdef _DEBUG
			std::cerr << "GetAuthorizationToken : Logitech Harmony response does not contain a valid session authorization token\n";
#endif
			m_szAuthorizationToken = "";
			return;
		}
		m_szAuthorizationToken = m_szAuthorizationToken.substr(0, pos);
		CloseStream(m_connection);
		m_bLoginNow = true;
	}
}


/************************************************************************
*									*
* Message handler							*
*  - handles notifications sent by Harmony Hub				*
*									*
************************************************************************/
void CHarmonyHub::ProcessHarmonyMessage(std::string *szMessageBlock)
{
	size_t pos = 9;
	size_t msgStart = 0;

	if (szMessageBlock->compare(pos, 7, "content") != 0)
	{
		// bad message format
#ifdef _DEBUG
		std::cerr << "error: unrecognized message format (no content-length)\n";
#endif
		return;
	}
	pos += 16;
	size_t valueEnd = szMessageBlock->find('"', pos);
	int msglen = atoi(szMessageBlock->substr(pos, valueEnd - pos).c_str());
	msgStart = valueEnd + 4;
	if (szMessageBlock->compare(msgStart, 8, "<message") != 0)
	{
		// message block incorrectly positioned
#ifdef _DEBUG
		std::cerr << "warning: message block incorrectly positioned\n";
#endif
		msgStart = szMessageBlock->find("<message", pos);
		if ((msgStart == std::string::npos) || ((msgStart - pos) > 8))
		{
			// unable to find the actual message block
#ifdef _DEBUG
			std::cerr << "error: unrecognized message format\n";
#endif
			return;
		}
	}
	std::string szMessage = szMessageBlock->substr(msgStart, msglen);

	// <event xmlns="connect.logitech.com" type="connect.stateDigest?notify">
	if (szMessage.find("stateDigest?notify", pos) != std::string::npos)
	{
		// Event state notification
		char cActivityStatus = 0;

		size_t jpos = szMessage.find("activityStatus");
		if (jpos != std::string::npos)
		{
			cActivityStatus = szMessage[jpos+16];
			bool bIsChanging = ((cActivityStatus == '1') || (cActivityStatus == '3'));
			if (bIsChanging != m_bIsChangingActivity)
			{
				m_bIsChangingActivity = bIsChanging;
				if (m_bIsChangingActivity)
					_log.Log(LOG_STATUS, "Harmony Hub: Changing activity");
				else
					_log.Log(LOG_STATUS, "Harmony Hub: Finished changing activity");
			}
		}

		if (_log.IsDebugLevelEnabled(DEBUG_HARDWARE) || (m_szHubSwVersion.empty()))
		{
			jpos = szMessage.find("hubSwVersion");
			if (jpos != std::string::npos)
			{
				m_szHubSwVersion = szMessage.substr(jpos+15, 16); // limit string length for end delimiter search
				jpos = m_szHubSwVersion.find('"');
				if (jpos != std::string::npos)
				{
					if (m_szHubSwVersion.empty())
						_log.Log(LOG_STATUS, "Harmony Hub: Software version: %s", m_szHubSwVersion.c_str());
					m_szHubSwVersion = m_szHubSwVersion.substr(0, jpos);
				}
			}
		}

		if (_log.IsDebugLevelEnabled(DEBUG_HARDWARE))
		{
			std::string activityId, stateVersion;
			jpos = szMessage.find("runningActivityList");
			if (jpos != std::string::npos)
			{
				activityId = szMessage.substr(jpos+22, 16); // limit string length for end delimiter search
				jpos = activityId.find('"');
				if (jpos != std::string::npos)
					activityId = activityId.substr(0, jpos);
			}
			if (jpos == std::string::npos)
				activityId = "NaN";
			else if (activityId.empty())
				activityId = "-1";

			jpos = szMessage.find("stateVersion");
			if (jpos != std::string::npos)
			{
				stateVersion = szMessage.substr(jpos+14, 16); // limit string length for end delimiter search
				jpos = stateVersion.find(',');
				if (jpos != std::string::npos)
					stateVersion = stateVersion.substr(0, jpos);
			}
			if ((jpos == std::string::npos) || stateVersion.empty())
				stateVersion = "NaN";

			_log.Debug(DEBUG_HARDWARE, "Harmony Hub: Event state notification: stateVersion = %s, hubSwVersion = %s, activityStatus = %c, activityId = %s", stateVersion.c_str(), m_szHubSwVersion.c_str(), cActivityStatus, activityId.c_str() );
		}
	}

	// <event xmlns="connect.logitech.com" type="harmony.engine?startActivityFinished">
	// <event xmlns="connect.logitech.com" type="harmony.engine?helpdiscretes">
	else if ((szMessage.find("engine?startActivityFinished", pos) != std::string::npos) ||
		 (szMessage.find("engine?helpdiscretes", pos) != std::string::npos))
	{
		// start activity finished
		size_t jpos = szMessage.find("activityId");
		std::string szActivityId;
		if (jpos != std::string::npos)
		{
			szActivityId = szMessage.substr(jpos+11, 16); // limit string length for end delimiter search
			jpos = szActivityId.find(':');
			if (jpos == std::string::npos)
				jpos = szActivityId.find(']');
			if (jpos != std::string::npos)
				szActivityId = szActivityId.substr(0, jpos);
		}

		if (jpos == std::string::npos)
			return;

		if (szActivityId != m_szCurActivityID)
		{
			CheckSetActivity(m_szCurActivityID, false);
			m_szCurActivityID = szActivityId;
			CheckSetActivity(m_szCurActivityID, true);
		}
	}

}


/************************************************************************
*									*
* XMPP communication block parser					*
*  - splits the communication block we received into its individual	*
*    components and sends it to the correct handler			*
*									*
************************************************************************/
void CHarmonyHub::ParseHarmonyTransmission(std::string *szHarmonyData)
{
	m_bNeedMoreData = false;
	size_t pos = 0;
	while ((pos != std::string::npos) && (pos < szHarmonyData->length()))
	{
		if (szHarmonyData->compare(pos, 5, "<iq/>") == 0)
		{
#ifdef _DEBUG
			std::cerr << "received ACK\n";
#endif
			pos += 5;
		}

		else if (szHarmonyData->compare(pos, 3, "<iq") == 0)
		{
			size_t iqStart = pos;
			pos = szHarmonyData->find("</iq>", iqStart);
			pos += 5;
			std::string szQueryResponse = szHarmonyData->substr(iqStart, pos - iqStart);
#ifdef _DEBUG
			std::cerr << szQueryResponse << '\n';
#endif
			ProcessQueryResponse(&szQueryResponse);
		}

		else if (szHarmonyData->compare(pos, 8, "<message") == 0)
		{
			size_t msgStart = pos;
			pos = szHarmonyData->find("</message>", msgStart);
			pos += 10;
			std::string szMessage = szHarmonyData->substr(msgStart, pos - msgStart);
#ifdef _DEBUG
			std::cerr << szMessage << '\n';
#endif
			ProcessHarmonyMessage(&szMessage);
			m_bReceivedMessage = true; // need this to fast forward our ping interval counter
		}

		else if (szHarmonyData->compare(pos, 8, "</stream") == 0)
		{
			// Hub is closing the connection
#ifdef _DEBUG
			std::cerr << "Hub closed connection\n";
#endif
			ResetCommunicationSocket();
			pos = szHarmonyData->length(); // this is always the last transmission
		}
	}
}



