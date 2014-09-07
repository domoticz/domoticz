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
  Intergration in Domoticz done by: Jan ten Hove
  
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

#define HARMONY_COMMUNICATION_PORT					5222
#define TEMP_AUTH_TOKEN								"TEMP_AUT_TOKEN"
#define LOGITECH_AUTH_URL							"https://svcs.myharmony.com/CompositeSecurityServices/Security.svc/json/GetUserAuthToken"
#define LOGITECH_AUTH_HOSTNAME						"svcs.myharmony.com"
#define LOGITECH_AUTH_PATH							"/CompositeSecurityServices/Security.svc/json/GetUserAuthToken"
#define HARMONY_HUB_AUTHORIZATION_TOKEN_FILENAME	"HarmonyHub.AuthorizationToken"
#define CONNECTION_ID								"12345678-1234-5678-1234-123456789012-1"
#define GET_CONFIG_COMMAND							"get_config_raw"
#define START_ACTIVITY_COMMAND						"start_activity"
#define GET_CURRENT_ACTIVITY_COMMAND				"get_current_activity_id_raw"
#define HARMONY_POLL_INTERVAL_SECONDS				1	//the get activity poll time
#define HARMONY_POLL_FETCH_ACTIVITY_SECONDS			60  //fetch the list of acivities every x seconds...
#define HARMONY_RETRY_LOGIN_SECONDS					60  //fetch the list of acivities every x seconds...

#define MAX_MISS_COMMANDS							5	//max commands to miss (when executing a command, the harmnoy commands may fail)

CHarmonyHub::CHarmonyHub(const int ID, const std::string &IPAddress, const unsigned int port, const std::string &userName, const std::string &password)
{
	m_userName = userName;
	m_password = password;
	m_harmonyAddress = IPAddress;
	m_usIPPort = port;
	m_HwdID=ID;
	m_commandcsocket=NULL;
	Init();
}



CHarmonyHub::~CHarmonyHub(void)
{
	Logout();
}

void CHarmonyHub::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pCmd=(tRBUF*) pdata;
	BYTE idx=0;

	if(this->m_bIsChangingActivity)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Command cannot be sent. Hub is changing activity");
		return;
	}
	//activities can be switched on
	if ((pCmd->LIGHTING2.packettype == pTypeLighting2) && (pCmd->LIGHTING2.cmnd==1))
	{
		//char szIdx[10];
		//sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, pCmd->LIGHTING2.id4);
		int lookUpId = (int)(pCmd->LIGHTING2.id1 << 24) |  (int)(pCmd->LIGHTING2.id2 << 16) | (int)(pCmd->LIGHTING2.id3 << 8) | (int)(pCmd->LIGHTING2.id4) ;
		std::stringstream sstr;
		sstr << lookUpId;

		//get the activity id from the db and send to h/w
		std::stringstream szQuery;
		std::vector<std::vector<std::string> > result;
		//szQuery << "SELECT StrParam1 FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szIdx  << "')";
		//result = m_sql.query(szQuery.str());
		//if (result.size() > 0) //should be there since it is switched on
		//{
		if (SubmitCommand(m_commandcsocket, m_szAuthorizationToken, START_ACTIVITY_COMMAND, sstr.str(), "") == 1)
		{
			_log.Log(LOG_ERROR,"Harmony Hub: Error sending the switch command");
		}			
		/*}
		else
			_log.Log(LOG_ERROR,"Harmony Hub: Device not found" );*/
	}
	else if((pCmd->LIGHTING2.packettype == pTypeLighting2) && (pCmd->LIGHTING2.cmnd==0))
	{
		if(SubmitCommand(m_commandcsocket, m_szAuthorizationToken, START_ACTIVITY_COMMAND, "PowerOff","") == 1)
		{
			_log.Log(LOG_ERROR,"Harmony Hub: Error sending the poweroff command");
		}			
	}
}

void CHarmonyHub::Init()
{
	m_stoprequested = false;
	m_bDoLogin = true;
	m_szCurActivityID="";
	m_usCommandsMissed=0;
	m_bIsChangingActivity=false;
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
	return true;
}


void CHarmonyHub::Do_Work()
{
	_log.Log(LOG_STATUS,"Harmony Hub: Worker started...");
	//start with getting the activities
	unsigned short checkAct=HARMONY_POLL_FETCH_ACTIVITY_SECONDS /  HARMONY_POLL_INTERVAL_SECONDS;
	
	while (!m_stoprequested)
	{
		sleep_seconds(HARMONY_POLL_INTERVAL_SECONDS);
		checkAct++;

		//check the active activity
		unsigned short checkLogin=HARMONY_RETRY_LOGIN_SECONDS /  HARMONY_POLL_INTERVAL_SECONDS;
		while(m_bDoLogin && !m_stoprequested)
		{
			if (HARMONY_RETRY_LOGIN_SECONDS  <= checkLogin)
			{
				checkLogin=0;
				if(Login() && SetupCommandSocket())
				{
					m_bDoLogin=false;
					break;
				}
			}
			else
			{
				mytime(&m_LastHeartbeat);
				sleep_seconds(1);
				checkLogin++;
			}			
		}

		//extra check after sleep if we must not quit right away
		if(m_stoprequested)
			break;

		//check/update the active activity
		if(!UpdateCurrentActivity())
			m_usCommandsMissed++;
		else
			m_usCommandsMissed=0;


		//check for activities change. Update our heartbeat too.
		if (HARMONY_POLL_FETCH_ACTIVITY_SECONDS /  HARMONY_POLL_INTERVAL_SECONDS <= checkAct) {
			if(!UpdateActivities())
				m_usCommandsMissed++;
			else
				m_usCommandsMissed=0;
			checkAct=0;
		}
		//and set the heartbeat while we're at it
		mytime(&m_LastHeartbeat);

		if(m_usCommandsMissed>=MAX_MISS_COMMANDS)
		{
			Logout();
			_log.Log(LOG_STATUS,"Harmony Hub: Too many commands missed. Resetting connection.");
			//m_bDoLogin=true;	//re login
		}
			
	}
	_log.Log(LOG_STATUS,"Harmony Hub: Worker stopped...");
}

bool CHarmonyHub::Login()
{
	// Read the token
	int val;
	bool bAuthorizationComplete = false;
	if(m_sql.GetTempVar(TEMP_AUTH_TOKEN, val, m_szAuthorizationToken))
	{

		//printf("\nLogin Authorization Token is: %s\n\n", m_szAuthorizationToken.c_str());
		if(m_szAuthorizationToken.length() > 0)
		{
			csocket authorizationcsocket;
			if(ConnectToHarmony(m_harmonyAddress, m_usIPPort, &authorizationcsocket) == 1)
			{
				_log.Log(LOG_ERROR,"Harmony Hub: Cannot connect to Harmony Hub");
				//printf("ERROR : %s\n", errorString.c_str());
				return false;
			}

			if(SwapAuthorizationToken(&authorizationcsocket, m_szAuthorizationToken) == 0)
			{
				// Authorization Token found worked.  
				// Bypass authorization through Logitech's servers.
				_log.Log(LOG_STATUS,"Harmony Hub: Authentication succesfull");
				return true;
			}

		}
	}

	if(bAuthorizationComplete == false)
	{
		// Log into the Logitech Web Service to retrieve the login authorization token
		if(HarmonyWebServiceLogin(m_userName, m_password, m_szAuthorizationToken) == 1)
		{
			_log.Log(LOG_ERROR,"Harmony Hub: Logitech web service login failed.");
			return false;
		}
		_log.Log(LOG_STATUS,"Harmony Hub: Logged in to the Logitech web service");

		//printf("\nLogin Authorization Token is: %s\n\n", m_szAuthorizationToken.c_str());

		// Write the Authorization Token to an Authorization Token file to bypass this step
		// on future sessions
		m_sql.UpdateTempVar(TEMP_AUTH_TOKEN, m_szAuthorizationToken.c_str());

		csocket authorizationcsocket;
		if(ConnectToHarmony(m_harmonyAddress, m_usIPPort, &authorizationcsocket) == 1)
		{
			_log.Log(LOG_ERROR,"Cannot connect to Harmony Hub");
			//printf("ERROR : %s\n", errorString.c_str());
			return false;
		}

		if(SwapAuthorizationToken(&authorizationcsocket, m_szAuthorizationToken) == 0)
		{
			// Authorization Token found worked.  
			// Bypass authorization through Logitech's servers.
			_log.Log(LOG_STATUS,"Harmony Hub: Authentication succesfull");
			return true;
		}
		return false;
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
	

	if(ConnectToHarmony(m_harmonyAddress, m_usIPPort,m_commandcsocket) == 1)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Cannot setup command socket to Harmony Hub");
		return false;
	}

	std::string strUserName = m_szAuthorizationToken;
	//strUserName.append("@connect.logitech.com/gatorade.");
	std::string  strPassword = m_szAuthorizationToken;

	if(StartCommunication(m_commandcsocket, strUserName, strPassword) == 1)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Start communication failed");
		return false;
	}
	return true;
}

bool CHarmonyHub::UpdateActivities()
{
	if(SubmitCommand(m_commandcsocket, m_szAuthorizationToken, GET_CONFIG_COMMAND, "", "") == 1)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Get activities failed");
		return false;
	}

	std::map< std::string, std::string> mapActivities;
	std::vector< Device > vecDevices;
	if(ParseConfiguration(m_szResultString, mapActivities, vecDevices) == 1)
	{
		_log.Log(LOG_ERROR,"Harmony Hub: Parse activities and devices failed");
		return false;
	}

	std::map< std::string, std::string>::iterator it = mapActivities.begin();
	std::map< std::string, std::string>::iterator ite = mapActivities.end();
	int cnt=0;
	for(; it != ite; ++it)
	{
		UpdateSwitch(cnt++, it->first.c_str(), strcmp(m_szCurActivityID.c_str(), it->first.c_str())==0, it->second);
		/*m_szResultString.append(it->first);
		m_szResultString.append(" - ");
		m_szResultString.append(it->second);
		m_szResultString.append("\n");*/

	}
	return true;
}

bool CHarmonyHub::UpdateCurrentActivity()
{
	if(SubmitCommand(m_commandcsocket, m_szAuthorizationToken, GET_CURRENT_ACTIVITY_COMMAND, "", "") == 1)
	{
		//_log.Log(LOG_ERROR,"Harmony Hub: Get current activity failed");
		return false;
	}
	//check if changed
	if(!strcmp(m_szCurActivityID.c_str(), m_szResultString.c_str())==0)
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
	//get the device id from the db (if alread inserted)
	int actId=atoi(activityID.c_str());
	std::stringstream hexId ;
	hexId << std::setw(7)  << std::hex << std::setfill('0') << std::uppercase << (int)( actId) ;
	std::string actHex = hexId.str();
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,DeviceID FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << actHex << "')";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() > 0) //if not yet inserted, it will be inserted active upon the next check of the activities list
	{
		UpdateSwitch(atoi(result[0][1].c_str()), activityID.c_str(),on,result[0][0]);

	}
}

void CHarmonyHub::UpdateSwitch(unsigned char idx,const char * realID, const bool bOn, const std::string &defaultname)
{
	bool bDeviceExits = true;
	std::stringstream hexId ;
	hexId << std::setw(7) << std::setfill('0') << std::hex << std::uppercase << (int)( atoi(realID) );
	//char szIdx[10];
	//sprintf(szIdx, "%X%02X%02X%02X", 0, 0, 0, idx);
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << hexId.str() << "')";
	result = m_sql.query(szQuery.str()); //-V519
	if (result.size() < 1)
	{
		bDeviceExits = false;
	}
	else
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == 0))
			return;
		if ((bOn && (nvalue != 0)))
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
	/*lcmd.LIGHTING2.id2 = 0;
	lcmd.LIGHTING2.id3 = 0;
	lcmd.LIGHTING2.id4 = idx;*/
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2);//decode message

	if (!bDeviceExits)
	{
		//Assign default name for device
		szQuery.clear();
		szQuery.str("");
		szQuery << "UPDATE DeviceStatus SET Name='" << defaultname << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << hexId.str() << "')";
		result = m_sql.query(szQuery.str());
	}
}

//  Logs into the Logitech Harmony web service
//  Returns a base64-encoded string containing a 48-byte Login Token in the third parameter
int CHarmonyHub::HarmonyWebServiceLogin(std::string strUserEmail, std::string strPassword, std::string& m_szAuthorizationToken )
{
	if(strUserEmail.length() == 0 || strPassword.length() == 0)
	{
		//errorString = "HarmonyWebServiceLogin : Empty email or password provided";
		return 1;
	} 


	// Build JSON request
	std::string strJSONText = "{\"email\":\"";
	strJSONText.append(strUserEmail.c_str());
	strJSONText.append("\",\"password\":\"");
	strJSONText.append(strPassword.c_str());
	strJSONText.append("\"}");

	std::string strHttpPayloadText;

	csocket authcsocket;
	authcsocket.connect(LOGITECH_AUTH_HOSTNAME, 80);

	if (authcsocket.getState() != csocket::CONNECTED)
	{
		//errorString = "HarmonyWebServiceLogin : Unable to connect to Logitech server";
		return 1;
	}

	char contentLength[32];
	sprintf( contentLength, "%d", strJSONText.length() );

	std::string strHttpRequestText;

	strHttpRequestText = "POST ";
	strHttpRequestText.append(LOGITECH_AUTH_URL);
	strHttpRequestText.append(" HTTP/1.1\r\nHost: ");
	strHttpRequestText.append(LOGITECH_AUTH_HOSTNAME);
	strHttpRequestText.append("\r\nAccept-Encoding: identity\r\nContent-Length: ");
	strHttpRequestText.append(contentLength);
	strHttpRequestText.append("\r\ncontent-type: application/json;charset=utf-8\r\n\r\n");

	authcsocket.write(strHttpRequestText.c_str(), strHttpRequestText.size());
	authcsocket.write(strJSONText.c_str(), strJSONText.length());

	memset(m_databuffer, 0, 1000000);
	authcsocket.read(m_databuffer, 1000000, false);
	strHttpPayloadText = m_databuffer;/* <- Expect: 0x00def280 "HTTP/1.1 200 OK Server: nginx/1.2.4 Date: Wed, 05 Feb 2014 17:52:13 GMT Content-Type: application/json; charset=utf-8 Content-Length: 127 Connection: keep-alive Cache-Control: private X-AspNet-Version: 4.0.30319 X-Powered-By: ASP.NET  {"GetUserAuthTokenResult":{"AccountId":0,"UserAuthToken":"KsRE6VVA3xrhtbqFbh0jWn8YTiweDeB\/b94Qeqf3ofWGM79zLSr62XQh8geJxw\/V"}}"*/

	// Parse the login authorization token from the response
	std::string strAuthTokenTag = "UserAuthToken\":\"";
	int pos = (int)strHttpPayloadText.find(strAuthTokenTag);
	if(pos == std::string::npos)
	{
		//errorString = "HarmonyWebServiceLogin : Logitech web service response does not contain a login authorization token";
		return 1;  
	}

	m_szAuthorizationToken = strHttpPayloadText.substr(pos + strAuthTokenTag.length());
	pos = (int)m_szAuthorizationToken.find("\"}}");
	m_szAuthorizationToken = m_szAuthorizationToken.substr(0, pos);

	// Remove forward slashes
	m_szAuthorizationToken.erase(std::remove(m_szAuthorizationToken.begin(), m_szAuthorizationToken.end(), '\\'), m_szAuthorizationToken.end());
	return 0;
}

int CHarmonyHub::ConnectToHarmony(const std::string &strHarmonyIPAddress, const int harmonyPortNumber, csocket* harmonyCommunicationcsocket)
{
	if(strHarmonyIPAddress.length() == 0 || harmonyPortNumber == 0 || harmonyPortNumber > 65535)
	{
		//errorString = "ConnectToHarmony : Empty Harmony IP Address or Port";
		return 1;
	}

	harmonyCommunicationcsocket->connect(strHarmonyIPAddress.c_str(), harmonyPortNumber);

	if (harmonyCommunicationcsocket->getState() != csocket::CONNECTED)
	{
		//errorString = "ConnectToHarmony : Unable to connect to specified IP Address on specified Port";
		return 1;
	}

	return 0;
}

int CHarmonyHub::StartCommunication(csocket* communicationcsocket, const std::string &strUserName, const std::string &strPassword)
{
	if(communicationcsocket == NULL || strUserName.length() == 0 || strPassword.length() == 0)
	{
		//errorString = "StartCommunication : Invalid communication parameter(s) provided";
		return 1;
	} 

	// Start communication
	std::string data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), data.length());
	memset(m_databuffer, 0, 1000000);
	communicationcsocket->read(m_databuffer, 1000000, false);

	std::string strData = m_databuffer;/* <- Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='XXXXXXXX' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features> */

	data = "<auth xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" mechanism=\"PLAIN\">";
	std::string tmp = "\0";
	tmp.append(strUserName);
	tmp.append("\0");
	tmp.append(strPassword);
	data.append(base64_encode((const unsigned char*)tmp.c_str(), tmp.length()));
	data.append("</auth>");
	communicationcsocket->write(data.c_str(), data.length());

	memset(m_databuffer, 0, 1000000);
	communicationcsocket->read(m_databuffer, 1000000, false);

	strData = m_databuffer; /* <- Expect: <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/> */
	if(strData != "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>")
	{
		//errorString = "StartCommunication : connection error";
		return 1;
	} 

	data = "<stream:stream to='connect.logitech.com' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' xml:lang='en' version='1.0'>";
	communicationcsocket->write(data.c_str(), data.length());

	memset(m_databuffer, 0, 1000000);
	communicationcsocket->read(m_databuffer, 1000000, false);

	strData = m_databuffer; /* <- Expect: <?xml version='1.0' encoding='iso-8859-1'?><stream:stream from='' id='057a30bd' version='1.0' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'><stream:features><mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'><mechanism>PLAIN</mechanism></mechanisms></stream:features> */

	return 0;
}

int CHarmonyHub::SwapAuthorizationToken(csocket* authorizationcsocket, std::string& m_szAuthorizationToken)
{
	if(authorizationcsocket == NULL || m_szAuthorizationToken.length() == 0)
	{
		//errorString = "SwapAuthorizationToken : NULL csocket or empty authorization token provided";
		return 1;
	}

	if(StartCommunication(authorizationcsocket, "guest", "gatorade.") != 0)
	{
		//errorString = "SwapAuthorizationToken : Communication failure";
		return 1;
	}

	std::string strData;
	std::string sendData;

	// GENERATE A LOGIN ID REQUEST USING THE HARMONY ID AND LOGIN AUTHORIZATION TOKEN 
	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.connect/vnd.logitech.pair\">token=");
	sendData.append(m_szAuthorizationToken.c_str());
	sendData.append(":name=foo#iOS6.0.1#iPhone</oa></iq>");

	std::string strIdentityTokenTag = "identity=";
	int pos = std::string::npos;

	authorizationcsocket->write(sendData.c_str(), sendData.length());

	memset(m_databuffer, 0, 1000000);
	authorizationcsocket->read(m_databuffer, 1000000, false);

	strData = m_databuffer; /* <- Expect: <iq/> ... <success xmlns= ... identity=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:status=succeeded ... */

	if(strData.find("<iq/>") != 0)
	{
		//errorString = "SwapAuthorizationToken : Invalid Harmony response";
		return 1;  
	}

	bool bIsDataReadable = false;
	authorizationcsocket->canRead(&bIsDataReadable, 0.3f);
	if(!bIsDataReadable && strData == "<iq/>")
	{
		bIsDataReadable = true;
	}

	while(bIsDataReadable)
	{
		memset(m_databuffer, 0, 1000000);
		authorizationcsocket->read(m_databuffer, 1000000, false);
		strData.append(m_databuffer);
		authorizationcsocket->canRead(&bIsDataReadable, 0.3f);
	};

	// Parse the session authorization token from the response
	pos = (int)strData.find(strIdentityTokenTag);
	if(pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a session authorization token";
		return 1;  
	}

	m_szAuthorizationToken = strData.substr(pos + strIdentityTokenTag.length());

	pos = (int)m_szAuthorizationToken.find(":status=succeeded");
	if(pos == std::string::npos)
	{
		//errorString = "SwapAuthorizationToken : Logitech Harmony response does not contain a valid session authorization token";
		return 1;  
	}
	m_szAuthorizationToken = m_szAuthorizationToken.substr(0, pos);

	return 0;
}


int CHarmonyHub::SubmitCommand(csocket* m_commandcsocket, const std::string& m_szAuthorizationToken, const std::string strCommand, const std::string strCommandParameterPrimary, const std::string strCommandParameterSecondary)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);

	if(m_commandcsocket== NULL || m_szAuthorizationToken.length() == 0)
	{
		//errorString = "SubmitCommand : NULL csocket or empty authorization token provided";
		return 1;
	}

	std::string lstrCommand = strCommand;
	if(lstrCommand.length() == 0)
	{
		// No command provided, return query for the current activity
		lstrCommand = "get_current_activity_id";
		return 0;
	}

	std::string strData;

	std::string sendData;

	sendData = "<iq type=\"get\" id=\"";
	sendData.append(CONNECTION_ID);
	sendData.append("\"><oa xmlns=\"connect.logitech.com\" mime=\"vnd.logitech.harmony/vnd.logitech.harmony.engine?");

	// Issue the provided command
	if(lstrCommand == "get_current_activity_id" || lstrCommand == "get_current_activity_id_raw")
	{
		sendData.append("getCurrentActivity\" /></iq>");
	}
	if(lstrCommand == "get_config_raw")
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

	m_commandcsocket->write(sendData.c_str(), sendData.length());

	memset(m_databuffer, 0, 1000000);
	m_commandcsocket->read(m_databuffer, 1000000, false);
	strData = m_databuffer; /* <- Expect: strData  == <iq/> */

	std::string iqTag = "<iq/>";
	int pos = (int)strData.find(iqTag);

	bool bIsDataReadable = false;
	bool wasChanged =false;
	while(pos != 0)
	{
		wasChanged = CheckIfChanging(strData);
		m_commandcsocket->canRead(&bIsDataReadable, 0.6f);
		if(bIsDataReadable)
		{
			//activity changing
			//read again
			memset(m_databuffer, 0, 1000000);
			m_commandcsocket->read(m_databuffer, 1000000, false);
			if(strlen(m_databuffer)==0)
				return 1;
			strData.append ( m_databuffer); /* <- Expect: strData  == <iq/> */
			pos = (int)strData.find(iqTag);
		}
		else if(wasChanged)
			return 0;
		else
		{
			//printf(strData.c_str());
			//errorString = "SubmitCommand: Invalid Harmony response";
			return 1;  
		}
	}

	CheckIfChanging(strData);
	
	m_commandcsocket->canRead(&bIsDataReadable, 0.6f);

	if(bIsDataReadable == false && strData == "<iq/>")
	{
		bIsDataReadable = true;
	}

	if(strCommand != "issue_device_command")
	{
		while(bIsDataReadable)
		{
			memset(m_databuffer, 0, 1000000);
			m_commandcsocket->read(m_databuffer, 1000000, false);
			if(strlen(m_databuffer)==0)
				return 1;
			strData.append(m_databuffer);
			m_commandcsocket->canRead(&bIsDataReadable, 0.3f);
		}
	}

	m_szResultString = strData;

	//check for the presence of startactivity or activityfinished
	//strData.find("startActivityFinished")!=0 || 
	CheckIfChanging(strData);
	


	if(strCommand == "get_current_activity_id" || strCommand == "get_current_activity_id_raw")
	{
		int resultStartPos = m_szResultString.find("result=");
		int resultEndPos = m_szResultString.find("]]>",resultStartPos);
		if(resultStartPos != std::string::npos && resultEndPos != std::string::npos)
		{
			m_szResultString = m_szResultString.substr(resultStartPos + 7, resultEndPos - resultStartPos - 7);
			if(m_szResultString.find("]]") != std::string::npos)
				_log.Log(LOG_STATUS,"Error");
			if(strCommand == "get_current_activity_id")
			{
				m_szResultString.insert(0, "Current Activity ID is : ");
			}
		}
		else
			m_szResultString="";
	}
	else if(strCommand == "get_config" || strCommand == "get_config_raw")
	{
		m_commandcsocket->canRead(&bIsDataReadable, 0.3f);

#ifndef WIN32
		bIsDataReadable = true;
#endif

		while(bIsDataReadable)
		{
			memset(m_databuffer, 0, 1000000);
			m_commandcsocket->read(m_databuffer, 1000000, false);

			strData.append(m_databuffer);
			m_commandcsocket->canRead(&bIsDataReadable, 0.3f);
		}


		pos = strData.find("![CDATA[{");
		if(pos != std::string::npos)
		{
			m_szResultString = "Logitech Harmony Configuration : \n" + strData.substr(pos + 9);
		}
	}
	else if (strCommand == "start_activity" || strCommand == "issue_device_command")
	{
		m_szResultString = "";
	}
	return 0;
}

bool CHarmonyHub::CheckIfChanging(const std::string& strData)
{
	//strdata might contain both, so can return when both checks are done
	bool newVal=m_bIsChangingActivity;
	int lastDetect=0;
	while(lastDetect!= std::string::npos)
	{
		int tempDetect=lastDetect = strData.find("connect.stateDigest?notify",lastDetect);
		if(tempDetect!=std::string::npos)
		{
			lastDetect = tempDetect;
			newVal=true;
		}

		lastDetect = strData.find("startActivityFinished",lastDetect);
		if(lastDetect!=std::string::npos)
			newVal=false;
		
	}
	if(newVal != m_bIsChangingActivity)
	{
		m_bIsChangingActivity=newVal;
		if(newVal)
			_log.Log(LOG_STATUS,"Harmony Hub is changing activity");
		else
			_log.Log(LOG_STATUS,"Harmony Hub finished changing activity");
		return true;
	}
	
	return false;
/*
	if((strData.find("startActivityFinished",changeDetect) != std::string::npos)&&m_bIsChangingActivity)
	{
		m_bIsChangingActivity=false;
		_log.Log(LOG_STATUS,"Harmony Hub finished changing activity");
		retVal = true;
	}
	return retVal;*/
}

int CHarmonyHub::ParseAction(const std::string& strAction, std::vector<Action>& vecDeviceActions, const std::string& strDeviceID)
{
	Action a;
	const std::string commandTag = "\\\"command\\\":\\\"";
	int commandStart = strAction.find(commandTag);
	int commandEnd = strAction.find("\\\",\\\"", commandStart);
	a.m_strCommand = strAction.substr(commandStart + commandTag.length(), commandEnd - commandStart - commandTag.length());

	const std::string deviceIdTag = "\\\"deviceId\\\":\\\"";
	int deviceIDStart = strAction.find(deviceIdTag, commandEnd);

	const std::string nameTag = "\\\"}\",\"name\":\"";
	int deviceIDEnd = strAction.find(nameTag, deviceIDStart);

	std::string commandDeviceID = strAction.substr(deviceIDStart + deviceIdTag.length(), deviceIDEnd - deviceIDStart - deviceIdTag.length());
	if(commandDeviceID != strDeviceID)
	{
		return 1;
	}

	int nameStart = deviceIDEnd + nameTag.length();

	const std::string labelTag = "\",\"label\":\"";
	int nameEnd = strAction.find(labelTag, nameStart);

	a.m_strName = strAction.substr(nameStart, nameEnd - nameStart);

	int labelStart = nameEnd + labelTag.length();
	int labelEnd = strAction.find("\"}", labelStart);

	a.m_strLabel = strAction.substr(labelStart, labelEnd - labelStart);

	vecDeviceActions.push_back(a);
	return 0;
}

int CHarmonyHub::ParseFunction(const std::string& strFunction, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID)
{
	Function f;
	int functionNameEnd = strFunction.find("\",\"function\":[{");
	if(functionNameEnd == std::string::npos)
	{
		return 1;
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
			return 1;
		}
		actionEnd = strFunction.find("\"}", actionEnd + labelTag.length());

		std::string strAction = strFunction.substr(actionStart + actionTag.length(), actionEnd - actionStart - actionTag.length());
		ParseAction(strAction, f.m_vecActions, strDeviceID);

		actionStart = strFunction.find(actionTag, actionEnd);
	}

	vecDeviceFunctions.push_back(f);

	return 0;
}

int CHarmonyHub::ParseControlGroup(const std::string& strControlGroup, std::vector<Function>& vecDeviceFunctions, const std::string& strDeviceID)
{
	const std::string nameTag = "{\"name\":\"";
	int funcStartPos = strControlGroup.find(nameTag);
	int funcEndPos = strControlGroup.find("]}");
	while(funcStartPos != std::string::npos)
	{
		std::string strFunction = strControlGroup.substr(funcStartPos + nameTag.length(), funcEndPos - funcStartPos - nameTag.length());
		if(ParseFunction(strFunction, vecDeviceFunctions, strDeviceID) != 0)
		{
			return 1;
		}
		funcStartPos = strControlGroup.find(nameTag, funcEndPos);
		funcEndPos = strControlGroup.find("}]}", funcStartPos);
	}

	return 0;
}

int CHarmonyHub::ParseConfiguration(const std::string& strConfiguration, std::map< std::string, std::string >& mapActivities, std::vector< Device >& vecDevices)
{
	std::string activityTypeDisplayNameTag = "\",\"activityTypeDisplayName\":\"";
	int activityTypeDisplayNameStartPos = strConfiguration.find(activityTypeDisplayNameTag);
	while(activityTypeDisplayNameStartPos != std::string::npos)
	{
		int activityStart = strConfiguration.rfind("{", activityTypeDisplayNameStartPos);
		if(activityStart != std::string::npos )
		{
			std::string activityString = strConfiguration.substr(activityStart+1, activityTypeDisplayNameStartPos - activityStart-1);

			std::string labelTag = "\"label\":\"";
			std::string idTag = "\",\"id\":\"";
			int labelStartPos = activityString.find(labelTag);
			int idStartPos = activityString.find(idTag, labelStartPos);

			// Try to pick up the label
			std::string strActivityLabel = activityString.substr(labelStartPos+9, idStartPos-labelStartPos-9);
			idStartPos += idTag.length();

			// Try to pick up the ID
			std::string strActivityID = activityString.substr(idStartPos, activityString.length() - idStartPos);

			mapActivities.insert(std::map< std::string, std::string>::value_type(strActivityID, strActivityLabel));
		}
		activityTypeDisplayNameStartPos = strConfiguration.find(activityTypeDisplayNameTag, activityTypeDisplayNameStartPos+activityTypeDisplayNameTag.length());
	}

	// Search for devices and commands
	std::string deviceDisplayNameTag = "deviceTypeDisplayName";
	int deviceTypeDisplayNamePos = strConfiguration.find(deviceDisplayNameTag);
	while(deviceTypeDisplayNamePos != std::string::npos && deviceTypeDisplayNamePos != strConfiguration.length())
	{
		//std::string deviceString = strConfiguration.substr(deviceTypeDisplayNamePos);
		int nextDeviceTypeDisplayNamePos = strConfiguration.find(deviceDisplayNameTag, deviceTypeDisplayNamePos + deviceDisplayNameTag.length());

		if(nextDeviceTypeDisplayNamePos == std::string::npos)
		{
			nextDeviceTypeDisplayNamePos = strConfiguration.length();
		}

		Device d;

		// Search for commands
		const std::string controlGroupTag = ",\"controlGroup\":[";
		const std::string controlPortTag = "],\"ControlPort\":";
		int controlGroupStartPos = strConfiguration.find(controlGroupTag, deviceTypeDisplayNamePos);
		int controlGroupEndPos = strConfiguration.find(controlPortTag, controlGroupStartPos + controlGroupTag.length());
		int deviceStartPos = strConfiguration.rfind("{", deviceTypeDisplayNamePos);
		int deviceEndPos = strConfiguration.find("}", controlGroupEndPos);

		if(deviceStartPos != std::string::npos && deviceEndPos != std::string::npos)
		{
			// Try to pick up the ID
			const std::string idTag = "\",\"id\":\"";
			int idStartPos = strConfiguration.find(idTag, deviceStartPos);
			if(idStartPos != std::string::npos && idStartPos < deviceEndPos)
			{
				int idEndPos = strConfiguration.find("\",\"", idStartPos + idTag.length());
				d.m_strID = strConfiguration.substr(idStartPos+idTag.length(), idEndPos-idStartPos-idTag.length());
			}
			else
			{
				deviceTypeDisplayNamePos = nextDeviceTypeDisplayNamePos ;
				continue;
			}

			// Definitely have a device

			// Try to pick up the label
			const std::string labelTag = "\"label\":\"";
			int labelStartPos = strConfiguration.find(labelTag, deviceStartPos);
			if(labelStartPos != std::string::npos && labelStartPos < deviceEndPos)
			{
				int labelEndPos = strConfiguration.find("\",\"", labelStartPos + labelTag.length());
				d.m_strLabel = strConfiguration.substr(labelStartPos + labelTag.length(), labelEndPos-labelStartPos - labelTag.length());
			}

			// Try to pick up the type
			std::string typeTag = ",\"type\":\"";
			int typeStartPos = strConfiguration.find(typeTag, deviceStartPos);
			if(typeStartPos != std::string::npos && typeStartPos < deviceEndPos)
			{
				int typeEndPos = strConfiguration.find("\",\"", typeStartPos + typeTag.length());
				d.m_strType = strConfiguration.substr(typeStartPos + typeTag.length(), typeEndPos - typeStartPos - typeTag.length());
			}

			// Try to pick up the manufacturer
			std::string manufacturerTag = "manufacturer\":\"";
			int manufacturerStartPos = strConfiguration.find(manufacturerTag, deviceStartPos);
			if(manufacturerStartPos != std::string::npos && manufacturerStartPos < deviceEndPos)
			{
				int manufacturerEndPos = strConfiguration.find("\",\"", manufacturerStartPos + manufacturerTag.length());
				d.m_strManufacturer = strConfiguration.substr(manufacturerStartPos+15, manufacturerEndPos-manufacturerStartPos-manufacturerTag.length());
			}

			// Try to pick up the model
			std::string modelTag = "model\":\"";
			int modelStartPos = strConfiguration.find(modelTag, deviceStartPos);
			if(modelStartPos != std::string::npos && modelStartPos < deviceEndPos)
			{
				int modelEndPos = strConfiguration.find("\",\"", modelStartPos + modelTag.length());
				d.m_strModel = strConfiguration.substr(modelStartPos+modelTag.length(), modelEndPos-modelStartPos-modelTag.length());
			}

			// Parse Commands
			std::string strControlGroup = strConfiguration.substr(controlGroupStartPos + controlGroupTag.length(), controlGroupEndPos - controlGroupStartPos - controlGroupTag.length());
			ParseControlGroup(strControlGroup, d.m_vecFunctions, d.m_strID);

			vecDevices.push_back(d);
		}
		deviceTypeDisplayNamePos = nextDeviceTypeDisplayNamePos;
	}
	return 0;
}


