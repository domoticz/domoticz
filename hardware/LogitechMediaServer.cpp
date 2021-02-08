#include "stdafx.h"
#include "LogitechMediaServer.h"
#include "../hardware/hardwaretypes.h"
#include "../main/json_helper.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../notifications/NotificationHelper.h"
#include "../httpclient/HTTPClient.h"

CLogitechMediaServer::CLogitechMediaServer(const int ID, const std::string &IPAddress, const int Port, const std::string &User, const std::string &Pwd, const int PollIntervalsec)
	: m_iThreadsRunning(0)
	, m_IP(IPAddress)
	, m_User(User)
	, m_Pwd(Pwd)
{
	m_HwdID = ID;
	m_Port = Port;
	m_bShowedStartupMessage = false;
	m_iMissedQueries = 0;
	SetSettings(PollIntervalsec);
}

CLogitechMediaServer::CLogitechMediaServer(const int ID) :
	m_iThreadsRunning(0)
{
	m_HwdID = ID;
	m_Port = 0;
	m_iMissedQueries = 0;
	m_bShowedStartupMessage = false;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Address, Port, Username, Password FROM Hardware WHERE ID==%d", m_HwdID);

	if (!result.empty())
	{
		m_IP = result[0][0];
		m_Port = atoi(result[0][1].c_str());
		m_User = result[0][2];
		m_Pwd = result[0][3];
	}

	SetSettings(10);
}

CLogitechMediaServer::~CLogitechMediaServer()
{
	m_bIsStarted = false;
}

Json::Value CLogitechMediaServer::Query(const std::string &sIP, const int iPort, const std::string &sPostdata)
{
	Json::Value root;
	std::vector<std::string> ExtraHeaders;
	std::string sResult;
	std::stringstream sURL;
	std::stringstream sPostData;

	if ((!m_User.empty()) && (!m_Pwd.empty()))
		sURL << "http://" << m_User << ":" << m_Pwd << "@" << sIP << ":" << iPort << "/jsonrpc.js";
	else
		sURL << "http://" << sIP << ":" << iPort << "/jsonrpc.js";

	sPostData << sPostdata;

	HTTPClient::SetTimeout(5);
	bool bRetVal = HTTPClient::POST(sURL.str(), sPostData.str(), ExtraHeaders, sResult);

	if (!bRetVal)
	{
		return root;
	}
	bRetVal = ParseJSon(sResult, root);
	if ((!bRetVal) || (!root.isObject()))
	{
		size_t aFind = sResult.find("401 Authorization Required");
		if ((aFind > 0) && (aFind != std::string::npos))
			_log.Log(LOG_ERROR, "Logitech Media Server: Username and/or password are incorrect. Check Logitech Media Server settings.");
		else
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

_eNotificationTypes	CLogitechMediaServer::NotificationType(_eMediaStatus nStatus)
{
	switch (nStatus)
	{
	case MSTAT_OFF:		return NTYPE_SWITCH_OFF;
	case MSTAT_ON:		return NTYPE_SWITCH_ON;
	case MSTAT_PAUSED:	return NTYPE_PAUSED;
	case MSTAT_STOPPED:	return NTYPE_STOPPED;
	case MSTAT_PLAYING:	return NTYPE_PLAYING;
	default:			return NTYPE_SWITCH_OFF;
	}
}

bool CLogitechMediaServer::StartHardware()
{
	StopHardware();

	RequestStart();

	m_bIsStarted = true;
	sOnConnected(this);
	m_iThreadsRunning = 0;
	m_bShowedStartupMessage = false;

	StartHeartbeatThread();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CLogitechMediaServer::StopHardware()
{
	StopHeartbeatThread();

	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CLogitechMediaServer::UpdateNodeStatus(const LogitechMediaServerNode &Node, const _eMediaStatus nStatus, const std::string &sStatus, bool bPingOK)
{
	//This has to be rebuild! No direct poking in the database, please use CMainWorker::UpdateDevice

	//Find out node, and update it's status
	for (auto &node : m_nodes)
	{
		if (node.ID == Node.ID)
		{
			//Found it
			//Retrieve devicename instead of playername in case it was renamed...
			std::string sDevName = node.Name;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE DeviceID=='%q'", node.szDevID);
			if (result.size() == 1) {
				std::vector<std::string> sd = result[0];
				sDevName = sd[0];
			}
			bool	bUseOnOff = false;
			if (((nStatus == MSTAT_OFF) && bPingOK) || ((nStatus != MSTAT_OFF) && !bPingOK)) bUseOnOff = true;
			time_t atime = mytime(nullptr);
			node.LastOK = atime;
			if ((node.nStatus != nStatus) || (node.sStatus != sStatus))
			{
				// 1:	Update the DeviceStatus
				if ((nStatus == MSTAT_PLAYING) || (nStatus == MSTAT_PAUSED) || (nStatus == MSTAT_STOPPED))
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s - '%s'", Node.Name.c_str(), Media_Player_States(nStatus), sStatus.c_str());
				else
					_log.Log(LOG_NORM, "Logitech Media Server: (%s) %s", Node.Name.c_str(), Media_Player_States(nStatus));
				struct tm ltime;
				localtime_r(&atime, &ltime);
				char szLastUpdate[40];
				sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', "
							  "LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == "
							  "'%q') AND (Unit == 1) AND (SwitchType == %d)",
							  int(nStatus), sStatus.c_str(), szLastUpdate, m_HwdID, node.szDevID, STYPE_Media);

				// 2:	Log the event if the actual status has changed
				const std::string &sShortStatus = sStatus;
				if ((node.nStatus != nStatus) || (node.sShortStatus != sShortStatus))
				{
					std::string sLongStatus = Media_Player_States(nStatus);
					if ((nStatus == MSTAT_PLAYING) || (nStatus == MSTAT_PAUSED) || (nStatus == MSTAT_STOPPED))
						if (sShortStatus.length()) sLongStatus += " - " + sShortStatus;
					result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue, "
								  "User) VALUES (%d, %d, '%q','%q')",
								  node.ID, int(nStatus), sLongStatus.c_str(), "Logitech");
				}

				// 3:	Trigger On/Off actions
				if (bUseOnOff)
				{
					result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE "
								  "(HardwareID==%d) AND (ID = '%q') AND (Unit == 1)",
								  m_HwdID, node.szDevID);
					if (!result.empty())
					{
						m_sql.HandleOnOffAction(bPingOK, result[0][0], result[0][1]);
					}
				}

				// 4:   Trigger Notifications & events on status change
				if (node.nStatus != nStatus)
				{
					m_notifications.CheckAndHandleNotification(node.ID, sDevName, NotificationType(nStatus), sStatus);
					m_mainworker.m_eventsystem.ProcessDevice(m_HwdID, node.ID, 1, int(pTypeLighting2), int(sTypeAC), 12,
										 100, int(nStatus), sStatus.c_str());
				}

				node.nStatus = nStatus;
				node.sStatus = sStatus;
				node.sShortStatus = sShortStatus;
			}
			break;
		}
	}
}

void CLogitechMediaServer::Do_Node_Work(const LogitechMediaServerNode &Node)
{
	bool bPingOK = false;
	_eMediaStatus nStatus = MSTAT_UNKNOWN;
	_eMediaStatus nOldStatus = Node.nStatus;
	std::string	sPlayerId = Node.IP;
	std::string sStatus;

	try
	{
		std::string sPostdata = R"({"id":1,"method":"slim.request","params":[")" + sPlayerId + R"(",["status","-",1,"tags:Aadly"]]})";
		Json::Value root = Query(m_IP, m_Port, sPostdata);

		if (root.isNull())
			nStatus = MSTAT_DISCONNECTED;
		else
		{
			bPingOK = true;

			if (root["player_connected"].asString() == "1")
			{
				if (root["power"].asString() == "0")
					nStatus = MSTAT_OFF;
				else {
					std::string sMode = root["mode"].asString();
					if (sMode == "play")
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_PLAYING;
					else if (sMode == "pause")
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_PAUSED;
					else if (sMode == "stop")
						if ((nOldStatus == MSTAT_OFF) || (nOldStatus == MSTAT_DISCONNECTED))
							nStatus = MSTAT_ON;
						else
							nStatus = MSTAT_STOPPED;
					else
						nStatus = MSTAT_ON;
					std::string sTitle;
					std::string sAlbum;
					std::string sArtist;
					std::string sAlbumArtist;
					std::string sTrackArtist;
					std::string sYear;
					std::string sDuration;
					std::string sLabel;

					if (!root["playlist_loop"].empty())
					{
						sTitle = root["playlist_loop"][0]["title"].asString();
						sAlbum = root["playlist_loop"][0]["album"].asString();
						sArtist = root["playlist_loop"][0]["artist"].asString();
						sAlbumArtist = root["playlist_loop"][0]["albumartist"].asString();
						sTrackArtist = root["playlist_loop"][0]["trackartist"].asString();
						sYear = root["playlist_loop"][0]["year"].asString();
						sDuration = root["playlist_loop"][0]["duration"].asString();

						if (!sTrackArtist.empty())
							sArtist = sTrackArtist;
						else if (!sAlbumArtist.empty())
							sArtist = sAlbumArtist;
						if (sYear == "0") sYear = "";
						if (!sYear.empty())
							sYear = " (" + sYear + ")";

						sLabel = sArtist + " - " + sTitle + sYear;
					}
					else
						sLabel = "(empty playlist)";

					sStatus = sLabel;
				}
			}
			else
				nStatus = MSTAT_DISCONNECTED;
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

	ReloadNodes();
	ReloadPlaylists();

	//Mark devices as 'Unused'
	m_sql.safe_query("UPDATE WOLNodes SET Timeout=-1 WHERE (HardwareID==%d)", m_HwdID);

	while (!IsStopRequested(500))
	{
		mcounter++;
		if (mcounter == 2)
		{
			mcounter = 0;
			scounter++;
			if ((scounter >= m_iPollInterval) || (bFirstTime))
			{
				std::lock_guard<std::mutex> l(m_mutex);

				scounter = 0;
				bFirstTime = false;

				GetPlayerInfo();

				for (const auto &node : m_nodes)
				{
					if (IsStopRequested(0))
						return;
					if (m_iThreadsRunning < 1000)
					{
						m_iThreadsRunning++;
						boost::thread t([this, node] { Do_Node_Work(node); });
						SetThreadName(t.native_handle(), "LogitechNode");
						t.join();
					}
				}
			}
		}
	}
	//Make sure all our background workers are stopped
	while (m_iThreadsRunning > 0)
	{
		sleep_milliseconds(150);
	}

	_log.Log(LOG_STATUS, "Logitech Media Server: Worker stopped...");
}

void CLogitechMediaServer::GetPlayerInfo()
{
	try
	{
		std::string sPostdata = R"({"id":1,"method":"slim.request","params":["",["serverstatus",0,999]]})";
		Json::Value root = Query(m_IP, m_Port, sPostdata);

		if (root.isNull()) {
			m_iMissedQueries++;
			if (m_iMissedQueries % 3 == 0) {
				_log.Log(LOG_ERROR, "Logitech Media Server: No response from server %s:%i", m_IP.c_str(), m_Port);
			}
		}
		else {
			SetHeartbeatReceived();
			m_iMissedQueries = 0;

			int totPlayers = root["player count"].asInt();
			if (totPlayers > 0) {
				if (!m_bShowedStartupMessage)
					_log.Log(LOG_STATUS, "Logitech Media Server: %i connected player(s) found.", totPlayers);
				for (int ii = 0; ii < totPlayers; ii++)
				{
					if (root["players_loop"][ii]["name"].empty())
						continue;

					//int isplayer = root["players_loop"][ii]["isplayer"].asInt();
					std::string name = root["players_loop"][ii]["name"].asString();
					std::string model = root["players_loop"][ii]["model"].asString();
					std::string ipport = root["players_loop"][ii]["ip"].asString();
					std::string macaddress = root["players_loop"][ii]["playerid"].asString();
					std::vector<std::string> IPPort;
					StringSplit(ipport, ":", IPPort);
					if (IPPort.size() < 2)
						continue; //invalid ip:port
					std::string ip = IPPort[0];
					//int port = atoi(IPPort[1].c_str());

					if (
						//(model == "slimp3") ||			//SliMP3
						(model == "Squeezebox") ||			//Squeezebox 1
						(model == "squeezebox2") ||			//Squeezebox 2
						(model == "squeezebox3") ||			//Squeezebox 3
						(model == "transporter") ||			//Transporter
						(model == "receiver") ||			//Squeezebox Receiver
						(model == "boom") ||				//Squeezebox Boom
						//(model == "softsqueeze") ||		//Softsqueeze
						(model == "controller") ||			//Squeezebox Controller
						(model == "squeezeplay") ||			//SqueezePlay
						(model == "squeezeplayer") ||		//SqueezePlay
						(model == "baby") ||				//Squeezebox Radio
						(model == "fab4") ||				//Squeezebox Touch
						(model == "iPengiPod") ||			//iPeng iPhone App
						(model == "iPengiPad") ||			//iPeng iPad App
						(model == "squeezelite") ||			//Max2Play SqueezePlug
						(model == "daphile")				//Audiophile Music Server & Player OS
						)
					{
						UpsertPlayer(name, ip, macaddress);
					}
					else {
						if (!m_bShowedStartupMessage)
							_log.Log(LOG_ERROR, "Logitech Media Server: model '%s' not supported.", model.c_str());
					}
				}
			}
			else {
				if (!m_bShowedStartupMessage)
					_log.Log(LOG_ERROR, "Logitech Media Server: No connected players found.");
			}
			m_bShowedStartupMessage = true;
		}
	}
	catch (...)
	{
	}
}

void CLogitechMediaServer::UpsertPlayer(const std::string &Name, const std::string &IPAddress, const std::string &MacAddress)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists...
	//1) Check on MacAddress (default)
	result = m_sql.safe_query("SELECT Name FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, MacAddress.c_str());
	if (!result.empty()) {
		if (result[0][0] != Name)
		{ // Update Name in case it has been changed
			m_sql.safe_query("UPDATE WOLNodes SET Name='%q', Timeout=0 WHERE (HardwareID==%d) AND (MacAddress=='%q')", Name.c_str(), m_HwdID, MacAddress.c_str());
			_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' renamed to '%s'", result[0][0].c_str(), Name.c_str());
		}
		else { //Mark device as 'Active'
			m_sql.safe_query("UPDATE WOLNodes SET Timeout=0 WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, MacAddress.c_str());
		}
		return;
	}

	//2) Check on Name+IP (old format); Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (!result.empty()) {
		m_sql.safe_query("UPDATE WOLNodes SET MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", MacAddress.c_str(), m_HwdID, Name.c_str(), IPAddress.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//3) Check on Name (old format), IP changed?; Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q')", m_HwdID, Name.c_str());
	if (!result.empty()) {
		m_sql.safe_query("UPDATE WOLNodes SET MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (Name=='%q')", MacAddress.c_str(), m_HwdID, Name.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//4) Check on IP (old format), Name changed?; Convert IP -> MacAddress
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, IPAddress.c_str());
	if (!result.empty()) {
		m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q', Timeout=0 WHERE (HardwareID==%d) AND (MacAddress=='%q')", Name.c_str(), MacAddress.c_str(), m_HwdID, IPAddress.c_str());
		_log.Log(LOG_STATUS, "Logitech Media Server: Player '%s' IP changed to MacAddress", Name.c_str());
		return;
	}

	//Player not found, add it...
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d, '%q', '%q', 0)", m_HwdID, Name.c_str(), MacAddress.c_str());
	_log.Log(LOG_STATUS, "Logitech Media Server: New Player '%s' added", Name.c_str());

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress='%q')", m_HwdID, MacAddress.c_str());
	if (result.empty())
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	m_sql.InsertDevice(m_HwdID, szID, 1, pTypeLighting2, sTypeAC, STYPE_Media, 0, "Unavailable", Name, 12, 255, 1);

	ReloadNodes();
}

void CLogitechMediaServer::SetSettings(const int PollIntervalsec)
{
	//Defaults
	m_iPollInterval = 30;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
}

bool CLogitechMediaServer::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
	for (const auto &node : m_nodes)
	{
		if (node.DevID == DevID)
		{
			int iParam = pSen->LIGHTING2.level;
			std::string sParam;
			switch (pSen->LIGHTING2.cmnd)
			{
			case light2_sOn:
			case light2_sGroupOn:
				return SendCommand(node.ID, "PowerOn");
			case light2_sOff:
			case light2_sGroupOff:
				return SendCommand(node.ID, "PowerOff");
			case gswitch_sPlay:
				SendCommand(node.ID, "NowPlaying");
				return SendCommand(node.ID, "Play");
			case gswitch_sPlayPlaylist:
				sParam = GetPlaylistByRefID(iParam);
				return SendCommand(node.ID, "PlayPlaylist", sParam);
			case gswitch_sPlayFavorites:
				return SendCommand(node.ID, "PlayFavorites");
			case gswitch_sStop:
				return SendCommand(node.ID, "Stop");
			case gswitch_sPause:
				return SendCommand(node.ID, "Pause");
			case gswitch_sSetVolume:
				sParam = std::to_string(iParam);
				return SendCommand(node.ID, "SetVolume", sParam);
			default:
				return true;
			}
		}
	}

	return false;
}

void CLogitechMediaServer::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		_log.Log(LOG_STATUS, "Logitech Media Server: %d player-switch(es) found.", (int)result.size());
		for (const auto &sd : result)
		{
			LogitechMediaServerNode pnode;
			pnode.ID = 0;
			pnode.DevID = atoi(sd[0].c_str());
			sprintf(pnode.szDevID, "%X%02X%02X%02X", 0, 0, (pnode.DevID & 0xFF00) >> 8, pnode.DevID & 0xFF);
			pnode.Name = sd[1];
			pnode.IP = sd[2];
			pnode.nStatus = MSTAT_UNKNOWN;
			pnode.sStatus = "";
			pnode.LastOK = mytime(nullptr);

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
	else
		_log.Log(LOG_ERROR, "Logitech Media Server: No player-switches found.");
}

void CLogitechMediaServer::ReloadPlaylists()
{
	m_playlists.clear();

	std::string sPostdata = R"({"id":1,"method":"slim.request","params":["",["playlists",0,999]]})";
	Json::Value root = Query(m_IP, m_Port, sPostdata);

	if (root.isNull()) {
		_log.Log(LOG_ERROR, "Logitech Media Server: No response from server %s:%i", m_IP.c_str(), m_Port);
	}
	else {
		int totPlaylists = root["count"].asInt();
		if (totPlaylists > 0) {
			_log.Log(LOG_STATUS, "Logitech Media Server: %i playlist(s) found.", totPlaylists);
			for (int ii = 0; ii < totPlaylists; ii++)
			{
				LMSPlaylistNode pnode;

				pnode.ID = root["playlists_loop"][ii]["id"].asInt();
				pnode.Name = root["playlists_loop"][ii]["playlist"].asString();
				pnode.refID = pnode.ID % 256;

				m_playlists.push_back(pnode);
			}
		}
		else
			_log.Log(LOG_STATUS, "Logitech Media Server: No playlists found.");
	}
}

std::string CLogitechMediaServer::GetPlaylistByRefID(const int ID)
{
	for (const auto &playlist : m_playlists)
		if (playlist.refID == ID)
			return playlist.Name;

	_log.Log(LOG_ERROR, "Logitech Media Server: Playlist ID %d not found.", ID);
	return "";
}

bool CLogitechMediaServer::SendCommand(const int ID, const std::string &command, const std::string &param)
{
	std::vector<std::vector<std::string> > result;
	std::string sPlayerId;
	std::string sLMSCmnd;

	// Get device details
	result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE (ID==%d)", ID);
	if (result.size() == 1)
	{
		// Get connection details
		long DeviceID = strtol(result[0][0].c_str(), nullptr, 16);
		result = m_sql.safe_query("SELECT Name, MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, DeviceID);
		sPlayerId = result[0][1];
	}

	if (result.size() == 1)
	{
		//std::string	sLMSCall;
		if (command == "Left") {
			sLMSCmnd = R"("button", "arrow_left")";
		}
		else if (command == "Right") {
			sLMSCmnd = R"("button", "arrow_right")";
		}
		else if (command == "Up") {
			sLMSCmnd = R"("button", "arrow_up")";
		}
		else if (command == "Down") {
			sLMSCmnd = R"("button", "arrow_down")";
		}
		else if (command == "Favorites") {
			sLMSCmnd = R"("button", "favorites")";
		}
		else if (command == "Browse") {
			sLMSCmnd = R"("button", "browse")";
		}
		else if (command == "NowPlaying") {
			sLMSCmnd = R"("button", "playdisp_toggle")";
		}
		else if (command == "Shuffle") {
			sLMSCmnd = R"("button", "shuffle_toggle")";
		}
		else if (command == "Repeat") {
			sLMSCmnd = R"("button", "repeat_toggle")";
		}
		else if (command == "Stop") {
			sLMSCmnd = R"("button", "stop")";
		}
		else if (command == "VolumeUp") {
			sLMSCmnd = R"("mixer", "volume", "+2")";
		}
		else if (command == "Mute") {
			sLMSCmnd = R"("mixer", "muting", "toggle")";
		}
		else if (command == "VolumeDown") {
			sLMSCmnd = R"("mixer", "volume", "-2")";
		}
		else if (command == "Rewind") {
			sLMSCmnd = R"("button", "rew.single")";
		}
		else if (command == "Play") {
			sLMSCmnd = R"("button", "play.single")";
		}
		else if (command == "PlayPlaylist") {
			if (param.empty())
				return false;
			sLMSCmnd = R"("playlist", "play", ")" + param + "\"";
		}
		else if (command == "PlayFavorites") {
			sLMSCmnd = R"("favorites", "playlist", "play")";
		}
		else if (command == "Pause") {
			sLMSCmnd = R"("button", "pause.single")";
		}
		else if (command == "Forward") {
			sLMSCmnd = R"("button", "fwd.single")";
		}
		else if (command == "PowerOn") {
			sLMSCmnd = R"("power", "1")";
		}
		else if (command == "PowerOff") {
			sLMSCmnd = R"("power", "0")";
		}
		else if (command == "SetVolume") {
			if (param.empty())
				return false;
			sLMSCmnd = R"("mixer", "volume", ")" + param + "\"";
		}

		if (!sLMSCmnd.empty())
		{
			std::string sPostdata = R"({"id":1,"method":"slim.request","params":[")" + sPlayerId + "\",[" + sLMSCmnd + "]]}";
			Json::Value root = Query(m_IP, m_Port, sPostdata);

			sPostdata = R"({"id":1,"method":"slim.request","params":[")" + sPlayerId + R"(",["status","-",1,"tags:uB"]]})";
			root = Query(m_IP, m_Port, sPostdata);

			if (root["player_connected"].asString() == "1")
			{
				std::string sPower = root["power"].asString();
				std::string sMode = root["mode"].asString();

				if (command == "Stop")
					return sMode == "stop";
				if (command == "Pause")
					return sMode == "pause";
				if (command == "Play")
					return sMode == "play";
				if (command == "PlayPlaylist")
					return sMode == "play";
				if (command == "PlayFavorites")
					return sMode == "play";
				if (command == "PowerOn")
					return sPower == "1";
				if (command == "PowerOff")
					return sPower == "0";
			}
			return false;
		}
		_log.Log(LOG_ERROR, "Logitech Media Server: (%s) Command: '%s'. Unknown command.", result[0][0].c_str(), command.c_str());
		return false;
	}
	_log.Log(LOG_ERROR, "Logitech Media Server: (%d) Command: '%s'. Device not found.", ID, command.c_str());
	return false;
}

void CLogitechMediaServer::SendText(const std::string &playerIP, const std::string &subject, const std::string &text, const int duration)
{
	if ((!playerIP.empty()) && (!text.empty()) && (duration > 0))
	{
		const std::string &sLine1 = subject;
		const std::string &sLine2 = text;
		std::string sFont; //"huge";
		std::string sBrightness = "4";
		std::string sDuration = std::to_string(duration);
		std::string sPostdata = R"({"id":1,"method":"slim.request","params":[")" + playerIP + R"(",["show","line1:)" + sLine1 + "\",\"line2:" + sLine2 + "\",\"duration:" + sDuration +
					"\",\"brightness:" + sBrightness + "\",\"font:" + sFont + "\"]]}";
		Json::Value root = Query(m_IP, m_Port, sPostdata);
	}
}

std::vector<CLogitechMediaServer::LMSPlaylistNode> CLogitechMediaServer::GetPlaylists()
{
	return m_playlists;
}

int CLogitechMediaServer::GetPlaylistRefID(const std::string &name)
{
	for (const auto &playlist : m_playlists)
		if (playlist.Name == name)
			return playlist.refID;

	_log.Log(LOG_ERROR, "Logitech Media Server: Playlist '%s' not found.", name.c_str());
	return 0;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_LMSSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			if ((hwid.empty()) || (mode1.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "LMSSetMode";

			int iMode1 = atoi(mode1.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d WHERE (ID == '%q')", iMode1, hwid.c_str());
			pHardware->SetSettings(iMode1);
			pHardware->Restart();
		}

		void CWebServer::Cmd_LMSDeleteUnusedDevices(WebEmSession & session, const request& req, Json::Value &/*root*/)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			m_sql.safe_query("DELETE FROM WOLNodes WHERE ((HardwareID==%d) AND (Timeout==-1))", iHardwareID);
		}

		void CWebServer::Cmd_LMSGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_LogitechMediaServer)
				return;

			root["status"] = "OK";
			root["title"] = "LMSGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress,(CASE Timeout WHEN -1 THEN 'Unused' ELSE 'Active' END) as Status FROM WOLNodes WHERE (HardwareID==%d)", iHardwareID);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Mac"] = sd[2];
					root["result"][ii]["Status"] = sd[3];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_LMSGetPlaylists(WebEmSession & /*session*/, const request& req, Json::Value &root)
		{
			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_LogitechMediaServer)
				return;
			CLogitechMediaServer *pHardware = reinterpret_cast<CLogitechMediaServer*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "LMSGetPlaylists";

			std::vector<CLogitechMediaServer::LMSPlaylistNode> _nodes = pHardware->GetPlaylists();

			int ii = 0;
			for (const auto &node : _nodes)
			{
				root["result"][ii]["id"] = node.ID;
				root["result"][ii]["refid"] = node.refID;
				root["result"][ii]["Name"] = node.Name;
				ii++;
			}
		}

		void CWebServer::Cmd_LMSMediaCommand(WebEmSession & /*session*/, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
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

	} // namespace server
} // namespace http
