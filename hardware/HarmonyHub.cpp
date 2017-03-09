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

#define CONNECTION_ID								"21345678-1234-5678-1234-123456789012-1"
#define GET_CONFIG_COMMAND							"get_config"
#define GET_CONFIG_COMMAND_RAW						"get_config_raw"
#define START_ACTIVITY_COMMAND						"start_activity"
#define GET_CURRENT_ACTIVITY_COMMAND				"get_current_activity_id"
#define GET_CURRENT_ACTIVITY_COMMAND_RAW			"get_current_activity_id_raw"
#define HARMONY_PING_INTERVAL_SECONDS				30	//the get activity poll time
#define HARMONY_RETRY_LOGIN_SECONDS					60  //fetch the list of activities every x seconds...

#define MAX_MISS_COMMANDS							5	//max commands to miss (when executing a command, the harmony commands may fail)

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

	if(this->m_bIsChangingActivity)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Command cannot be sent. Hub is changing activity");
		return false;
	}
	//activities can be switched on
	if ((pCmd->LIGHTING2.packettype == pTypeLighting2) && (pCmd->LIGHTING2.cmnd==1))
	{
		int lookUpId = (int)(pCmd->LIGHTING2.id1 << 24) |  (int)(pCmd->LIGHTING2.id2 << 16) | (int)(pCmd->LIGHTING2.id3 << 8) | (int)(pCmd->LIGHTING2.id4) ;
		std::stringstream sstr;
		sstr << lookUpId;

		//get the activity id from the db and send to h/w
		if (!SubmitCommand(START_ACTIVITY_COMMAND, sstr.str(), ""))
		{
			_log.Log(LOG_ERROR,"Harmony Hub: Error sending the switch command");
			return false;
		}			
	}
	else if((pCmd->LIGHTING2.packettype == pTypeLighting2) && (pCmd->LIGHTING2.cmnd==0))
	{
		if(!SubmitCommand(START_ACTIVITY_COMMAND, "PowerOff",""))
		{
			_log.Log(LOG_ERROR,"Harmony Hub: Error sending the power-off command");
			return false;
		}			
	}
	return true;
}

void CHarmonyHub::Init()
{
	m_stoprequested = false;
	m_bDoLogin = true;
	m_szCurActivityID="";
	m_bIsChangingActivity=false;
	m_hubSwVersion = "";
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

	int scounter = 0;
	int mcounter = 0;
	bool bFirstTime = true;

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		if (m_stoprequested)
			break;

		mcounter++;
		if (mcounter<2)
			continue;
		mcounter = 0;

		scounter++;

		if (scounter % 12 == 0)
		{
			m_LastHeartbeat=mytime(NULL);
		}

		if (m_bDoLogin)
		{
			if ((scounter%HARMONY_RETRY_LOGIN_SECONDS == 0) || (bFirstTime))
			{
				bFirstTime = false;
				if(Login() && SetupCommandSocket())
				{
					m_bDoLogin=false;
					if (!UpdateCurrentActivity())
					{
						Logout();
						m_bDoLogin = true;
					}
					else
					{
						if (!UpdateActivities())
						{
							Logout();
							m_bDoLogin = true;
						}
					}
				}
			}
			continue;
		}

		if (scounter % HARMONY_PING_INTERVAL_SECONDS == 0)
		{
			//Ping the server
			if (!SendPing())
			{
				_log.Log(LOG_STATUS, "Harmony Hub: Error pinging server.. Resetting connection.");
				Logout();
			}
			continue;
		}
		bool bIsDataReadable = true;
		m_commandcsocket->canRead(&bIsDataReadable, 0.5f);
		if (bIsDataReadable)
		{
			boost::lock_guard<boost::mutex> lock(m_mutex);
			std::string strData;
			while (bIsDataReadable)
			{
				if (memset(m_databuffer, 0, BUFFER_SIZE) > 0)
				{
					m_commandcsocket->read(m_databuffer, BUFFER_SIZE, false);
					std::string szNewData = std::string(m_databuffer);
					if (!szNewData.empty())
					{
						strData.append(m_databuffer);
						m_commandcsocket->canRead(&bIsDataReadable, 0.4f);
					}
					else
						bIsDataReadable = false;
				}
				else
					bIsDataReadable = false;
			}
			if (!strData.empty())
				CheckIfChanging(strData);
		}
	}
	_log.Log(LOG_STATUS,"Harmony Hub: Worker stopped...");
}

bool CHarmonyHub::Login()
{
	csocket authorizationcsocket;
	if(!ConnectToHarmony(m_harmonyAddress, m_usIPPort, &authorizationcsocket))
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Cannot connect to Harmony Hub. Check IP/Port.");
		return false;
	}
	if(GetAuthorizationToken(&authorizationcsocket)==true)
	{
		_log.Log(LOG_STATUS,"Harmony Hub: Authentication successful");
		return true;
	}
	return false;
}

void CHarmonyHub::Logout()
{
	if(m_commandcsocket)
		delete m_commandcsocket;
	m_commandcsocket = NULL;
	m_bIsChangingActivity=false;
	m_bDoLogin=true;

}

bool CHarmonyHub::SetupCommandSocket()
{
	if(m_commandcsocket)
		Logout();

	m_commandcsocket = new csocket();


	if(!ConnectToHarmony(m_harmonyAddress, m_usIPPort,m_commandcsocket))
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Cannot setup command socket to Harmony Hub");
		return false;
	}

	std::string strUserName = m_szAuthorizationToken;
	//strUserName.append("@connect.logitech.com/gatorade.");
	std::string  strPassword = m_szAuthorizationToken;

	if(!StartCommunication(m_commandcsocket, strUserName, strPassword))
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Start communication failed");
		return false;
	}
	return true;
}

bool CHarmonyHub::UpdateActivities()
{
	if(!SubmitCommand(GET_CONFIG_COMMAND_RAW, "", ""))
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Get activities failed");
		return false;
	}

	std::map< std::string, std::string> mapActivities;

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
			mapActivities[aID] = aLabel;
		}

		std::map< std::string, std::string>::const_iterator itt;
		int cnt = 0;
		for (itt = mapActivities.begin(); itt != mapActivities.end(); ++itt)
		{
			UpdateSwitch(cnt++, itt->first.c_str(), (m_szCurActivityID == itt->first), itt->second);
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
	if(!SubmitCommand(GET_CURRENT_ACTIVITY_COMMAND_RAW, "", ""))
	{
		//_log.Log(LOG_ERROR,"Harmony Hub: Get current activity failed");
		return false;
	}

	//check if changed
	if(m_szCurActivityID!=m_szResultString)
	{
		if(!m_szCurActivityID.empty())
		{
			//need to set the old activity to off
			CheckSetActivity(m_szCurActivityID,false );
		}
		CheckSetActivity(m_szResultString,true);
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
	if (result.size() > 0) //if not yet inserted, it will be inserted active upon the next check of the activities list
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
	if(strHarmonyIPAddress.size() == 0 || harmonyPortNumber == 0 || harmonyPortNumber > 65535)
		return false;

	harmonyCommunicationcsocket->connect(strHarmonyIPAddress.c_str(), harmonyPortNumber);

	return (harmonyCommunicationcsocket->getState() == csocket::CONNECTED);
}

bool CHarmonyHub::StartCommunication(csocket* communicationcsocket, const std::string &strUserName, const std::string &strPassword)
{
	if(communicationcsocket == NULL || strUserName.size() == 0 || strPassword.size() == 0)
	{
		return false;
	} 

	// Start communication
	std::string data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), data.size());
	memset(m_databuffer, 0, BUFFER_SIZE);
	communicationcsocket->read(m_databuffer, BUFFER_SIZE, false);

	std::string strData = m_databuffer;/* <- Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='XXXXXXXX' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features> */

	data = "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"PLAIN\">";
	std::string tmp = "\0";
	tmp.append(strUserName);
	tmp.append("\0");
	tmp.append(strPassword);
	data.append(base64_encode((const unsigned char*)tmp.c_str(), tmp.size()));
	data.append("</auth>");
	communicationcsocket->write(data.c_str(), data.size());

	memset(m_databuffer, 0, BUFFER_SIZE);
	communicationcsocket->read(m_databuffer, BUFFER_SIZE, false);

	strData = m_databuffer; /* <- Expect: <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/> */
	if(strData != "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>")
	{
		//errorString = "StartCommunication : connection error";
		return false;
	} 

	data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), data.size());

	memset(m_databuffer, 0, BUFFER_SIZE);
	communicationcsocket->read(m_databuffer, BUFFER_SIZE, false);

	strData = m_databuffer; /* <- Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='057a30bd' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features> */

	return true;
}

bool CHarmonyHub::GetAuthorizationToken(csocket* authorizationcsocket)
{
	if(!StartCommunication(authorizationcsocket, "guest", "gatorade."))
	{
		//errorString = "SwapAuthorizationToken : Communication failure";
		return false;
	}

	std::string strData;
	std::string sendData;

	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.pair\">method=pair");
	sendData.append(":name=domoticz#iOS10.1.0#iPhone</oa></iq>");

	authorizationcsocket->write(sendData.c_str(), sendData.size());

	memset(m_databuffer, 0, BUFFER_SIZE);
	authorizationcsocket->read(m_databuffer, BUFFER_SIZE, false);

	strData = m_databuffer; /* <- Expect: <iq/> ... <success xmlns= ... identity=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:status=succeeded ... */

	if(strData.find("<iq/>") != 0)
	{
		//errorString = "SwapAuthorizationToken : Invalid Harmony response";
		return false;  
	}

	bool bIsDataReadable = false;
	authorizationcsocket->canRead(&bIsDataReadable, 5.0f);
	if(!bIsDataReadable && strData == "<iq/>")
	{
		bIsDataReadable = true;
	}

	while(bIsDataReadable)
	{
		memset(m_databuffer, 0, BUFFER_SIZE);
		authorizationcsocket->read(m_databuffer, BUFFER_SIZE, false);
		strData.append(m_databuffer);
		authorizationcsocket->canRead(&bIsDataReadable, 1.0f);
	};

	std::string strIdentityTokenTag = "identity=";

	// Parse the session authorization token from the response
	int pos = (int)strData.find(strIdentityTokenTag);
	if(pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a session authorization token";
		return false;  
	}

	m_szAuthorizationToken = strData.substr(pos + strIdentityTokenTag.size());

	pos = (int)m_szAuthorizationToken.find(":status=succeeded");
	if(pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a valid session authorization token";
		return false;  
	}
	m_szAuthorizationToken = m_szAuthorizationToken.substr(0, pos);

	return true;
}

bool CHarmonyHub::SendPing()
{
	int res;
	boost::lock_guard<boost::mutex> lock(m_mutex);
	if (m_commandcsocket == NULL || m_szAuthorizationToken.size() == 0)
	{
		return false;
	}
	std::string strData;
	std::string sendData;

	// GENERATE A LOGIN ID REQUEST USING THE HARMONY ID AND LOGIN AUTHORIZATION TOKEN 
	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.ping\">token=");
	sendData.append(m_szAuthorizationToken.c_str());
	sendData.append(":name=domoticz#iOS10.1.0#iPhone</oa></iq>");

	m_commandcsocket->write(sendData.c_str(), sendData.size());


	bool bIsDataReadable = true;
	memset(m_databuffer, 0, BUFFER_SIZE);
	res= m_commandcsocket->read(m_databuffer, BUFFER_SIZE, false);
	if (res <= 0)
	{
		/* there should be some bytes received so <= 0 is not good */
		return false;
	}
	strData = m_databuffer; 
	if(strData.compare("<iq/>") == 0)
	{
		/* must be new SW version 4.10.30+ so read ping confirmation */
		memset(m_databuffer, 0, BUFFER_SIZE);
		res= m_commandcsocket->read(m_databuffer, BUFFER_SIZE, false);
		if (res <= 0)
		{
			/* there should be some bytes received so <= 0 is not good */
			return false;
		}
	}
	strData = m_databuffer; /* <- Expect: strData  == 200 */
	if (strData.empty())
		return false;
	CheckIfChanging(strData);
	return (strData.find("errorcode='200'") != std::string::npos);
}

bool CHarmonyHub::SubmitCommand(const std::string &strCommand, const std::string &strCommandParameterPrimary, const std::string &strCommandParameterSecondary)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	int pos;
	if(m_commandcsocket== NULL || m_szAuthorizationToken.size() == 0)
	{
		//errorString = "SubmitCommand : NULL csocket or empty authorization token provided";
		return false;

	}

	std::string lstrCommand = strCommand;
	if(lstrCommand.size() == 0)
	{
		// No command provided, return query for the current activity
		lstrCommand = GET_CURRENT_ACTIVITY_COMMAND;
	}

	std::string strData;

	std::string sendData;

	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.harmony/vnd.logitech.harmony.engine?");

	// Issue the provided command
	if (lstrCommand == GET_CURRENT_ACTIVITY_COMMAND || lstrCommand == GET_CURRENT_ACTIVITY_COMMAND_RAW)
	{
		sendData.append("getCurrentActivity\" /></iq>");
	}
	else if (lstrCommand == GET_CONFIG_COMMAND_RAW)
	{
		sendData.append("config\"></oa></iq>");        
	}
	else if (lstrCommand == "start_activity")
	{
		sendData.append("startactivity\">activityId=");
		sendData.append(strCommandParameterPrimary.c_str());
		sendData.append(":timestamp=0</oa></iq>");
	}
	else if (lstrCommand == "issue_device_command")
	{
		sendData.append("holdAction\">action={\"type\"::\"IRCommand\",\"deviceId\"::\"");
		sendData.append(strCommandParameterPrimary.c_str());
		sendData.append("\",\"command\"::\"");
		sendData.append(strCommandParameterSecondary.c_str());
		sendData.append("\"}:status=press</oa></iq>");
	}

	m_commandcsocket->write(sendData.c_str(), sendData.size());

	bool bIsDataReadable = true;
	m_commandcsocket->canRead(&bIsDataReadable,5.0f);
	while(bIsDataReadable)
	{

		if(memset(m_databuffer, 0, BUFFER_SIZE)>0)
		{
			m_commandcsocket->read(m_databuffer, BUFFER_SIZE, false);
			std::string szNewData = std::string(m_databuffer);
			if (!szNewData.empty())
			{
				strData.append(m_databuffer);
				m_commandcsocket->canRead(&bIsDataReadable, 0.4f);
			}
			else
				bIsDataReadable = false;
		}
		else
			bIsDataReadable=false;
	}
	if (strData.empty())
		return false;

	CheckIfChanging(strData);

	if (strCommand == GET_CURRENT_ACTIVITY_COMMAND || strCommand == GET_CURRENT_ACTIVITY_COMMAND_RAW)
	{
		pos = strData.find("result=");
		if (pos != std::string::npos)
		{
			strData = strData.substr(pos + 7);
			pos = strData.find("]]>");
			if (pos != std::string::npos)
			{
				strData = strData.substr(0, pos);
				m_szResultString = strData;
				return true;
			}
		}
		else
		{
			//No valid response received
			if (m_bIsChangingActivity)
				m_szResultString = m_szCurActivityID; //changing, so no response from HH
			else
				return false;
		}
	}
	else if (strCommand == GET_CONFIG_COMMAND || strCommand == GET_CONFIG_COMMAND_RAW)
	{
		m_commandcsocket->canRead(&bIsDataReadable, 2.0f);
		while(bIsDataReadable)
		{
			memset(m_databuffer, 0, BUFFER_SIZE);
			m_commandcsocket->read(m_databuffer, BUFFER_SIZE, false);
			std::string szNewData = std::string(m_databuffer);
			if (!szNewData.empty())
			{
				strData.append(m_databuffer);
				m_commandcsocket->canRead(&bIsDataReadable, 0.4f);
			}
			else
				bIsDataReadable = false;
		}

		pos = strData.find("<![CDATA[");
		if (pos == std::string::npos)
			return false;
		strData=strData.substr(pos + 9);
		pos = strData.find("]]>");
		if (pos != std::string::npos)
		{
			m_szResultString = strData.substr(0, pos);
		}
	}
	else if (strCommand == "start_activity" || strCommand == "issue_device_command")
	{
		m_szResultString = "";
	}
	return true;
}

bool CHarmonyHub::CheckIfChanging(const std::string& strData)
{
	//activityStatus
	// 0 = Hub is off
	// 1 = Activity is starting
	// 2 = Activity is started
	// 3 = Hub is turning off

	bool bIsChanging = m_bIsChangingActivity;
	std::string LastActivity = m_szCurActivityID;

	std::string szData = strData;
	int pos;

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
			szResponse = szResponse.substr(0, pos);

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

		Json::Reader jReader;
		Json::Value root;
		bool ret = jReader.parse(szResponse, root);
		if ((!ret) || (!root.isObject()))
			continue;

		if (root["activityStatus"].empty())
			continue;

		try
		{
			int activityStatus = root["activityStatus"].asInt();
			if (!root["hubSwVersion"].empty())
			{
				std::string hubSwVersion = root["hubSwVersion"].asString();
				if (hubSwVersion != m_hubSwVersion)
				{
					m_hubSwVersion = hubSwVersion;
					_log.Log(LOG_STATUS, "Harmony Hub: Software version: %s", m_hubSwVersion.c_str());
				}
			}
			bIsChanging = (activityStatus == 1);
			if (activityStatus == 2)
			{
				if (!root["activityId"].empty())
				{
					LastActivity = root["activityId"].asString();
				}
			}
			else if (activityStatus == 3)
			{
				//Power Off
				LastActivity = "-1";
			}
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "Harmony Hub: Invalid data received! (Check Activity change, JSon activity)");
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

