#include "stdafx.h"
#include "LogitechMediaServer.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"

CLogitechMediaServer::CLogitechMediaServer(const int ID, const std::string IPAddress, const int Port, const int PollIntervalsec, const int PingTimeoutms) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_IP = IPAddress;
	m_Port = Port;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CLogitechMediaServer::CLogitechMediaServer(const int ID, const std::string IPAddress, const int Port) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_IP = IPAddress;
	m_Port = Port;
	SetSettings(10, 3000);
}

CLogitechMediaServer::CLogitechMediaServer(const int ID) : m_stoprequested(false), m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_IP = "";
	m_Port = 0;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Address, Port FROM Hardware WHERE ID==%d", m_HwdID);

	if (result.size() > 0)
	{
		m_IP = result[0][0];
		m_Port = atoi(result[0][1].c_str());
	}

	SetSettings(10, 3000);
}

CLogitechMediaServer::~CLogitechMediaServer(void)
{
	m_bIsStarted = false;
	m_bShowedUnsupported = false;
}

Json::Value CLogitechMediaServer::Query(std::string sIP, int iPort, std::string sPostdata)
{
	Json::Value root;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sURL;
	std::stringstream sPostData;

	sURL << "http://" << sIP << ":" << iPort << "/jsonrpc.js";
	sPostData << sPostdata;
	HTTPClient::SetTimeout(m_iPingTimeoutms / 1000);
	bool bRetVal = HTTPClient::POST(sURL.str(), sPostData.str(), ExtraHeaders, sResult);

	if (!bRetVal)
	{
		return root;
	}
	Json::Reader jReader;
	bRetVal = jReader.parse(sResult, root);
	if (!bRetVal)
	{
		_log.Log(LOG_ERROR, "Logitech Media Server: PARSE ERROR: %s", sResult.c_str());
		return root;
	}
	if (root["method"].empty())
	{
		_log.Log(LOG_ERROR, "Logitech Media Server: '%s' request '%s'", sURL.str().c_str(), sPostData.str().c_str());
		return root;
	}
	return root["result"];
}

bool CLogitechMediaServer::StartHardware()
{
	StopHardware();
	m_bIsStarted = true;
	sOnConnected(this);
	m_iThreadsRunning = 0;

	StartHeartbeatThread();

	ReloadNodes();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLogitechMediaServer::Do_Work, this)));

	return (m_thread != NULL);
}

bool CLogitechMediaServer::StopHardware()
{
	StopHeartbeatThread();

	try {
		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread.reset();

			//Make sure all our background workers are stopped
			int iRetryCounter = 0;
			while ((m_iThreadsRunning > 0) && (iRetryCounter<15))
			{
				sleep_milliseconds(500);
				iRetryCounter++;
			}
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	m_bIsStarted = false;
	return true;
}

void CLogitechMediaServer::UpdateNodeStatus(const LogitechMediaServerNode &Node, const _eMediaStatus nStatus, const std::string sStatus, bool bPingOK)
{
	//Find out node, and update it's status
	std::vector<LogitechMediaServerNode>::iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->ID == Node.ID)
		{
			//Found it
			bool	bUseOnOff = false;
			if (((nStatus == MSTAT_OFF) && bPingOK) || ((nStatus != MSTAT_OFF) && !bPingOK)) bUseOnOff = true;
			time_t atime = mytime(NULL);
			itt->LastOK = atime;
			if ((itt->nStatus != nStatus) || (itt->sStatus != sStatus))
			{
				// 1:	Update the DeviceStatus
				if (nStatus == MSTAT_ON)
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s - '%s'", Node.Name.c_str(), Media_Player_States(nStatus), sStatus.c_str());
				else
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s", Node.Name.c_str(), Media_Player_States(nStatus));
				struct tm ltime;
				localtime_r(&atime, &ltime);
				char szLastUpdate[40];
				sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
					int(nStatus), sStatus.c_str(), szLastUpdate, m_HwdID, itt->szDevID, STYPE_Media);

				// 2:	Log the event if the actual status has changed
				std::string sShortStatus = sStatus;
				if ((itt->nStatus != nStatus) || (itt->sShortStatus != sShortStatus))
				{
					std::string sLongStatus = Media_Player_States(nStatus);
					if (sShortStatus.length()) sLongStatus += " - " + sShortStatus;
					result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue) VALUES (%d, %d, '%q')", itt->ID, int(nStatus), sLongStatus.c_str());
				}

				// 3:	Trigger On/Off actions
				if (bUseOnOff)
				{
					result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (ID = '%q') AND (Unit == 1)", m_HwdID, itt->szDevID);
					if (result.size() > 0)
					{
						m_sql.HandleOnOffAction(bPingOK, result[0][0], result[0][1]);
					}
				}

				itt->nStatus = nStatus;
				itt->sStatus = sStatus;
				itt->sShortStatus = sShortStatus;
			}
			break;
		}
	}
}

void CLogitechMediaServer::Do_Node_Work(const LogitechMediaServerNode &Node)
{
	bool bPingOK = false;
	_eMediaStatus nStatus = MSTAT_UNKNOWN;
	std::string	sPlayerId = Node.IP;
	std::string	sStatus = "";

	try
	{
		std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[\"status\",\"-\",1,\"tags:Aadly\"]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);

		if (!root.size())
			nStatus = MSTAT_OFF;
		else
		{
			bPingOK = true;

			if (root["player_connected"].asString() == "1")
			{
				if (root["power"].asString() == "0")
					nStatus = MSTAT_OFF;
				else {
					nStatus = MSTAT_ON;
					std::string sMode = root["mode"].asString();
					std::string	sTitle = "";
					std::string	sAlbum = "";
					std::string	sArtist = "";
					std::string	sAlbumArtist = "";
					std::string	sTrackArtist = "";
					std::string	sYear = "";
					std::string	sDuration = "";
					std::string sLabel = "";

					if (root["playlist_loop"].size()) {
						sTitle = root["playlist_loop"][0]["title"].asString();
						sAlbum = root["playlist_loop"][0]["album"].asString();
						sArtist = root["playlist_loop"][0]["artist"].asString();
						sAlbumArtist = root["playlist_loop"][0]["albumartist"].asString();
						sTrackArtist = root["playlist_loop"][0]["trackartist"].asString();
						sYear = root["playlist_loop"][0]["year"].asString();
						sDuration = root["playlist_loop"][0]["duration"].asString();

						if (sTrackArtist != "")
							sArtist = sTrackArtist;
						else
							if (sAlbumArtist != "")
								sArtist = sAlbumArtist;
						if (sYear == "0") sYear = "";
						if (sYear != "")
							sYear = " (" + sYear + ")";
					}

					sLabel = sArtist + " - " + sTitle + sYear;
					sStatus = sLabel;
				}
			}
		}
	}
	catch (...)
	{
		bPingOK = false;
	}
	UpdateNodeStatus(Node, nStatus, sStatus, bPingOK);
	if (m_iThreadsRunning > 0) m_iThreadsRunning--;
}

void CLogitechMediaServer::Do_Work()
{
	int mcounter = 0;
	int scounter = 0;
	bool bFirstTime = true;

	_log.Log(LOG_STATUS, "Logitech Media Server: Worker started...");

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		mcounter++;
		if (mcounter == 2)
		{
			mcounter = 0;
			scounter++;
			if ((scounter >= m_iPollInterval) || (bFirstTime))
			{
				boost::lock_guard<boost::mutex> l(m_mutex);

				scounter = 0;
				bFirstTime = false;

				GetPlayerInfo();

				std::vector<LogitechMediaServerNode>::const_iterator itt;
				for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
				{
					if (m_stoprequested)
						return;
					if (m_iThreadsRunning < 1000)
					{
						m_iThreadsRunning++;
						boost::thread t(boost::bind(&CLogitechMediaServer::Do_Node_Work, this, *itt));
						t.join();
					}
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "Logitech Media Server: Worker stopped...");
}

void CLogitechMediaServer::GetPlayerInfo()
{
	try
	{
		std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"\",[\"serverstatus\",0,999]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);
		bool bSHowedWarning = false;

		int totPlayers = root["player count"].asInt();
		for (int ii = 0; ii < totPlayers; ii++)
		{
			if (root["players_loop"][ii]["name"].empty())
				continue;

			int isplayer = root["players_loop"][ii]["isplayer"].asInt();
			std::string name = root["players_loop"][ii]["name"].asString();
			std::string model = root["players_loop"][ii]["model"].asString();
			std::string ipport = root["players_loop"][ii]["ip"].asString();
			std::string mac = root["players_loop"][ii]["playerid"].asString();
			std::vector<std::string> IPPort;
			StringSplit(ipport, ":", IPPort);
			if (IPPort.size() < 2)
				continue; //invalid ip:port
			std::string ip = IPPort[0];
			int port = atoi(IPPort[1].c_str());

			//	slimp3		=> 'SliMP3'					?
			//	Squeezebox	=> 'Squeezebox 1'			V
			//	squeezebox2	=> 'Squeezebox 2'			V
			//	squeezebox3	=> 'Squeezebox 3'			V
			//	transporter	=> 'Transporter'			?
			//	receiver	=> 'Squeezebox Receiver'	X
			//	boom		=> 'Squeezebox Boom'		?
			//	softsqueeze	=> 'Softsqueeze'			?
			//	controller	=> 'Squeezebox Controller'	X
			//	squeezeplay	=> 'SqueezePlay'			?
			//	baby		=> 'Squeezebox Radio'		V
			//	fab4		=> 'Squeezebox Touch'		V
			//	squeezelite	=> 'Max2Play SqueezePlug'	V
			//	iPengiPod	=> 'iPeng iOS App'			X

			if ((model == "Squeezebox") || (model == "squeezebox2") || (model == "squeezebox3") || (model == "baby") || (model == "fab4") || (model == "squeezelite")) {
				InsertUpdatePlayer(name, ip, port);
			}
			else {
				//show only once
				if (!m_bShowedUnsupported) {
					_log.Log(LOG_ERROR, "Logitech Media Server: model '%s' not supported.", model.c_str());
					bSHowedWarning = true;
				}
			}
		}
		if (!m_bShowedUnsupported)
			m_bShowedUnsupported = bSHowedWarning;
	}
	catch (...)
	{
	}
}

void CLogitechMediaServer::InsertUpdatePlayer(const std::string &Name, const std::string &IPAddress, const int Port)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.size()>0)
		return; //Already exists
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d, '%q', '%q', %d)", m_HwdID, Name.c_str(), IPAddress.c_str(), Port);

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.size()<1)
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d, '%q', 1, %d, %d, %d, 1, 12, 255, '%q', 0, 'Unavailable')",
		m_HwdID, szID, int(pTypeLighting2), int(sTypeAC), int(STYPE_Media), Name.c_str());

	ReloadNodes();
}

void CLogitechMediaServer::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

void CLogitechMediaServer::Restart()
{
	StopHardware();
	StartHardware();
}

bool CLogitechMediaServer::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pSen = (tRBUF*)pdata;

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
	std::vector<LogitechMediaServerNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->DevID == DevID)
		{
			std::string	sPlayerId = itt->IP;
			std::string sPower = "0";

			if (pSen->LIGHTING2.cmnd == light2_sOn)
			{
				sPower = "1";
			}

			std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[\"power\",\"" + sPower + "\"]]}";
			Json::Value root = Query(m_IP, m_Port, sPostdata);

			return true;
		}
	}

	return false;
}

void CLogitechMediaServer::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			LogitechMediaServerNode pnode;
			pnode.DevID = atoi(sd[0].c_str());
			sprintf(pnode.szDevID, "%X%02X%02X%02X", 0, 0, (pnode.DevID & 0xFF00) >> 8, pnode.DevID & 0xFF);
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			int Port = atoi(sd[3].c_str());
			pnode.Port = (Port > 0) ? Port : 8080;
			pnode.nStatus = MSTAT_UNKNOWN;
			pnode.sStatus = "";
			pnode.LastOK = mytime(NULL);

			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == 1)", m_HwdID, pnode.szDevID);
			if (result2.size() == 1)
			{
				pnode.ID = atoi(result2[0][0].c_str());
				pnode.nStatus = (_eMediaStatus)atoi(result2[0][1].c_str());
				pnode.sStatus = result2[0][2];
			}

			m_nodes.push_back(pnode);
		}
	}
}

void CLogitechMediaServer::SendCommand(const int ID, const std::string &command)
{
	std::vector<std::vector<std::string> > result;
	std::string sPlayerId = "";
	std::string sLMSCmnd = "";

	// Get device details
	result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE (ID==%d)", ID);
	if (result.size() == 1)
	{
		// Get connection details
		long	DeviceID = strtol(result[0][0].c_str(), NULL, 16);
		result = m_sql.safe_query("SELECT Name, MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, DeviceID);
		sPlayerId = result[0][1];
	}

	if (result.size() == 1)
	{
		std::string	sLMSCall;
		if (command == "Left") {
			sLMSCmnd = "\"button\", \"arrow_left\"";
		}
		else if (command == "Right") {
			sLMSCmnd = "\"button\", \"arrow_right\"";
		}
		else if (command == "Up") {
			sLMSCmnd = "\"button\", \"arrow_up\"";
		}
		else if (command == "Down") {
			sLMSCmnd = "\"button\", \"arrow_down\"";
		}
		else if (command == "Favorites") {
			sLMSCmnd = "\"button\", \"favorites\"";
		}
		else if (command == "Browse") {
			sLMSCmnd = "\"button\", \"browse\"";
		}
		else if (command == "NowPlaying") {
			sLMSCmnd = "\"button\", \"playdisp_toggle\"";
		}
		else if (command == "Shuffle") {
			sLMSCmnd = "\"button\", \"shuffle_toggle\"";
		}
		else if (command == "Repeat") {
			sLMSCmnd = "\"button\", \"repeat_toggle\"";
		}
		else if (command == "Stop") {
			sLMSCmnd = "\"button\", \"stop\"";
		}
		else if (command == "VolumeUp") {
			sLMSCmnd = "\"mixer\", \"volume\", \"+2\"";
		}
		else if (command == "Mute") {
			sLMSCmnd = "\"mixer\", \"muting\", \"toggle\"";
		}
		else if (command == "VolumeDown") {
			sLMSCmnd = "\"mixer\", \"volume\", \"-2\"";
		}
		else if (command == "Rewind") {
			sLMSCmnd = "\"button\", \"rew.single\"";
		}
		else if (command == "Play") {
			sLMSCmnd = "\"button\", \"play.single\"";
		}
		else if (command == "Pause") {
			sLMSCmnd = "\"button\", \"pause.single\"";
		}
		else if (command == "Forward") {
			sLMSCmnd = "\"button\", \"fwd.single\"";
		}

		if (sLMSCmnd != "")
		{
			std::string sPostdata = "{\"id\":1,\"method\":\"slim.request\",\"params\":[\"" + sPlayerId + "\",[" + sLMSCmnd + "]]}";
			Json::Value root = Query(m_IP, m_Port, sPostdata);
		}
		else
		{
			_log.Log(LOG_ERROR, "Logitech Media Server: (%s) Command: '%s'. Unknown command.", result[0][0].c_str(), command.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Logitech Media Server: (%d) Command: '%s'. Device not found.", ID, command.c_str());
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_LMSSetMode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}
			std::string hwid = m_pWebEm->FindValue("idx");
			std::string mode1 = m_pWebEm->FindValue("mode1");
			std::string mode2 = m_pWebEm->FindValue("mode2");
			if (
				(hwid == "") ||
				(mode1 == "") ||
				(mode2 == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			CLogitechMediaServer *pHardware = (CLogitechMediaServer*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "LMSSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}

		void CWebServer::Cmd_LMSGetNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_LogitechMediaServer)
				return;

			root["status"] = "OK";
			root["title"] = "LMSGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
					root["result"][ii]["Port"] = atoi(sd[3].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_LMSMediaCommand(Json::Value &root)
		{
			std::string sIdx = m_pWebEm->FindValue("idx");
			std::string sAction = m_pWebEm->FindValue("action");
			if (sIdx.empty())
				return;
			int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "LMSMediaCommand";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DS.SwitchType, H.Type, H.ID FROM DeviceStatus DS, Hardware H WHERE (DS.ID=='%q') AND (DS.HardwareID == H.ID)", sIdx.c_str());
			if (result.size() == 1)
			{
				_eSwitchType	sType = (_eSwitchType)atoi(result[0][0].c_str());
				_eHardwareTypes	hType = (_eHardwareTypes)atoi(result[0][1].c_str());
				int HwID = atoi(result[0][2].c_str());
				// Is the device a media Player?
				if (sType == STYPE_Media)
				{
					switch (hType) {
					case HTYPE_LogitechMediaServer:
						CLogitechMediaServer LMS(HwID);
						LMS.SendCommand(idx, sAction);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}