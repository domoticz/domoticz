#include "stdafx.h"
#include "HEOS.h"
#include "../hardware/hardwaretypes.h"
#include "../json/json.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../main/EventSystem.h"
#include "../webserver/cWebem.h"

#include <iostream>

#define DEBUG_LOGGING false
#define RETRY_DELAY 30

CHEOS::CHEOS(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &User, const std::string &Pwd, const int PollIntervalsec, const int PingTimeoutms) :
	m_IP(IPAddress),
	m_User(User),
	m_Pwd(Pwd)
{
	m_HwdID = ID;
	m_bDoRestart = false;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_retrycntr = RETRY_DELAY;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CHEOS::~CHEOS(void)
{
}

void CHEOS::ParseLine()
{
	if (m_bufferpos < 2)
		return;
	std::string sLine((char*)&m_buffer);

	try
	{
		Json::Reader jReader;
		Json::Value root;

		if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: Handling message: '%s'.", sLine.c_str());

		bool bRetVal = jReader.parse(sLine, root);
		if ((!bRetVal) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "DENON by HEOS: PARSE ERROR: '%s'", sLine.c_str());
		}
		else
		{
			// HEOS generated messages
			if (root.isMember("heos"))
			{
				if (root["heos"].isMember("result"))
				{
					if (root["heos"]["result"] == "success")
					{
						if (root["heos"].isMember("command"))
						{
							if (root["heos"]["command"] == "system/heart_beat")
							{

							}
							else if (root["heos"]["command"] == "player/get_players")
							{
								if (root.isMember("payload"))
								{
									int key = 0;
									for (Json::ValueIterator itr = root["payload"].begin(); itr != root["payload"].end(); itr++) {

										if (root["payload"][key].isMember("name") && root["payload"][key].isMember("pid"))
										{
											std::string pid = std::to_string(root["payload"][key]["pid"].asInt());
											AddNode(root["payload"][key]["name"].asCString(), pid);
										}
										else
										{
											if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: No players found.");
										}
										key++;
									}
								}
								else
								{
									if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: No players found (No Payload).");
								}
							}
							else if (root["heos"]["command"] == "player/get_play_state" || root["heos"]["command"] == "player/set_play_state")
							{
								if (root["heos"].isMember("message"))
								{
									std::vector<std::string> SplitMessage;
									StringSplit(root["heos"]["message"].asString(), "&", SplitMessage);
									if (SplitMessage.size() > 0)
									{
										std::vector<std::string> SplitMessagePlayer;
										StringSplit(SplitMessage[0], "=", SplitMessagePlayer);
										std::vector<std::string> SplitMessageState;
										StringSplit(SplitMessage[1], "=", SplitMessageState);
										std::string pid = SplitMessagePlayer[1];
										std::string state = SplitMessageState[1];

										_eMediaStatus nStatus = MSTAT_UNKNOWN;

										if (state == "play")
											nStatus = MSTAT_PLAYING;
										else if (state == "pause")
											nStatus = MSTAT_PAUSED;
										else if (state == "stop")
											nStatus = MSTAT_STOPPED;
										else
											nStatus = MSTAT_ON;

										std::string	sStatus = "";

										UpdateNodeStatus(pid, nStatus, sStatus);

										/* If playing request now playing information */
										if (state == "play") {
											int PlayerID = atoi(pid.c_str());
											SendCommand("getNowPlaying", PlayerID);
										}

										m_lastUpdate = 0;
									}
								}
							}
							else if (root["heos"]["command"] == "player/get_now_playing_media")
							{
								if (root["heos"].isMember("message"))
								{
									std::vector<std::string> SplitMessage;
									StringSplit(root["heos"]["message"].asString(), "=", SplitMessage);
									if (SplitMessage.size() > 0)
									{
										std::string sLabel = "";
										std::string	sStatus = "";
										std::string pid = SplitMessage[1];

										if (root.isMember("payload"))
										{

											std::string	sTitle = "";
											std::string	sAlbum = "";
											std::string	sArtist = "";
											std::string	sStation = "";

											sTitle = root["payload"]["song"].asString();
											sAlbum = root["payload"]["album"].asString();
											sArtist = root["payload"]["artist"].asString();
											sStation = root["payload"]["station"].asString();

											if (sStation != "")
											{
												sLabel = sArtist + " - " + sTitle + " - " + sStation;
											}
											else
											{
												sLabel = sArtist + " - " + sTitle;
											}
										}
										else
										{
											sLabel = "(empty playlist)";
										}


										sStatus = sLabel;

										UpdateNodesStatus(pid, sStatus);

										m_lastUpdate = 0;
									}
								}
							}
						}
					}
					else
					{
						if (root["heos"].isMember("command"))
						{
							if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: Failed: '%s'.", root["heos"]["command"].asCString());
						}
					}
				}
				else
				{
					if (root["heos"].isMember("command"))
					{
						if (root["heos"]["command"] == "event/player_state_changed")
						{
							if (root["heos"].isMember("message"))
							{
								std::vector<std::string> SplitMessage;
								StringSplit(root["heos"]["message"].asString(), "&", SplitMessage);
								if (SplitMessage.size() > 0)
								{
									std::vector<std::string> SplitMessagePlayer;
									StringSplit(SplitMessage[0], "=", SplitMessagePlayer);
									std::vector<std::string> SplitMessageState;
									StringSplit(SplitMessage[1], "=", SplitMessageState);
									std::string pid = SplitMessagePlayer[1];
									std::string state = SplitMessageState[1];

									_eMediaStatus nStatus = MSTAT_UNKNOWN;

									if (state == "play")
										nStatus = MSTAT_PLAYING;
									else if (state == "pause")
										nStatus = MSTAT_PAUSED;
									else if (state == "stop")
										nStatus = MSTAT_STOPPED;
									else
										nStatus = MSTAT_ON;

									std::string	sStatus = "";

									UpdateNodeStatus(pid, nStatus, sStatus);

									/* If playing request now playing information */
									if (state == "play") {
										int PlayerID = atoi(pid.c_str());
										SendCommand("getNowPlaying", PlayerID);
									}

									m_lastUpdate = 0;
								}
							}
						}
						else if (root["heos"]["command"] == "event/players_changed")
						{
							SendCommand("getPlayers");
						}
						else if (root["heos"]["command"] == "event/groups_changed")
						{
							SendCommand("getPlayers");
						}
						else if (root["heos"]["command"] == "event/player_now_playing_changed")
						{
							std::vector<std::string> SplitMessage;
							StringSplit(root["heos"]["message"].asString(), "=", SplitMessage);
							if (SplitMessage.size() > 0)
							{
								std::string pid = SplitMessage[1];
								int PlayerID = atoi(pid.c_str());
								SendCommand("getPlayState", PlayerID);
							}
						}
						else if (root["heos"]["command"] == "event/player_mute_changed")
						{

						}
						else if (root["heos"]["command"] == "event/repeat_mode_changed")
						{

						}
						else if (root["heos"]["command"] == "event/shuffle_mode_changed")
						{

						}
					}
				}
			}
			else
			{
				if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: Message not generated by HEOS System.");
			}

		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "DENON by HEOS: Exception: %s", e.what());
	}
}

void CHEOS::SendCommand(const std::string &command)
{
	std::stringstream ssMessage;
	std::string	sMessage;
	bool systemCall = false;

	// Register for change events
	if (command == "registerForEvents")
	{
		ssMessage << "heos://system/register_for_change_events?enable=on";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	// Unregister for change events
	if (command == "unRegisterForEvents")
	{
		ssMessage << "heos://system/register_for_change_events?enable=off";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "heartbeat")
	{
		ssMessage << "heos://system/heart_beat";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "getPlayers")
	{
		ssMessage << "heos://player/get_players";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	/* Group related commands */

	if (command == "getGroups")
	{
		ssMessage << "heos://group/get_groups";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	/* Process */

	if (sMessage.length())
	{
		if (WriteInt(sMessage))
		{
			if (systemCall)
			{
				if (DEBUG_LOGGING) _log.Log(LOG_NORM, "HEOS by DENON: Sent command: '%s'.", sMessage.c_str());
			}
			else
			{
				_log.Log(LOG_NORM, "HEOS by DENON: Sent command: '%s'.", sMessage.c_str());
			}
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "HEOS by DENON: Command: '%s'. Unknown command.", command.c_str());
	}
}

void CHEOS::SendCommand(const std::string &command, const int iValue)
{
	std::stringstream ssMessage;
	std::string	sMessage;
	bool systemCall = false;

	if (command == "getPlayerInfo")
	{
		ssMessage << "heos://player/get_player_info?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "getPlayState")
	{
		ssMessage << "heos://player/get_play_state?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "setPlayStatePlay" || command == "play")
	{
		ssMessage << "heos://player/set_play_state?pid=" << iValue << "&state=play";
		sMessage = ssMessage.str();
	}

	if (command == "setPlayStatePause" || command == "pause")
	{
		ssMessage << "heos://player/set_play_state?pid=" << iValue << "&state=pause";
		sMessage = ssMessage.str();
	}

	if (command == "setPlayStateStop" || command == "stop")
	{
		ssMessage << "heos://player/set_play_state?pid=" << iValue << "&state=stop";
		sMessage = ssMessage.str();
	}

	if (command == "getNowPlaying")
	{
		ssMessage << "heos://player/get_now_playing_media?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "getVolume")
	{
		ssMessage << "heos://player/get_volume?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "setVolumeUp")
	{
		ssMessage << "heos://player/volume_up?pid=" << iValue << "";
		sMessage = ssMessage.str();
	}

	if (command == "setVolumeDown")
	{
		ssMessage << "heos://player/volume_down?pid=" << iValue << "";
		sMessage = ssMessage.str();
	}

	if (command == "getMute")
	{
		ssMessage << "heos://player/get_mute?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "setMuteOn")
	{
		ssMessage << "heos://player/set_mute?pid=" << iValue << "&state=on";
		sMessage = ssMessage.str();
	}

	if (command == "setMuteOff")
	{
		ssMessage << "heos://player/set_mute?pid=" << iValue << "&state=off";
		sMessage = ssMessage.str();
	}

	if (command == "toggleMute")
	{
		ssMessage << "heos://player/toggle_mute?pid=" << iValue << "";
		sMessage = ssMessage.str();
	}

	if (command == "getPlayMode")
	{
		ssMessage << "heos://player/get_play_mode?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	/* Set playmode
	if (command == "setPlayMode")
	{
		ssMessage << "heos://player/volume_up?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}
	*/

	/* Queue related commands */

	if (command == "getQueue")
	{
		ssMessage << "heos://player/get_queue?pid=" << iValue << "";
		sMessage = ssMessage.str();
		systemCall = true;
	}

	if (command == "playNext")
	{
		ssMessage << "heos://player/play_next?pid=" << iValue << "";
		sMessage = ssMessage.str();
	}

	if (command == "playPrev")
	{
		ssMessage << "heos://player/play_previous?pid=" << iValue << "";
		sMessage = ssMessage.str();
	}

	/* Process */
	if (DEBUG_LOGGING) _log.Log(LOG_NORM, "DENON by HEOS: Debug: '%s'.", sMessage.c_str());

	if (sMessage.length())
	{
		if (WriteInt(sMessage))
		{
			if (systemCall)
			{
				if (DEBUG_LOGGING) _log.Log(LOG_NORM, "HEOS by DENON: Sent command: '%s'.", sMessage.c_str());
			}
			else
			{
				_log.Log(LOG_NORM, "HEOS by DENON: Sent command: '%s'.", sMessage.c_str());
			}
		}
		else
		{
			if (DEBUG_LOGGING) _log.Log(LOG_NORM, "HEOS by DENON: Not Connected - Message not sent: '%s'.", sMessage.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "HEOS by DENON: Command: '%s'. Unknown command.", command.c_str());
	}
}

void CHEOS::Do_Work()
{

	_log.Log(LOG_STATUS, "HEOS by DENON: Worker started...");

	ReloadNodes();

	bool bFirstTime = true;
	bool bCheckedForPlayers = false;
	int sec_counter = 25;
	m_lastUpdate = 25;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		m_lastUpdate++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (bFirstTime)
		{
			bFirstTime = false;
			connect(m_IP, m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_IP, m_usIPPort);
			}
			update();
			if (mIsConnected)
			{
				if (!bCheckedForPlayers)
				{
					// Update all players and groups
					SendCommand("getPlayers");
					bCheckedForPlayers = true;
					// Enable event changes
					SendCommand("registerForEvents");
				}
				if (sec_counter % 30 == 0 && m_lastUpdate >= 30)//updates every 30 seconds
				{
					bFirstTime = false;
					std::vector<HEOSNode>::const_iterator itt;
					for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
					{
						SendCommand("getPlayState", itt->DevID);
					}
				}
			}
		}
	}

	_log.Log(LOG_STATUS, "HEOS by DENON: Worker stopped...");

}

_eNotificationTypes	CHEOS::NotificationType(_eMediaStatus nStatus)
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

bool CHEOS::StartHardware()
{
	m_stoprequested = false;
	m_bDoRestart = false;

	//force connect the next first time
	m_retrycntr = RETRY_DELAY;
	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CHEOS::Do_Work, this);
	return (m_thread != nullptr);
}

bool CHEOS::StopHardware()
{
	m_stoprequested = true;
	if (isConnected())
	{
		try {
			disconnect();
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}
	try {
		if (m_thread)
		{
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}

	m_bIsStarted = false;
	return true;
}

void CHEOS::OnConnect()
{
	_log.Log(LOG_STATUS, "HEOS by DENON: Connected to: %s:%d", m_IP.c_str(), m_usIPPort);
	m_bDoRestart = false;
	m_bIsStarted = true;
	m_bufferpos = 0;
	sOnConnected(this);
}

void CHEOS::OnDisconnect()
{
	_log.Log(LOG_STATUS, "HEOS by DENON: Disconnected");
}

void CHEOS::OnData(const unsigned char *pData, size_t length)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	ParseData(pData, length);
}

void CHEOS::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR, "HEOS by DENON: Error: %s", e.what());
}

void CHEOS::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "HEOS by DENON: Can not connect to: %s:%d", m_IP.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "HEOS by DENON: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "HEOS by DENON: %s", error.message().c_str());
}

void CHEOS::ParseData(const unsigned char *pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const unsigned char c = pData[ii];
		if (c == 0x0d)
		{
			ii++;
			continue;
		}

		if (c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

/*
bool CHEOS::WriteInt(const unsigned char *pData, const unsigned char Len)
{
	if (!mIsConnected)
	{
		return false;
	}
	write(pData, Len);
	return true;
}
*/

bool CHEOS::WriteInt(const std::string &sendStr)
{
	std::stringstream ssSend;
	std::string	sSend;

	if (!mIsConnected)
	{
		return false;
	}

	ssSend << sendStr << "\r\n";
	sSend = ssSend.str();

	write((const unsigned char*)sSend.c_str(), sSend.size());
	return true;
}

void CHEOS::UpdateNodeStatus(const std::string &DevID, const _eMediaStatus nStatus, const std::string &sStatus)
{
	std::vector<std::vector<std::string> > result;

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szLastUpdate[40];
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
		int(nStatus), sStatus.c_str(), szLastUpdate, m_HwdID, DevID.c_str(), STYPE_Media);
}

void CHEOS::UpdateNodesStatus(const std::string &DevID, const std::string &sStatus)
{
	std::vector<std::vector<std::string> > result;

	time_t now = time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szLastUpdate[40];
	sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

	result = m_sql.safe_query("UPDATE DeviceStatus SET sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
		sStatus.c_str(), szLastUpdate, m_HwdID, DevID.c_str(), STYPE_Media);
}

void CHEOS::AddNode(const std::string &Name, const std::string &PlayerID)
{
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, PlayerID.c_str());
	if (!result.empty()) {
		int ID = atoi(result[0][0].c_str());
		UpdateNode(ID, Name);
		return;
	}

	m_sql.InsertDevice(m_HwdID, PlayerID.c_str(), 1, pTypeLighting2, sTypeAC, STYPE_Media, 0, "Unavailable", Name, 12, 255, 1);

	ReloadNodes();
}

void CHEOS::UpdateNode(const int ID, const std::string &Name)
{
	m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (ID=='%d')", Name.c_str(), m_HwdID, ID);

	ReloadNodes();
}

void CHEOS::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

bool CHEOS::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
	std::vector<HEOSNode>::const_iterator itt;
	for (itt = m_nodes.begin(); itt != m_nodes.end(); ++itt)
	{
		if (itt->DevID == DevID)
		{
			//int iParam = pSen->LIGHTING2.level;
			std::string sParam;
			switch (pSen->LIGHTING2.cmnd)
			{
			case light2_sOn:
				SendCommand("setPlayStatePlay", itt->DevID);
				return true;
			case light2_sGroupOn:
			case light2_sOff:
				SendCommand("setPlayStateStop", itt->DevID);
				return true;
			case light2_sGroupOff:
			case gswitch_sPlay:
				SendCommand("getNowPlaying", itt->DevID);
				SendCommand("setPlayStatePlay", itt->DevID);
				return true;
			case gswitch_sPlayPlaylist:
			case gswitch_sPlayFavorites:
			case gswitch_sStop:
				SendCommand("setPlayStateStop", itt->DevID);
				return true;
			case gswitch_sPause:
				SendCommand("setPlayStatePause", itt->DevID);
				return true;
			case gswitch_sSetVolume:
			default:
				return true;
			}
		}
	}

	return false;
}

void CHEOS::ReloadNodes()
{
	m_nodes.clear();
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,DeviceID, Name, nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		_log.Log(LOG_STATUS, "DENON for HEOS: %d players found.", (int)result.size());
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			HEOSNode pnode;
			pnode.ID = atoi(sd[0].c_str());
			pnode.DevID = atoi(sd[1].c_str());
			pnode.Name = sd[2];
			pnode.nStatus = (_eMediaStatus)atoi(sd[3].c_str());
			pnode.sStatus = sd[4];
			pnode.LastOK = mytime(NULL);

			m_nodes.push_back(pnode);
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "DENON for HEOS: No players found.");
	}
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_HEOSSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			std::string mode2 = request::findValue(&req, "mode2");
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
			if (pBaseHardware->HwdType != HTYPE_HEOS)
				return;
			CHEOS *pHardware = reinterpret_cast<CHEOS*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "HEOSSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
		}

		void CWebServer::Cmd_HEOSMediaCommand(WebEmSession & /*session*/, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			//int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "HEOSMediaCommand";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DS.SwitchType, DS.DeviceID, H.Type, H.ID FROM DeviceStatus DS, Hardware H WHERE (DS.ID=='%q') AND (DS.HardwareID == H.ID)", sIdx.c_str());


			if (result.size() == 1)
			{
				_eSwitchType	sType = (_eSwitchType)atoi(result[0][0].c_str());
				int PlayerID = atoi(result[0][1].c_str());
				_eHardwareTypes	hType = (_eHardwareTypes)atoi(result[0][2].c_str());
				//int HwID = atoi(result[0][3].c_str());
				// Is the device a media Player?
				if (sType == STYPE_Media)
				{
					switch (hType) {
					case HTYPE_HEOS:
						CDomoticzHardwareBase * pBaseHardware = m_mainworker.GetHardwareByIDType(result[0][3].c_str(), HTYPE_HEOS);
						if (pBaseHardware == NULL)
							return;
						CHEOS *pHEOS = reinterpret_cast<CHEOS*>(pBaseHardware);

						pHEOS->SendCommand(sAction, PlayerID);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}
