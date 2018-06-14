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
 - Refactored: address several issues with working with firmware 4.14.123
*/

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
#include "../json/json.h"

#define CONNECTION_ID				"21345678-1234-5678-1234-123456789012-1"
#define GET_CONFIG_COMMAND			"get_config"
#define START_ACTIVITY_COMMAND			"start_activity"
#define GET_CURRENT_ACTIVITY_COMMAND		"get_current_activity_id"
#define HARMONY_PING_INTERVAL_SECONDS		30	// must be smaller than 40 seconds or Hub will start feeding us empty messages
#define HARMONY_SEND_ACK_SECONDS		10	// must be smaller than 20 seconds...
#define HARMONY_RETRY_LOGIN_SECONDS		60
#define HEARTBEAT_SECONDS			12


// Note:
// HarmonyHub is on Wifi and can thus send frames with a maximum payload length of 2324 bytes
// Normal implementations will however obey the 1500 bytes MTU from the wired networks that
// they are attached to and this may be limited even further if the router uses mechanisms like
// PPTP for connecting the (Wireless) LAN to the internet.
#define SOCKET_BUFFER_SIZE  1500


// socket timeout values
#define TIMEOUT_WAIT_FOR_ANSWER 2.0f
#define TIMEOUT_WAIT_FOR_NEXT_FRAME 0.4f



CHarmonyHub::CHarmonyHub(const int ID, const std::string &IPAddress, const unsigned int port):
m_harmonyAddress(IPAddress)
{
	m_usIPPort = port;
	m_HwdID=ID;
	m_commandcsocket=NULL;
	Init();
}


CHarmonyHub::~CHarmonyHub(void)
{
	Logout();
}


bool CHarmonyHub::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF*>(pdata);

	if (this->m_bIsChangingActivity)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Command cannot be sent. Hub is changing activity");
		return false;
	}

	if (this->m_bDoLogin)
	{
		if (this->m_szAuthorizationToken.empty() &&
		     (pCmd->LIGHTING2.id1 == 0xFF) && (pCmd->LIGHTING2.id2 == 0xFF) &&
		     (pCmd->LIGHTING2.id3 == 0xFF) && (pCmd->LIGHTING2.id4 == 0xFF) &&
		     (pCmd->LIGHTING2.cmnd == 0)
		) // "secret" undefined state request to silence error reporting
		{
			if (this->m_bShowConnectError)
				_log.Log(LOG_STATUS, "Harmony Hub: disable connection error logging");
			this->m_bShowConnectError = false;
			return false;
		}

		_log.Log(LOG_STATUS, "Harmony Hub: Received a switch command but we are not connected - attempting connect now");
		if (Login() && SetupCommandSocket())
		{
			this->m_bDoLogin = false;
			if (!UpdateActivities() || !UpdateCurrentActivity())
			{
				ResetCommandSocket();
			}
		}
		if (this->m_bDoLogin)
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Connect failed: cannot send the switch command");
			return false;
		}
		sleep_milliseconds(500); // Hub doesn't seem to like it if we instantly issue the switch command after connecting
	}

	if (pCmd->LIGHTING2.packettype == pTypeLighting2)
	{
		int lookUpId = (int)(pCmd->LIGHTING2.id1 << 24) |  (int)(pCmd->LIGHTING2.id2 << 16) | (int)(pCmd->LIGHTING2.id3 << 8) | (int)(pCmd->LIGHTING2.id4) ;
		std::stringstream sstr;
		sstr << lookUpId;
		std::string realID = sstr.str();

		if (pCmd->LIGHTING2.cmnd == 0)
		{
			if (m_szCurActivityID != realID)
			{
				return false;
			}
			if (realID == "-1") // powering off the PowerOff activity leads to an undefined state in the frontend
			{
				// send it anyway, user may be trying to correct a wrong state of the Hub
				SubmitCommand(START_ACTIVITY_COMMAND, "-1", "");
				// but don't allow the frontend to update the button state to the off position
				return false;
			}
			if (!SubmitCommand(START_ACTIVITY_COMMAND, "-1", ""))
			{
				_log.Log(LOG_ERROR, "Harmony Hub: Error sending the power-off command");
				return false;
			}
		}
		else if (!SubmitCommand(START_ACTIVITY_COMMAND, realID, ""))
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Error sending the switch command");
			return false;
		}
	}
	return true;
}


void CHarmonyHub::Init()
{
	m_stoprequested = false;
	m_bDoLogin = true;
	m_szCurActivityID = "";
	m_bIsChangingActivity = false;
	m_hubSwVersion = "";
	m_bShowConnectError = true;
}


bool CHarmonyHub::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CHarmonyHub::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}


bool CHarmonyHub::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
	m_bIsStarted=false;
	if (!m_bDoLogin)
		Logout();
	m_bIsChangingActivity=false;
	m_hubSwVersion = "";
	return true;
}


void CHarmonyHub::Do_Work()
{
	_log.Log(LOG_STATUS,"Harmony Hub: Worker thread started..."); 

	bool bFirstTime = true;
	char mcounter = 0;		// heartbeat
	unsigned int scounter = 0;	// seconds
	char fcounter = 0;		// failed login attempts
	char lcounter = 0;		// login counter

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		if (m_stoprequested)
			break;

		mcounter++;
		if (mcounter % 2)
			continue;

		scounter++;

		if (mcounter % (HEARTBEAT_SECONDS * 2) == 0)
		{
			mcounter = 0;
			m_LastHeartbeat=mytime(NULL);
		}

		if (m_bDoLogin)
		{
			if (m_mapActivities.size() == 0)
			{
				// compensate heartbeat for possible large timeout on accessing Hub
				if ((HEARTBEAT_SECONDS * 2 - mcounter) < 10)
					continue;
				mcounter += 10;
			}

			if ((scounter % HARMONY_RETRY_LOGIN_SECONDS == 0) || (bFirstTime))
			{
				bFirstTime = false;
				lcounter++;
				if (lcounter <= fcounter)
					continue;

				scounter = 0;
				lcounter = 0;
				if (fcounter > 0)
					_log.Log(LOG_NORM, "Harmony Hub: Reattempt login.");

				if (Login() && SetupCommandSocket())
				{
					m_bDoLogin=false;
					if (!UpdateActivities() || !UpdateCurrentActivity())
					{
						_log.Log(LOG_ERROR, "Harmony Hub: Error updating activities.. Resetting connection.");
						ResetCommandSocket();
						continue;
					}

					_log.Log(LOG_STATUS, "Harmony Hub: Connected to Hub.");
					fcounter = 0;
				}
				else if (fcounter < 5) // will cause login attempts to gradually decrease to once per 6 minutes
					fcounter ++;
			}
			continue;
		}

		if (scounter % HARMONY_PING_INTERVAL_SECONDS == 0)
		{
			// Ping hub to see if it is still alive
			if (!SendPing())
			{
				_log.Log(LOG_ERROR, "Harmony Hub: Error pinging server.. Resetting connection.");
				ResetCommandSocket();
				scounter = HARMONY_RETRY_LOGIN_SECONDS - 5; // wait 5 seconds before attempting login again
			}
			continue;
		}

		bool bIsDataReadable = true;
		m_commandcsocket->canRead(&bIsDataReadable, 0); // we're not expecting data at this point, so don't wait
		if (bIsDataReadable)
		{
			// Harmony Hub requires us to send a 'ping' within 20 seconds after receiving volunteered status reports
			scounter = HARMONY_PING_INTERVAL_SECONDS - HARMONY_SEND_ACK_SECONDS;

			boost::lock_guard<boost::mutex> lock(m_mutex);
			std::string strData;
			ReceiveMessage(m_commandcsocket, strData, -1, TIMEOUT_WAIT_FOR_NEXT_FRAME, false);

			if (!strData.empty())
				CheckIfChanging(strData);
			else
			{
				ResetCommandSocket(); // we exceeded our ACK time frame and Harmony Hub will no longer accept our commands or send valid data
				scounter = HARMONY_RETRY_LOGIN_SECONDS - 5; // wait 5 seconds before attempting login again

			}
		}
	}
	_log.Log(LOG_STATUS,"Harmony Hub: Worker stopped...");
}

bool CHarmonyHub::Login()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);

	if (!m_szAuthorizationToken.empty()) // we already have an authentication token
		return true;

	csocket authorizationcsocket;
	if (!ConnectToHarmony(m_harmonyAddress, m_usIPPort, &authorizationcsocket))
	{
		if (m_bShowConnectError)
			_log.Log(LOG_ERROR,"Harmony Hub: Cannot connect to Harmony Hub. Check IP/Port.");
		return false;
	}
	if (GetAuthorizationToken(&authorizationcsocket)==true)
	{
		_log.Log(LOG_STATUS,"Harmony Hub: Authentication successful");
		m_bShowConnectError = true;
		return true;
	}
	return false;
}


bool CHarmonyHub::ReceiveMessage(csocket* communicationcsocket, std::string &strMessage, float waitTimePrimary, float waitTimeSecondary, bool append)
{
	bool bGotData = false;
	if (!append)
		strMessage = "";
	bool bIsDataReadable = true;
	if (waitTimePrimary >= 0)
		communicationcsocket->canRead(&bIsDataReadable, waitTimePrimary);

	char databuffer[SOCKET_BUFFER_SIZE];
	while (bIsDataReadable)
	{
		int bytesReceived = communicationcsocket->read(databuffer, SOCKET_BUFFER_SIZE, false);
		if (bytesReceived > 0)
		{
			strMessage.append(databuffer, 0, bytesReceived);
			if (waitTimeSecondary < 0)
				return true;
			communicationcsocket->canRead(&bIsDataReadable, waitTimeSecondary);
			bGotData = true;
		}
		else
			bIsDataReadable = false;
	}
	return bGotData;
}


void CHarmonyHub::ResetCommandSocket()
{
	if (m_commandcsocket)
		delete m_commandcsocket;
	m_commandcsocket = NULL;
	m_bIsChangingActivity = false;
	m_bDoLogin = true;
	m_szCurActivityID = "";
	m_bShowConnectError = true;
}


void CHarmonyHub::Logout()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	ResetCommandSocket();
	m_szAuthorizationToken = "";
	m_mapActivities.clear();
}


bool CHarmonyHub::SetupCommandSocket()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	if (m_commandcsocket)
		ResetCommandSocket();

	m_commandcsocket = new csocket();

	if (!ConnectToHarmony(m_harmonyAddress, m_usIPPort,m_commandcsocket))
	{
		if (m_bShowConnectError)
			_log.Log(LOG_ERROR,"Harmony Hub: Cannot setup command socket to Harmony Hub");
		m_bShowConnectError = false;
		return false;
	}

	std::string strUserName = m_szAuthorizationToken;
	//strUserName.append("@connect.logitech.com/gatorade.");
	std::string strPassword = m_szAuthorizationToken;

	if (!StartCommunication(m_commandcsocket, strUserName, strPassword))
	{
		if (m_bShowConnectError)
			_log.Log(LOG_ERROR,"Harmony Hub: Start communication failed");
		m_bShowConnectError = false;
		return false;
	}
	m_bShowConnectError = true;
	return true;
}


bool CHarmonyHub::UpdateActivities()
{
	if (m_mapActivities.size() > 0) // we already have the list of activities.
		return true;
	
	if (!SubmitCommand(GET_CONFIG_COMMAND, "", ""))
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Get activities failed");
		return false;
	}

	Json::Reader jReader;
	Json::Value root;
	bool ret = jReader.parse(m_szResultString, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities)");
		return false;
	}

	if (root["activity"].empty())
	{
		_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities)");
		return false;
	}

	try
	{
		int totActivities = (int)root["activity"].size();
		for (int ii = 0; ii < totActivities; ii++)
		{
			std::string aID = root["activity"][ii]["id"].asString();
			std::string aLabel = root["activity"][ii]["label"].asString();
			m_mapActivities[aID] = aLabel;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Update Activities, JSon activity)");
	}
	return true;
}


bool CHarmonyHub::UpdateCurrentActivity()
{
	if (!SubmitCommand(GET_CURRENT_ACTIVITY_COMMAND, "", ""))
	{
		//_log.Log(LOG_ERROR,"Harmony Hub: Get current activity failed");
		return false;
	}

	//check if changed
	if (m_szCurActivityID != m_szResultString) 
	{
		if (m_szCurActivityID.empty()) // initialize all switches
		{
			std::map< std::string, std::string>::const_iterator itt;
			int cnt = 0;
			for (itt = m_mapActivities.begin(); itt != m_mapActivities.end(); ++itt)
			{
				UpdateSwitch(cnt++, itt->first.c_str(), (m_szResultString == itt->first), itt->second);
			}
		}
		else
		{
			CheckSetActivity(m_szCurActivityID, false);
			CheckSetActivity(m_szResultString, true);
		}
		m_szCurActivityID = m_szResultString;
	}

	return true;
}


void CHarmonyHub::CheckSetActivity(const std::string &activityID, const bool on)
{
	//get the device id from the db (if already inserted)
	int actId=atoi(activityID.c_str());
	std::stringstream hexId ;
	hexId << std::setw(7)  << std::hex << std::setfill('0') << std::uppercase << (int)( actId) ;
	std::string actHex = hexId.str();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, actHex.c_str());
	if (!result.empty()) //if not yet inserted, it will be inserted active upon the next check of the activities list
	{
		UpdateSwitch(atoi(result[0][1].c_str()), activityID.c_str(),on,result[0][0]);
	}
}


void CHarmonyHub::UpdateSwitch(unsigned char idx,const char * realID, const bool bOn, const std::string &defaultname)
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
	int level = 15;
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}


bool CHarmonyHub::ConnectToHarmony(const std::string &strHarmonyIPAddress, const int harmonyPortNumber, csocket* harmonyCommunicationcsocket)
{
	if (strHarmonyIPAddress.length() == 0 || harmonyPortNumber == 0 || harmonyPortNumber > 65535)
		return false;

	harmonyCommunicationcsocket->connect(strHarmonyIPAddress.c_str(), harmonyPortNumber);

	return (harmonyCommunicationcsocket->getState() == csocket::CONNECTED);
}


bool CHarmonyHub::StartCommunication(csocket* communicationcsocket, const std::string &strUserName, const std::string &strPassword)
{
	if (communicationcsocket == NULL || strUserName.length() == 0 || strPassword.length() == 0)
		return false;

	// Start communication
	std::string strReq = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(strReq.c_str(), static_cast<unsigned int>(strReq.length()));
	std::string strData;

	ReceiveMessage(communicationcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, -1, false);
	/* <- Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='XXXXXXXX' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features> */

	if (strData.find("<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>") == std::string::npos)
	{
		//errorString = "StartCommunication : unexpected response";
		return false;
	}


	std::string strAuth = "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"PLAIN\">";
	std::string strCred = "\0";
	strCred.append(strUserName);
	strCred.append("\0");
	strCred.append(strPassword);
	strAuth.append(base64_encode((const unsigned char*)strCred.c_str(), static_cast<unsigned int>(strCred.length())));
	strAuth.append("</auth>");
	communicationcsocket->write(strAuth.c_str(), static_cast<unsigned int>(strAuth.length()));

	ReceiveMessage(communicationcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, -1, false);
	/* <- Expect: <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/> */

	if (strData != "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>")
	{
		//errorString = "StartCommunication : authentication error";
		return false;
	}

	//strReq = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(strReq.c_str(), static_cast<unsigned int>(strReq.length()));

	ReceiveMessage(communicationcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, -1, false);
	/* <- Expect: <stream:stream from='connect.logitech.com' id='XXXXXXXX' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/><session xmlns='urn:ietf:params:xml:nx:xmpp-session'/></stream:features> */

	if (strData.find("<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>") == std::string::npos)
	{
		//errorString = "startCommunication : bind failed";
		return false;
	}

	return true;
}


bool CHarmonyHub::GetAuthorizationToken(csocket* authorizationcsocket)
{
	if (!StartCommunication(authorizationcsocket, "guest", "gatorade."))
	{
		//errorString = "SwapAuthorizationToken : Communication failure";
		return false;
	}

	std::string strData;
	std::string strReq;

	strReq = "<iq type=\"get\" id=\"";
	strReq.append(CONNECTION_ID);
	strReq.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.pair\">method=pair");
	strReq.append(":name=domoticz#iOS10.1.0#iPhone</oa></iq>");

	authorizationcsocket->write(strReq.c_str(), static_cast<unsigned int>(strReq.length()));

	ReceiveMessage(authorizationcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, -1, false);
	/* <- Expect: <iq/> */

	if (strData.find("<iq/>") != 0)
	{
		//errorString = "SwapAuthorizationToken : Invalid Harmony response";
		return false;
	}

	ReceiveMessage(authorizationcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, TIMEOUT_WAIT_FOR_NEXT_FRAME, false);

	// Parse the session authorization token from the response
	size_t pos = (int)strData.find("identity=");
	if (pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a session authorization token";
		return false;
	}
	m_szAuthorizationToken = strData.substr(pos + 9);

	pos = (int)m_szAuthorizationToken.find(":");
	if (pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a valid session authorization token";
		return false;
	}
	m_szAuthorizationToken = m_szAuthorizationToken.substr(0, pos);

	return true;
}


bool CHarmonyHub::SendPing()
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	if (m_commandcsocket == NULL || m_szAuthorizationToken.empty())
		return false;

	std::string strData;
	std::string strReq;

	// GENERATE A PING REQUEST USING THE HARMONY ID AND LOGIN AUTHORIZATION TOKEN 
	strReq = "<iq type=\"get\" id=\"";
	strReq.append(CONNECTION_ID);
	strReq.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.ping\">token=");
	strReq.append(m_szAuthorizationToken.c_str());
	strReq.append(":name=domoticz#iOS10.1.0#iPhone</oa></iq>");

	m_commandcsocket->write(strReq.c_str(), static_cast<unsigned int>(strReq.length()));

	ReceiveMessage(m_commandcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, TIMEOUT_WAIT_FOR_NEXT_FRAME, false);
/* <- Expect:
 <- optional messages: <message content-length="123"/><message from ... </message>
 <- ping return: <iq/><iq id="12345678-1234-5678-1234-123456789012-1" type="get"><oa xmlns='connect.logitech.com' mime='vnd.logitech.connect/vnd.logitech.ping' errorcode='200' errorstring='OK'><![CDATA[status=alive:uuid=12345678-1234-5678-1234-123456789012:susTrigger=xmpp:name=domoticz#iOS10.1.0#iPhone:id=12345678-1234-5678-1234-123456789012-1:token=12345678-1234-5678-1234-123456789012]]></oa></iq> */

	if (strData.empty())
		return false;

	if (strData.find("</message>") != std::string::npos) // messages included
		CheckIfChanging(strData);

	return CheckIqGood(strData);
}


bool CHarmonyHub::CheckIqGood(const std::string& strData)
{
	size_t iqstart = 0;
	while (1)
	{
		iqstart = strData.find("<iq ", iqstart);
		if (iqstart == std::string::npos)
			return true;
		size_t iqend = strData.find("</iq>", iqstart);
		if (iqend == std::string::npos)
			return false;
		std::string iqmsg = strData.substr(iqstart, iqend);
		iqstart = iqend;
		if (iqmsg.find("errorcode='200'") != std::string::npos) // status OK
			continue;
		if (iqmsg.find("errorcode='100'") != std::string::npos) // status continue - Hub has more data
			continue;
		return false;
	}
	return true;
}


bool CHarmonyHub::SubmitCommand(const std::string &strCommand, const std::string &strCommandParameterPrimary, const std::string &strCommandParameterSecondary)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	if (m_commandcsocket == NULL || m_szAuthorizationToken.empty())
	{
		//errorString = "SubmitCommand : NULL csocket or empty authorization token provided";
		return false;

	}

	std::string lstrCommand = strCommand;
	if (lstrCommand.length() == 0)
	{
		// No command provided, return query for the current activity
		lstrCommand = GET_CURRENT_ACTIVITY_COMMAND;
	}

	std::string strData;
	std::string strReq;

	strReq = "<iq type=\"get\" id=\"";
	strReq.append(CONNECTION_ID);
	strReq.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.harmony/vnd.logitech.harmony.engine?");

	// Issue the provided command
	if (lstrCommand == GET_CURRENT_ACTIVITY_COMMAND)
	{
		strReq.append("getCurrentActivity\" /></iq>");
	}
	else if (lstrCommand == GET_CONFIG_COMMAND)
	{
		strReq.append("config\"></oa></iq>");
	}
	else if (lstrCommand == START_ACTIVITY_COMMAND)
	{
		strReq.append("startactivity\">activityId=");
		strReq.append(strCommandParameterPrimary.c_str());
		strReq.append(":timestamp=0</oa></iq>");
	}

	m_commandcsocket->write(strReq.c_str(), static_cast<unsigned int>(strReq.length()));

	ReceiveMessage(m_commandcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, TIMEOUT_WAIT_FOR_NEXT_FRAME, false);

	if (strData.empty())
		return false;

	CheckIfChanging(strData);

	if (strCommand == GET_CURRENT_ACTIVITY_COMMAND)
	{
		size_t astart = strData.find("result=");
		if (astart != std::string::npos)
		{
			size_t aend = strData.find("]]>",astart + 7);
			if (aend != std::string::npos)
			{
				m_szResultString = strData.substr(astart + 7, aend - astart - 7);
				return true;
			}
		}

		//No valid response received
		if (m_bIsChangingActivity)
		{
			m_szResultString = m_szCurActivityID; //changing, so no response from HH
			return true;
		}

		return false;
	}
	else if (strCommand == GET_CONFIG_COMMAND)
	{
		ReceiveMessage(m_commandcsocket, strData, TIMEOUT_WAIT_FOR_ANSWER, TIMEOUT_WAIT_FOR_NEXT_FRAME, true);

		size_t cstart = strData.find("<![CDATA[");
		if (cstart != std::string::npos)
		{
			cstart += 9;
			size_t cend = strData.find("]]>",cstart);
			if (cend != std::string::npos)
			{
				m_szResultString = strData.substr(cstart, cend - cstart);
				return true;
			}
		}

		return false;
	}
	else if (strCommand == START_ACTIVITY_COMMAND)
	{
		m_szResultString = "";
	}
	return true;
}


bool CHarmonyHub::CheckIfChanging(const std::string& strData)
{
	//activityStatus
	// 0 = All activities are off
	// 1 = Activity is starting
	// 2 = Activity is started
	// 3 = Activity is stopping

	bool bIsChanging = m_bIsChangingActivity;
	std::string LastActivity = m_szCurActivityID;

	std::string szData = strData;
	size_t pos;

	while (!szData.empty())
	{
		size_t apos = szData.find("</message>");
		if (apos == std::string::npos)
			break;

		std::string szResponse = szData.substr(0, apos);
		szData = szData.substr(apos + 10);
		if (szResponse.find("getCurrentActivity") != std::string::npos)
			continue; //dont want you

		if (szResponse.find("startActivityFinished") != std::string::npos)
		{
			bIsChanging = false;
			pos = szResponse.find("<![CDATA[");
			if (pos == std::string::npos)
				continue;
			szResponse = szResponse.substr(pos + 9);

			pos = szResponse.find("]]>");
			if (pos == std::string::npos)
				continue;
			szResponse[pos-1] = ':';

			pos = szResponse.find("activityId=");
			if (pos == std::string::npos)
				continue;
			szResponse = szResponse.substr(pos + 11);

			pos = szResponse.find(":");
			if (pos == std::string::npos)
				continue;
			szResponse = szResponse.substr(0, pos);

			LastActivity = szResponse;
			continue;
		}

		pos = szResponse.find("<![CDATA[");
		if (pos == std::string::npos)
			continue;
		szResponse = szResponse.substr(pos + 9);

		pos = szResponse.find("]]>");
		if (pos == std::string::npos)
			continue;
		szResponse = szResponse.substr(0, pos);

		pos = szResponse.find("activityStatus");
		if (pos == std::string::npos)
			continue;

		char cActivityStatus = szResponse[pos+16];

		pos = szResponse.find("hubSwVersion");
		if (pos != std::string::npos)
		{
			std::string hubSwVersion = szResponse.substr(pos+15, 16); // limit string length for end delimiter search
			pos = hubSwVersion.find("\"");
			if (pos == std::string::npos)
				continue;
			hubSwVersion = hubSwVersion.substr(0, pos);
			if (hubSwVersion != m_hubSwVersion)
			{
				m_hubSwVersion = hubSwVersion;
				_log.Log(LOG_STATUS, "Harmony Hub: Software version: %s", m_hubSwVersion.c_str());
			}
		}

		bIsChanging = ((cActivityStatus == '1') || (cActivityStatus == '3'));
		if (cActivityStatus == '2')
		{
			pos = szResponse.find("activityId");
			if (pos == std::string::npos)
				continue;
			szResponse = szResponse.substr(pos + 13, 10); // limit string length for end delimiter search
			pos = szResponse.find("\"");
			if (pos != std::string::npos)
			{
				LastActivity = szResponse.substr(0, pos);
			}
		}
		else if (cActivityStatus == '0')
		{
			//Power Off
			LastActivity = "-1";
		}
	}

	if (bIsChanging != m_bIsChangingActivity)
	{
		m_bIsChangingActivity = bIsChanging;
		if (m_bIsChangingActivity)
			_log.Log(LOG_STATUS, "Harmony Hub: Changing activity");
		else
			_log.Log(LOG_STATUS, "Harmony Hub: Finished changing activity");
	}
	if (m_szCurActivityID != LastActivity)
	{
		if (!m_szCurActivityID.empty())
		{
			//need to set the old activity to off
			CheckSetActivity(m_szCurActivityID, false);
		}
		CheckSetActivity(LastActivity, true);
		m_szCurActivityID = LastActivity;
	}
	return true;
}

/*
bool CHarmonyHub::ParseAction(const std::string& strAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID)
{
	Action a;
	const std::string commandTag = "\\\"command\\\":\\\"";
	int commandStart = strAction.find(commandTag);
	int commandEnd = strAction.find("\\\",\\\"", commandStart);
	a.m_strCommand = strAction.substr(commandStart + commandTag.size(), commandEnd - commandStart - commandTag.size());

	const std::string deviceIdTag = "\\\"deviceId\\\":\\\"";
	int deviceIDStart = strAction.find(deviceIdTag, commandEnd);

	const std::string nameTag = "\\\"}\",\"name\":\"";
	int deviceIDEnd = strAction.find(nameTag, deviceIDStart);

	std::string commandDeviceID = strAction.substr(deviceIDStart + deviceIdTag.size(), deviceIDEnd - deviceIDStart - deviceIdTag.size());
	if(commandDeviceID != strDeviceID)
	{
		return false;
	}

	int nameStart = deviceIDEnd + nameTag.size();

	const std::string labelTag = "\",\"label\":\"";
	int nameEnd = strAction.find(labelTag, nameStart);

	a.m_strName = strAction.substr(nameStart, nameEnd - nameStart);

	int labelStart = nameEnd + labelTag.size();
	int labelEnd = strAction.find("\"}", labelStart);

	a.m_strLabel = strAction.substr(labelStart, labelEnd - labelStart);

	vecDeviceActions.push_back(a);
	return true;
}
*/
/*
bool CHarmonyHub::ParseFunction(const std::string& strFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID)
{
	Function f;
	int functionNameEnd = strFunction.find("\",\"function\":[{");
	if(functionNameEnd == std::string::npos)
	{
		return false;
	}

	f.m_strName = strFunction.substr(0, functionNameEnd);

	const std::string actionTag = "\"action\":\"";
	int actionStart = strFunction.find(actionTag, functionNameEnd);

	while(actionStart != std::string::npos)
	{
		const std::string labelTag = "\"label\":\"";
		int actionEnd = strFunction.find(labelTag, actionStart);
		if(actionEnd == std::string::npos)
		{
			return false;
		}
		actionEnd = strFunction.find("\"}", actionEnd + labelTag.size());

		std::string strAction = strFunction.substr(actionStart + actionTag.size(), actionEnd - actionStart - actionTag.size());
		ParseAction(strAction, f.m_vecActions, strDeviceID);

		actionStart = strFunction.find(actionTag, actionEnd);
	}

	vecDeviceFunctions.push_back(f);

	return true;
}
*/

