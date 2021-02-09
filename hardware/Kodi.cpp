#include "stdafx.h"
#include "Kodi.h"
#include "../hardware/hardwaretypes.h"
#include "../main/json_helper.h"
#include "../main/EventSystem.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../notifications/NotificationHelper.h"

#define MAX_TITLE_LEN 40

//Giz: To Author, please try to rebuild this class using ASyncTCP
//Giz: Are those 'new'/thread lines not a memory leak ? Maybe use a shared_ptr for them ?

void CKodiNode::CKodiStatus::Clear()
{
	m_nStatus = MSTAT_UNKNOWN;
	m_sStatus = "";
	m_iPlayerID = -1;
	m_sType = "";
	m_sTitle = "";
	m_sArtist = "";
	m_sChannel = "";
	m_iSeason = 0;
	m_iEpisode = 0;
	m_sLabel = "";
	m_sPercent = "";
	m_sYear = "";
	m_sLive = "";
	m_tLastOK = mytime(nullptr);
}

std::string	CKodiNode::CKodiStatus::LogMessage()
{
	std::string sLogText;
	if (!m_sType.empty())
	{
		if (m_sType == "episode")
		{
			if (m_sShowTitle.length()) sLogText = m_sShowTitle;
			if (m_iSeason) sLogText += " [S" + std::to_string(m_iSeason) + "E" + std::to_string(m_iEpisode) + "]";
			if (m_sTitle.length()) sLogText += ", " + m_sTitle;
			if ((m_sLabel != m_sTitle) && (m_sLabel.length())) sLogText += ", " + m_sLabel;
		}
		else if (m_sType == "song")
		{
			if (m_sArtist.length()) sLogText = m_sArtist;
			if (m_sAlbum.length()) sLogText += " (" + m_sAlbum + ")";
			if (m_sTitle.length()) sLogText += ", " + m_sTitle;
			if ((m_sLabel != m_sTitle) && (m_sLabel.length())) sLogText += ", " + m_sLabel;
		}
		else if (m_sType == "channel")
		{
			if (m_sChannel.length()) sLogText = m_sChannel;
			if (m_sLive.length()) sLogText += " " + m_sLive;
			if (m_sTitle.length()) sLogText += ", " + m_sTitle;
			if ((m_sLabel != m_sTitle) && (m_sLabel != m_sChannel) && (m_sLabel.length())) sLogText += ", " + m_sLabel;
		}
		else if (m_sType == "movie")
		{
			if (m_sTitle.length()) sLogText = m_sTitle;
			if ((m_sLabel != m_sTitle) && (m_sLabel.length())) sLogText += ", " + m_sLabel;
		}
		else if (m_sType == "picture")
		{
			if (m_sLabel.length()) sLogText = m_sLabel;
		}
		else
		{
			if (m_sTitle.length()) sLogText = m_sTitle;
			if ((m_sLabel != m_sTitle) && (m_sLabel.length())) {
				if (sLogText.length()) sLogText += ", ";
				sLogText += m_sLabel;
			}
		}
		if (m_sYear.length()) sLogText += " " + m_sYear;
	}

	return sLogText;
}

std::string	CKodiNode::CKodiStatus::StatusMessage()
{
	// if title is too long shorten it by removing things in brackets, followed by things after a ", "
	std::string	sStatus = LogMessage();
	if (sStatus.length() > MAX_TITLE_LEN)
	{
		boost::algorithm::replace_all(sStatus, " - ", ", ");
		boost::algorithm::replace_first(sStatus, ", ", " - ");  // Leave 1st hyphen (after state)
	}
	while (sStatus.length() > MAX_TITLE_LEN)
	{
		size_t begin = sStatus.find_first_of('(', 0);
		size_t end = sStatus.find_first_of(')', begin);
		if ((std::string::npos == begin) || (std::string::npos == end) || (begin >= end)) break;
		sStatus.erase(begin, end - begin + 1);
	}
	while (sStatus.length() > MAX_TITLE_LEN)
	{
		size_t end = sStatus.find_last_of(',');
		if (std::string::npos == end) break;
		sStatus = sStatus.substr(0, end);
	}
	boost::algorithm::trim(sStatus);
	stdreplace(sStatus, " ,", ",");
	sStatus = sStatus.substr(0, MAX_TITLE_LEN);

	if (m_sPercent.length()) sStatus += ", " + m_sPercent;

	return sStatus;
}

bool CKodiNode::CKodiStatus::LogRequired(CKodiStatus& pPrevious)
{
	return ((LogMessage() != pPrevious.LogMessage()) || (m_nStatus != pPrevious.Status()));
}

bool CKodiNode::CKodiStatus::UpdateRequired(CKodiStatus& pPrevious)
{
	return ((StatusMessage() != pPrevious.StatusMessage()) || (m_nStatus != pPrevious.Status()));
}

bool CKodiNode::CKodiStatus::OnOffRequired(CKodiStatus& pPrevious)
{
	return ((m_nStatus == MSTAT_OFF) || (pPrevious.Status() == MSTAT_OFF)) && (m_nStatus != pPrevious.Status());
}

_eNotificationTypes	CKodiNode::CKodiStatus::NotificationType()
{
	switch (m_nStatus)
	{
	case MSTAT_OFF:		return NTYPE_SWITCH_OFF;
	case MSTAT_ON:		return NTYPE_SWITCH_ON;
	case MSTAT_PAUSED:	return NTYPE_PAUSED;
	case MSTAT_VIDEO:	return NTYPE_VIDEO;
	case MSTAT_AUDIO:	return NTYPE_AUDIO;
	case MSTAT_PHOTO:	return NTYPE_PHOTO;
	default:			return NTYPE_SWITCH_OFF;
	}
}

CKodiNode::CKodiNode(boost::asio::io_service *pIos, const int pHwdID, const int PollIntervalsec, const int pTimeoutMs,
	const std::string& pID, const std::string& pName, const std::string& pIP, const std::string& pPort)
{
	m_Busy = false;
	m_Stoppable = false;
	m_PlaylistPosition = 0;

	m_Ios = pIos;
	m_HwdID = pHwdID;
	m_DevID = atoi(pID.c_str());
	sprintf(m_szDevID, "%X%02X%02X%02X", 0, 0, (m_DevID & 0xFF00) >> 8, m_DevID & 0xFF);
	m_Name = pName;
	m_IP = pIP;
	m_Port = pPort;
	m_iTimeoutCnt = (pTimeoutMs > 999) ? pTimeoutMs / 1000 : pTimeoutMs;
	m_iPollIntSec = PollIntervalsec;
	m_iMissedPongs = 0;

	m_Socket = nullptr;

	_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Created.", m_Name.c_str());

	std::vector<std::vector<std::string> > result2;
	result2 = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == 1)", m_HwdID, m_szDevID);
	if (result2.size() == 1)
	{
		m_ID = atoi(result2[0][0].c_str());
		m_PreviousStatus.Status((_eMediaStatus)atoi(result2[0][1].c_str()));
		m_PreviousStatus.Status(result2[0][2]);
	}
	m_CurrentStatus = m_PreviousStatus;
}

CKodiNode::~CKodiNode()
{
	handleDisconnect();
	_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Destroyed.", m_Name.c_str());
}

void CKodiNode::handleMessage(std::string& pMessage)
{
	try
	{
		Json::Value root;
		std::string	sMessage;
		std::stringstream ssMessage;

		_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Handling message: '%s'.", m_Name.c_str(), pMessage.c_str());
		bool bRet = ParseJSon(pMessage, root);
		if ((!bRet) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "Kodi: (%s) PARSE ERROR: '%s'", m_Name.c_str(), pMessage.c_str());
		}
		else if (root["error"].empty() != true)
		{
			int		iMessageID = root["id"].asInt();
			switch (iMessageID)
			{
			case 2002: // attempt to start music playlist (error is because playlist does not exist, try video)
				m_PlaylistType = "1";
				ssMessage << R"({"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":)" << m_PlaylistType << R"(,"item":{"directory": "special://profile/playlists/video/)"
					  << m_Playlist << R"(.xsp", "media":"video"}},"id":2003})";
				handleWrite(ssMessage.str());
				break;
			case 2003: // error because video playlist does not exist, stop.
				_log.Log(LOG_ERROR, "Kodi: (%s) Playlist '%s' could not be added, probably does not exist.", m_Name.c_str(), m_Playlist.c_str());
				break;
			default:
				/* e.g {"error":{"code":-32100,"message":"Failed to execute method."},"id":1001,"jsonrpc":"2.0"}	*/
				_log.Log(LOG_ERROR, "Kodi: (%s) Code %d Text '%s' ID '%d' Request '%s'", m_Name.c_str(), root["error"]["code"].asInt(), root["error"]["message"].asCString(), root["id"].asInt(), m_sLastMessage.c_str());
			}
		}
		else
		{
			// Kodi generated messages
			if (root.isMember("params"))
			{
				if (root["params"].isMember("sender"))
				{
					if (root["params"]["sender"] != "xmbc")
					{
						if (root.isMember("method"))
						{
							if ((root["method"] == "Player.OnStop") || (root["method"] == "System.OnWake"))
							{
								m_CurrentStatus.Clear();
								m_CurrentStatus.Status(MSTAT_ON);
								UpdateStatus();
							}
							else if ((root["method"] == "Player.OnPlay") || (root["method"] == "Player.OnResume"))
							{
								m_CurrentStatus.Clear();
								m_CurrentStatus.PlayerID(root["params"]["data"]["player"]["playerid"].asInt());
								if (root["params"]["data"]["item"]["type"] == "picture")
									m_CurrentStatus.Status(MSTAT_PHOTO);
								else if (root["params"]["data"]["item"]["type"] == "episode")
									m_CurrentStatus.Status(MSTAT_VIDEO);
								else if (root["params"]["data"]["item"]["type"] == "channel")
									m_CurrentStatus.Status(MSTAT_VIDEO);
								else if (root["params"]["data"]["item"]["type"] == "movie")
									m_CurrentStatus.Status(MSTAT_VIDEO);
								else if (root["params"]["data"]["item"]["type"] == "song")
									m_CurrentStatus.Status(MSTAT_AUDIO);
								else if (root["params"]["data"]["item"]["type"] == "musicvideo")
									m_CurrentStatus.Status(MSTAT_VIDEO);
								else
								{
									_log.Log(LOG_ERROR, "Kodi: (%s) Message error, unknown type in OnPlay/OnResume message: '%s' from '%s'", m_Name.c_str(), root["params"]["data"]["item"]["type"].asCString(), pMessage.c_str());
								}

								if (!m_CurrentStatus.PlayerID().empty()) // if we now have a player id then request more details
								{
									sMessage = R"({"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
										   R"(,"properties":["artist","album","year","channel","showtitle","season","episode","title"]}})";
									handleWrite(sMessage);
								}
							}
							else if (root["method"] == "Player.OnPause")
							{
								m_CurrentStatus.Status(MSTAT_PAUSED);
								UpdateStatus();
							}
							else if (root["method"] == "Player.OnSeek")
							{
								if (!m_CurrentStatus.PlayerID().empty())
									sMessage = R"({"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
										   R"(,"properties":["live","percentage","speed"]}})";
								else
									sMessage = R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1005})";
								handleWrite(sMessage);
							}
							else if ((root["method"] == "System.OnQuit") || (root["method"] == "System.OnSleep") || (root["method"] == "System.OnRestart"))
							{
								m_CurrentStatus.Clear();
								m_CurrentStatus.Status(MSTAT_OFF);
								UpdateStatus();
							}
							else if (root["method"] == "Application.OnVolumeChanged")
							{
								float		iVolume = root["params"]["data"]["volume"].asFloat();
								bool		bMuted = root["params"]["data"]["muted"].asBool();
								_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Volume changed to %3.5f, Muted: %s.", m_Name.c_str(), iVolume, bMuted?"true":"false");
							}
							else if (root["method"] == "Player.OnSpeedChanged")
							{
								_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Speed changed.", m_Name.c_str());
							}
							else
								_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Message warning, unhandled method: '%s'", m_Name.c_str(), root["method"].asCString());
						}
						else
							_log.Log(LOG_ERROR, "Kodi: (%s) Message error, params but no method: '%s'", m_Name.c_str(), pMessage.c_str());
					}
					else
						_log.Log(LOG_ERROR, "Kodi: (%s) Message error, invalid sender: '%s'", m_Name.c_str(), root["params"]["sender"].asCString());
				}
				else
					_log.Log(LOG_ERROR, "Kodi: (%s) Message error, params but no sender: '%s'", m_Name.c_str(), pMessage.c_str());
			}
			else  // responses to queries
			{
				if ((root.isMember("result") && (root.isMember("id"))))
				{
					bool		bCanShutdown = false;
					bool		bCanHibernate = false;
					bool		bCanSuspend = false;
					int		iMessageID = root["id"].asInt();
					switch (iMessageID)
					{
					case 1001:		//Ping response
						if (root["result"] == "pong")
						{
							m_iMissedPongs = 0;
							m_CurrentStatus.Clear();
							m_CurrentStatus.Status(MSTAT_ON);
							UpdateStatus();
						}
						break;
					case 1002:		//Poll response
						if (root["result"].isMember("live"))
						{
							m_CurrentStatus.Live(root["result"]["live"].asBool());
						}
						if (root["result"].isMember("percentage"))
						{
							m_CurrentStatus.Percent(root["result"]["percentage"].asFloat());
						}
						if (root["result"].isMember("speed"))
						{
							if (!root["result"]["speed"].asInt())
								m_CurrentStatus.Status(MSTAT_PAUSED);	// useful when Domoticz restarts when media aleady paused
							if (root["result"]["speed"].asInt() && m_CurrentStatus.Status() == MSTAT_PAUSED)
							{
								// Buffering when playing internet streams show 0 speed but don't trigger OnPause/OnPlay so force a refresh when speed is not 0 again
								sMessage = R"({"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
									   R"(,"properties":["artist","album","year","channel","showtitle","season","episode","title"]}})";
								handleWrite(sMessage);
							}
						}
						UpdateStatus();
						break;
					case 1003:		//OnPlay media details response
						if (root["result"].isMember("item"))
						{
							if (root["result"]["item"].isMember("type"))			m_CurrentStatus.Type(root["result"]["item"]["type"].asCString());
							if (m_CurrentStatus.Type() == "song")
							{
								m_CurrentStatus.Status(MSTAT_AUDIO);
								if (root["result"]["item"]["artist"][0].empty() != true)
								{
									m_CurrentStatus.Artist(root["result"]["item"]["artist"][0].asCString());
								}
								if (root["result"]["item"].isMember("album"))
								{
									m_CurrentStatus.Album(root["result"]["item"]["album"].asCString());
								}
							}
							if (m_CurrentStatus.Type() == "episode")
							{
								m_CurrentStatus.Status(MSTAT_VIDEO);
								if (root["result"]["item"].isMember("showtitle"))	m_CurrentStatus.ShowTitle(root["result"]["item"]["showtitle"].asCString());
								if (root["result"]["item"].isMember("season"))		m_CurrentStatus.Season((int)root["result"]["item"]["season"].asInt());
								if (root["result"]["item"].isMember("episode"))		m_CurrentStatus.Episode((int)root["result"]["item"]["episode"].asInt());
							}
							if (m_CurrentStatus.Type() == "channel")
							{
								m_CurrentStatus.Status(MSTAT_VIDEO);
								if (root["result"]["item"].isMember("channel"))		m_CurrentStatus.Channel(root["result"]["item"]["channel"].asCString());
							}
							if (m_CurrentStatus.Type() == "unknown")
							{
								m_CurrentStatus.Status(MSTAT_VIDEO);
							}
							if (m_CurrentStatus.Type() == "picture")
							{
								m_CurrentStatus.Status(MSTAT_PHOTO);
							}
							if (root["result"]["item"].isMember("title"))			m_CurrentStatus.Title(root["result"]["item"]["title"].asCString());
							if (root["result"]["item"].isMember("year"))			m_CurrentStatus.Year(root["result"]["item"]["year"].asInt());
							if (root["result"]["item"].isMember("label"))			m_CurrentStatus.Label(root["result"]["item"]["label"].asCString());
							if ((!m_CurrentStatus.PlayerID().empty()) && (m_CurrentStatus.Type() != "picture")) // request final details
							{
								sMessage = R"({"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
									   R"(,"properties":["live","percentage","speed"]}})";
								handleWrite(sMessage);
							}
							UpdateStatus();
						}
						break;
					case 1004:		//Shutdown details response
						{
							m_Stoppable = false;
							std::string	sAction = "Nothing";
							if (root["result"].isMember("canshutdown"))
							{
								bCanShutdown = root["result"]["canshutdown"].asBool();
								if (bCanShutdown) sAction = "Shutdown";
							}
							if (root["result"].isMember("canhibernate") && sAction != "Nothing")
							{
								bCanHibernate = root["result"]["canhibernate"].asBool();
								if (bCanHibernate) sAction = "Hibernate";
							}
							if (root["result"].isMember("cansuspend")&& sAction != "Nothing")
							{
								bCanSuspend = root["result"]["cansuspend"].asBool();
								if (bCanSuspend) sAction = "Suspend";
							}
							_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Switch Off: CanShutdown:%s, CanHibernate:%s, CanSuspend:%s. %s requested.", m_Name.c_str(),
								bCanShutdown ? "true" : "false", bCanHibernate ? "true" : "false", bCanSuspend ? "true" : "false", sAction.c_str());

							if (sAction != "Nothing")
							{
								m_Stoppable = true;
								sMessage = R"({"jsonrpc":"2.0","method":"System.)" + sAction + R"(","id":1008})";
								handleWrite(sMessage);
							}
						}
						break;
					case 1005:		//GetPlayers response, normally requried when Domoticz starts up and media is already streaming
						if (root["result"][0].isMember("playerid"))
						{
							m_CurrentStatus.PlayerID(root["result"][0]["playerid"].asInt());
							sMessage = R"({"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
								   R"(,"properties":["artist","album","year","channel","showtitle","season","episode","title"]}})";
							handleWrite(sMessage);
						}
						break;
					case 1006:		//Remote Control response
						if (root["result"] != "OK")
							_log.Log(LOG_ERROR, "Kodi: (%s) Send Command Failed: '%s'", m_Name.c_str(), root["result"].asCString());
						break;
					case 1007:		//Can Shutdown response (after connect)
						handleWrite(std::string(R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1005})"));
						if (root["result"].isMember("canshutdown"))
						{
							bCanShutdown = root["result"]["canshutdown"].asBool();
						}
						if (root["result"].isMember("canhibernate"))
						{
							bCanHibernate = root["result"]["canhibernate"].asBool();
						}
						if (root["result"].isMember("cansuspend"))
						{
							bCanSuspend = root["result"]["cansuspend"].asBool();
						}
						m_Stoppable = (bCanShutdown || bCanHibernate || bCanSuspend);
						break;
					case 1008:		//Shutdown response
						if (root["result"] == "OK")
							_log.Log(LOG_NORM, "Kodi: (%s) Shutdown command accepted.", m_Name.c_str());
						break;
					case 1009:		//SetVolume response
						_log.Log(LOG_NORM, "Kodi: (%s) Volume set to %d.", m_Name.c_str(), root["result"].asInt());
						break;
					case 1010:		//Execute Addon response
						_log.Log(LOG_NORM, "Kodi: (%s) Executed Addon %s.", m_Name.c_str(), root["result"].asString().c_str());
						break;
					// 2000+ messages relate to playlist triggering functionality
					case 2000: // clear video playlist response
						handleWrite(R"({"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":1},"id":2001})");
						break;
					case 2001: // clear music playlist response
						ssMessage << R"({"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":)" << m_PlaylistType
							  << R"(,"item":{"directory": "special://profile/playlists/music/)" << m_Playlist << R"(.xsp", "media":"music"}},"id":2002})";
						handleWrite(ssMessage.str());
						break;
					case 2002: // attempt to add playlist response
					case 2003:
						ssMessage << R"({"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"playlistid":)" << m_PlaylistType << ",\"position\":" << m_PlaylistPosition
							  << "}},\"id\":2004}";
						handleWrite(ssMessage.str());
						break;
					case 2004: // signal outcome
						if (root["result"] == "OK")
							_log.Log(LOG_NORM, "Kodi: (%s) Playlist command '%s' at position %d accepted.", m_Name.c_str(), m_Playlist.c_str(), m_PlaylistPosition);
						break;
					// 2100+ messages relate to favorites triggering functionality
					case 2100: // return favorites response
						if (root["result"].isMember("favourites")) {
							if (root["result"]["limits"].isMember("total"))
							{
								int	iFavCount = root["result"]["limits"]["total"].asInt();
								// set play position to last entry if greater than total
								if (m_PlaylistPosition < 0) m_PlaylistPosition = 0;
								if (m_PlaylistPosition >= iFavCount) m_PlaylistPosition = iFavCount - 1;
								if (iFavCount)
									for (int i = 0; i < iFavCount; i++) {
										_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Favourites %d is '%s', type '%s'.", m_Name.c_str(), i, root["result"]["favourites"][i]["title"].asCString(), root["result"]["favourites"][i]["type"].asCString());
										std::string sType = root["result"]["favourites"][i]["type"].asCString();
										if (i == m_PlaylistPosition) {
											if (sType == "media") {
												std::string sPath = root["result"]["favourites"][i]["path"].asCString();
												_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Favourites %d has path '%s' and will be played.", m_Name.c_str(), i, sPath.c_str());
												ssMessage << R"({"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"file":")" << sPath
													  << R"("}},"id":2101})";
												handleWrite(ssMessage.str());
												break;
											}
											_log.Log(LOG_NORM,
												 "Kodi: (%s) Requested Favourite ('%s') is not playable, next playable item will be selected.",
												 m_Name.c_str(), root["result"]["favourites"][i]["title"].asCString());
											m_PlaylistPosition++;
										}
									}
								else
									_log.Log(LOG_NORM, "Kodi: (%s) No Favourites returned.", m_Name.c_str());
							}
						}
						break;
					case 2101: // signal outcome
						if (root["result"] == "OK")
							_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Favourite play request successful.", m_Name.c_str());
						break;
					default:
						_log.Log(LOG_ERROR, "Kodi: (%s) Message error, unknown ID found: '%s'", m_Name.c_str(), pMessage.c_str());
					}
				}
			}
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Exception: %s", m_Name.c_str(), e.what());
	}
}

void CKodiNode::UpdateStatus()
{
	//This has to be rebuild! No direct poking in the database, please use CMainWorker::UpdateDevice

	std::vector<std::vector<std::string> > result;
	m_CurrentStatus.LastOK(mytime(nullptr));

	// 1:	Update the DeviceStatus
	if (m_CurrentStatus.UpdateRequired(m_PreviousStatus))
	{
		result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
			int(m_CurrentStatus.Status()), m_CurrentStatus.StatusMessage().c_str(), m_CurrentStatus.LastOK().c_str(), m_HwdID, m_szDevID, STYPE_Media);
	}

	// 2:	Log the event if the actual status has changed (not counting the percentage)
	std::string	sLogText = m_CurrentStatus.StatusText();
	if (m_CurrentStatus.LogRequired(m_PreviousStatus))
	{
		if (m_CurrentStatus.IsStreaming()) sLogText += " - " + m_CurrentStatus.LogMessage();
		result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue, User) VALUES (%d, %d, '%q','%q')", m_ID, int(m_CurrentStatus.Status()), sLogText.c_str(), "Kodi");
		_log.Log(LOG_NORM, "Kodi: (%s) Event: '%s'.", m_Name.c_str(), sLogText.c_str());
	}

	// 3:	Trigger On/Off actions
	if (m_CurrentStatus.OnOffRequired(m_PreviousStatus))
	{
		result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (ID = '%q') AND (Unit == 1)", m_HwdID, m_szDevID);
		if (!result.empty())
		{
			m_sql.HandleOnOffAction(m_CurrentStatus.IsOn(), result[0][0], result[0][1]);
		}
	}

	// 4:	Trigger Notifications & events on status change
	if (m_CurrentStatus.Status() != m_PreviousStatus.Status())
	{
		m_notifications.CheckAndHandleNotification(m_ID, m_Name, m_CurrentStatus.NotificationType(), sLogText);
		m_mainworker.m_eventsystem.ProcessDevice(m_HwdID, m_ID, 1, int(pTypeLighting2), int(sTypeAC), 12, 100, int(m_CurrentStatus.Status()), m_CurrentStatus.StatusMessage().c_str());
	}

	m_PreviousStatus = m_CurrentStatus;
}

void CKodiNode::handleConnect()
{
	try
	{
		if (!IsStopRequested(0) && !m_Socket)
		{
			m_iMissedPongs = 0;
			boost::system::error_code ec;
			boost::asio::ip::tcp::resolver resolver(*m_Ios);
			boost::asio::ip::tcp::resolver::query query(m_IP, (m_Port[0] != '-' ? m_Port : m_Port.substr(1)));
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			boost::asio::ip::tcp::endpoint endpoint = *iter;
			m_Socket = new boost::asio::ip::tcp::socket(*m_Ios);
			m_Socket->connect(endpoint, ec);
			if (!ec)
			{
				_log.Log(LOG_NORM, "Kodi: (%s) Connected to '%s:%s'.", m_Name.c_str(), m_IP.c_str(), (m_Port[0] != '-' ? m_Port.c_str() : m_Port.substr(1).c_str()));
				if (m_CurrentStatus.Status() == MSTAT_OFF)
				{
					m_CurrentStatus.Clear();
					m_CurrentStatus.Status(MSTAT_ON);
					UpdateStatus();
				}
				m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [p = shared_from_this()](auto err, auto bytes) { p->handleRead(err, bytes); });
				handleWrite(std::string(R"({"jsonrpc":"2.0","method":"System.GetProperties","params":{"properties":["canhibernate","cansuspend","canshutdown"]},"id":1007})"));
			}
			else
			{
				if (
					(ec.value() != 113) &&
					(ec.value() != 111) &&
					(ec.value() != 10060) &&
					(ec.value() != 10061) &&
					(ec.value() != 10064) //&&
					//(ec.value() != 10061)
					)
				{
					_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Connect to '%s:%s' failed: (%d) %s", m_Name.c_str(), m_IP.c_str(), (m_Port[0] != '-' ? m_Port.c_str() : m_Port.substr(1).c_str()), ec.value(), ec.message().c_str());
				}
				delete m_Socket;
				m_Socket = nullptr;
				m_CurrentStatus.Clear();
				m_CurrentStatus.Status(MSTAT_OFF);
				UpdateStatus();
			}
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Exception: '%s' connecting to '%s'", m_Name.c_str(), e.what(), m_IP.c_str());
	}
}

void CKodiNode::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if (!e)
	{
		//do something with the data
		std::string sData(m_Buffer.begin(), bytes_transferred);
		sData = m_RetainedData + sData;  // if there was some data left over from last time add it back in
		int iPos = 1;
		while (iPos) {
			iPos = sData.find("}{", 0) + 1;		//  Look for message separater in case there is more than one
			if (!iPos) // no, just one or part of one
			{
				if ((sData.substr(sData.length()-1, 1) == "}") &&
					(std::count(sData.begin(), sData.end(), '{') == std::count(sData.begin(), sData.end(), '}'))) // whole message so process
				{
					handleMessage(sData);
					sData = "";
				}
			}
			else  // more than one message so process the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				handleMessage(sMessage);
			}
		}
		m_RetainedData = sData;

		//ready for next read
		if (!IsStopRequested(0) && m_Socket)
		{
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [p = shared_from_this()](auto &&err, auto &&bytes) { p->handleRead(err, bytes); });
		}
	}
	else
	{
		if (e.value() != 1236)		// local disconnect cause by hardware reload
		{
			if ((e.value() != 2) && (e.value() != 121))	// Semaphore tmieout expiry or end of file aka 'lost contact'
				_log.Log(LOG_ERROR, "Kodi: (%s) Async Read Exception: %d, %s", m_Name.c_str(), e.value(), e.message().c_str());
			m_CurrentStatus.Clear();
			m_CurrentStatus.Status(MSTAT_OFF);
			UpdateStatus();
			handleDisconnect();
		}
	}
}

void CKodiNode::handleWrite(const std::string &pMessage)
{
	if (!IsStopRequested(0)) {
		if (m_Socket)
		{
			_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Sending data: '%s'", m_Name.c_str(), pMessage.c_str());
			m_Socket->write_some(boost::asio::buffer(pMessage.c_str(), pMessage.length()));
			m_sLastMessage = pMessage;
		}
		else
    {
	    _log.Log(LOG_ERROR, "Kodi: (%s) Data not sent to nullptr socket: '%s'", m_Name.c_str(), pMessage.c_str());
    }
  }
}

void CKodiNode::handleDisconnect()
{
	if (m_Socket)
	{
		_log.Log(LOG_NORM, "Kodi: (%s) Disconnected.", m_Name.c_str());
		boost::system::error_code	ec;
		m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_Socket->close();
		delete m_Socket;
		m_Socket = nullptr;
	}
}

void CKodiNode::Do_Work()
{
	m_Busy = true;
	_log.Debug(DEBUG_HARDWARE, "Kodi: (%s) Entering work loop.", m_Name.c_str());
	int	iPollCount = 2;

	try
	{
		while (!IsStopRequested(1000))
		{
			if (!m_Socket)
			{
				handleConnect();
				iPollCount = 1;
			}

			if (!iPollCount--)
			{
				iPollCount = m_iPollIntSec - 1;
				std::string	sMessage;
				if (m_CurrentStatus.IsStreaming())
				{	// Update percentage if playing media (required because Player.OnPropertyChanged never get received as of Kodi 'Helix')
					if (!m_CurrentStatus.PlayerID().empty())
						sMessage = R"({"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":)" + m_CurrentStatus.PlayerID() +
							   R"(,"properties":["live","percentage","speed"]}})";
					else
						sMessage = R"({"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1005})";
				}
				else
				{
					sMessage = R"({"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1001})";
					if (m_iMissedPongs++ > m_iTimeoutCnt)
					{
						_log.Log(LOG_NORM, "Kodi: (%s) Missed %d pings, assumed off.", m_Name.c_str(), m_iTimeoutCnt);
						boost::system::error_code	ec;
						m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
						continue;
					};
				}
				handleWrite(sMessage);
			}
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Exception: %s", m_Name.c_str(), e.what());
	}
	_log.Log(LOG_NORM, "Kodi: (%s) Exiting work loop.", m_Name.c_str());
	m_Busy = false;
	delete m_Socket;
	m_Socket = nullptr;
}

void CKodiNode::SendCommand(const std::string &command)
{
	std::string	sKodiCall;
	std::string sKodiParam;
	if (command == "Home")
	{
		sKodiCall = "Input.Home";
	}
	else if (command == "Up")
	{
		sKodiCall = "Input.Up";
	}
	else if (command == "Down")
	{
		sKodiCall = "Input.Down";
	}
	else if (command == "Left")
	{
		sKodiCall = "Input.Left";
	}
	else if (command == "Right")
	{
		sKodiCall = "Input.Right";
	}
	else  // Assume generic ExecuteAction  for any unrecognised strings
	{
		sKodiCall = "Input.ExecuteAction";
		std::string	sLower = command;
		std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);
		sKodiParam = sLower;
	}

	if (sKodiCall.length())
	{
		//		http://kodi.wiki/view/JSON-RPC_API/v6#Input.Action
		//		{ "jsonrpc": "2.0", "method": "Input.ExecuteAction", "params": { "action": "stop" }, "id": 1006 }
		std::string sMessage = R"({"jsonrpc":"2.0","method":")" + sKodiCall + R"(","params":{)";
		if (sKodiParam.length())
			sMessage += R"("action":")" + sKodiParam + "\"";
		sMessage += "},\"id\":1006}";

		if (m_Socket != nullptr)
		{
			handleWrite(sMessage);
			_log.Log(LOG_NORM, "Kodi: (%s) Sent command: '%s %s'.", m_Name.c_str(), sKodiCall.c_str(), sKodiParam.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "Kodi: (%s) Command not sent, Kodi is not connected: '%s %s'.", m_Name.c_str(), sKodiCall.c_str(), sKodiParam.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Command: '%s'. Unknown command.", m_Name.c_str(), command.c_str());
	}
}

void CKodiNode::SendCommand(const std::string &command, const int iValue)
{
	std::stringstream ssMessage;
	std::string	sMessage;
	std::string	sKodiCall;
	if (command == "setvolume")
	{
		sKodiCall = "Set Volume";
		ssMessage << R"({"jsonrpc":"2.0","method":"Application.SetVolume","params":{"volume":)" << iValue << "},\"id\":1009}";
		sMessage = ssMessage.str();
	}

	if (command == "playlist")
	{
		// clear any current playlists starting with audio, state machine in handleMessage will take care of the rest
		m_PlaylistPosition = iValue;
		sMessage = R"({"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":0},"id":2000})";
	}

	if (command == "favorites")
	{
		// Favorites are effectively a playlist but rewuire different handling to start items playing
		sKodiCall = "Favourites";
		m_PlaylistPosition = iValue;
		sMessage = R"({"jsonrpc":"2.0","method":"Favourites.GetFavourites","params":{"properties":["path"]},"id":2100})";
	}

	if (command == "execute")
	{
		sKodiCall = "Execute Addon " + m_ExecuteCommand;
		//		ssMessage << "{\"jsonrpc\":\"2.0\",\"method\":\"Addons.GetAddons\",\"id\":1010}";
		ssMessage << R"({"jsonrpc":"2.0","method":"Addons.ExecuteAddon","params":{"addonid":")" << m_ExecuteCommand << R"("},"id":1010})";
		sMessage = ssMessage.str();
		m_ExecuteCommand = "";
	}

	if (sMessage.length())
	{
		if (m_Socket != nullptr)
		{
			handleWrite(sMessage);
			_log.Log(LOG_NORM, "Kodi: (%s) Sent command: '%s'.", m_Name.c_str(), sKodiCall.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "Kodi: (%s) Command not sent, Kodi is not connected: '%s'.", m_Name.c_str(), sKodiCall.c_str());
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Command: '%s'. Unknown command.", m_Name.c_str(), command.c_str());
	}
}

bool CKodiNode::SendShutdown()
{
	std::string sMessage = R"({"jsonrpc":"2.0","method":"System.GetProperties","params":{"properties":["canhibernate","cansuspend","canshutdown"]},"id":1004})";
	handleWrite(sMessage);

	if (m_Stoppable) _log.Log(LOG_NORM, "Kodi: (%s) Shutdown requested and is supported.", m_Name.c_str());
	else 			 _log.Log(LOG_NORM, "Kodi: (%s) Shutdown requested but is probably not supported.", m_Name.c_str());
	return m_Stoppable;
}

void CKodiNode::SetPlaylist(const std::string& playlist)
{
	m_Playlist = playlist;
	m_PlaylistType = "0";
	if (m_Playlist.length() > 0)
	{
		if (m_CurrentStatus.IsStreaming())  // Stop Kodi if it is currently stream media
		{
			SendCommand("stop");
		}
	}
}

void CKodiNode::SetExecuteCommand(const std::string& command)
{
	m_ExecuteCommand = command;
}

std::vector<std::shared_ptr<CKodiNode> > CKodi::m_pNodes;

CKodi::CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms)
{
	m_HwdID = ID;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CKodi::CKodi(const int ID)
{
	m_HwdID = ID;
	SetSettings(10, 3000);
}

CKodi::~CKodi()
{
	m_bIsStarted = false;
}

bool CKodi::StartHardware()
{
	StopHardware();

	RequestStart();

	m_bIsStarted = true;
	sOnConnected(this);

	StartHeartbeatThread();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	_log.Log(LOG_STATUS, "Kodi: Started");

	return true;
}

bool CKodi::StopHardware()
{
	StopHeartbeatThread();

	try {
		//needs to be tested by the author if we can remove the try/catch here
		if (m_thread)
		{
			RequestStop();
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{

	}
	m_bIsStarted = false;
	return true;
}

void CKodi::Do_Work()
{
	int scounter = 0;

	ReloadNodes();

	while (!IsStopRequested(500))
	{
		if (scounter++ >= (m_iPollInterval*2))
		{
			std::lock_guard<std::mutex> l(m_mutex);

			scounter = 0;
			bool bWorkToDo = false;
			for (const auto &node : m_pNodes)
			{
				if (!node->IsBusy())
				{
					_log.Log(LOG_NORM, "Kodi: (%s) - Restarting thread.", node->m_Name.c_str());
					boost::thread *tAsync = new boost::thread(&CKodiNode::Do_Work, node);
					SetThreadName(tAsync->native_handle(), "KodiNode");
					m_ios.stop();
				}
				if (node->IsOn())
					bWorkToDo = true;
			}

			if (bWorkToDo && m_ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				m_ios.reset();
				// Note that this is the only thread that handles async i/o so we don't
				// need to worry about locking or concurrency issues when processing messages
				_log.Log(LOG_NORM, "Kodi: Restarting I/O service thread.");
				boost::thread bt([p = &m_ios] { p->run(); });
				SetThreadName(bt.native_handle(), "KodiIO");
			}
		}
	}
	UnloadNodes();

	_log.Log(LOG_STATUS, "Kodi: Worker stopped...");
}

void CKodi::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

bool CKodi::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;

	for (const auto &node : m_pNodes)
	{
		if (node->m_DevID == DevID)
		{
			if (node->IsOn())
			{
				int iParam = pSen->LIGHTING2.level;
				switch (pSen->LIGHTING2.cmnd)
				{
				case light2_sOff:
				case light2_sGroupOff:
					return node->SendShutdown();
				case gswitch_sStop:
					node->SendCommand("stop");
					return true;
				case gswitch_sPlay:
					node->SendCommand("play");
					return true;
				case gswitch_sPause:
					node->SendCommand("pause");
					return true;
				case gswitch_sSetVolume:
					node->SendCommand("setvolume", iParam);
					return true;
				case gswitch_sPlayPlaylist:
					node->SendCommand("playlist", iParam);
					return true;
				case gswitch_sPlayFavorites:
					node->SendCommand("favorites", iParam);
					return true;
				case gswitch_sExecute:
					node->SendCommand("execute", iParam);
					return true;
				default:
					return true;
				}
			}
			else
				_log.Log(LOG_NORM, "Kodi: (%s) Command not sent, Device is 'Off'.", node->m_Name.c_str());
		}
	}

	_log.Log(LOG_ERROR, "Kodi: (%ld) Shutdown. Device not found.", DevID);
	return false;
}

void CKodi::AddNode(const std::string &Name, const std::string &IPAddress, const int Port)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (!result.empty())
		return; //Already exists
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress, Timeout) VALUES (%d, '%q', '%q', %d)", m_HwdID, Name.c_str(), IPAddress.c_str(), Port);

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress='%q')", m_HwdID, Name.c_str(), IPAddress.c_str());
	if (result.empty())
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	m_sql.InsertDevice(m_HwdID, szID, 1, pTypeLighting2, sTypeAC, STYPE_Media, 0, "Unavailable", Name, 12, 255, 1);

	ReloadNodes();
}

bool CKodi::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);
	if (result.empty())
		return false; //Not Found!?

	m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q', Timeout=%d WHERE (HardwareID==%d) AND (ID==%d)", Name.c_str(), IPAddress.c_str(), Port, m_HwdID, ID);

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also update Light/Switch
	m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')", Name.c_str(), m_HwdID, szID);
	ReloadNodes();
	return true;
}

void CKodi::RemoveNode(const int ID)
{
	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);

	//Also delete the switch
	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szID);
	ReloadNodes();
}

void CKodi::RemoveAllNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);

	//Also delete the all switches
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	ReloadNodes();
}

void CKodi::ReloadNodes()
{
	UnloadNodes();

	m_ios.reset();	// in case this is not the first time in

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		std::lock_guard<std::mutex> l(m_mutex);

		// create a vector to hold the nodes
		for (const auto &sd : result)
		{
			auto pNode = std::make_shared<CKodiNode>(&m_ios, m_HwdID, m_iPollInterval, m_iPingTimeoutms, sd[0], sd[1], sd[2], sd[3]);
			m_pNodes.push_back(pNode);
		}
		// start the threads to control each kodi
		for (const auto &m_pNode : m_pNodes)
		{
			_log.Log(LOG_NORM, "Kodi: (%s) Starting thread.", m_pNode->m_Name.c_str());
			boost::thread *tAsync = new boost::thread(&CKodiNode::Do_Work, m_pNode);
			SetThreadName(tAsync->native_handle(), "KodiNode");
		}
		sleep_milliseconds(100);
		_log.Log(LOG_NORM, "Kodi: Starting I/O service thread.");
		boost::thread bt([p = &m_ios] { p->run(); });
		SetThreadName(bt.native_handle(), "KodiIO");
	}
}

void CKodi::UnloadNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);

	m_ios.stop();	// stop the service if it is running
	sleep_milliseconds(100);

	while (((!m_pNodes.empty()) || (!m_ios.stopped())))
	{
		for (auto itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
		{
			(*itt)->StopRequest();
			if (!(*itt)->IsBusy())
			{
				_log.Log(LOG_NORM, "Kodi: (%s) Removing device.", (*itt)->m_Name.c_str());
				m_pNodes.erase(itt);
				break;
			}
		}
		sleep_milliseconds(150);
	}
	m_pNodes.clear();
}

void CKodi::SendCommand(const int ID, const std::string &command)
{
	for (const auto &node : m_pNodes)
	{
		if (node->m_ID == ID)
		{
			node->SendCommand(command);
			return;
		}
	}

	_log.Log(LOG_ERROR, "Kodi: (%d) Command: '%s'. Device not found.", ID, command.c_str());
}

bool CKodi::SetPlaylist(const int ID, const std::string &playlist)
{
	for (const auto &node : m_pNodes)
	{
		if (node->m_ID == ID)
		{
			node->SetPlaylist(playlist);
			return true;
		}
	}
	return false;
}

bool CKodi::SetExecuteCommand(const int ID, const std::string &command)
{
	for (const auto &node : m_pNodes)
	{
		if (node->m_ID == ID)
		{
			node->SetExecuteCommand(command);
			return true;
		}
	}
	return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_KodiGetNodes(WebEmSession & session, const request& req, Json::Value &root)
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
			if (pHardware->HwdType != HTYPE_Kodi)
				return;

			root["status"] = "OK";
			root["title"] = "KodiGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", iHardwareID);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
					root["result"][ii]["Port"] = atoi(sd[3].c_str());
					ii++;
				}
			}
		}

		void CWebServer::Cmd_KodiSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			std::string mode2 = request::findValue(&req, "mode2");
			if ((hwid.empty()) || (mode1.empty()) || (mode2.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = reinterpret_cast<CKodi*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "KodiSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}

		void CWebServer::Cmd_KodiAddNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			int Port = atoi(request::findValue(&req, "port").c_str());
			if ((hwid.empty()) || (name.empty()) || (ip.empty()) || (Port == 0))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = reinterpret_cast<CKodi*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "KodiAddNode";
			pHardware->AddNode(name, ip, Port);
		}

		void CWebServer::Cmd_KodiUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			int Port = atoi(request::findValue(&req, "port").c_str());
			if ((hwid.empty()) || (nodeid.empty()) || (name.empty()) || (ip.empty()) || (Port == 0))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = reinterpret_cast<CKodi*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Port);
		}

		void CWebServer::Cmd_KodiRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if ((hwid.empty()) || (nodeid.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = reinterpret_cast<CKodi*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_KodiClearNodes(WebEmSession & session, const request& req, Json::Value &root)
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
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = reinterpret_cast<CKodi*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "KodiClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_KodiMediaCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "KodiMediaCommand";

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
					case HTYPE_Kodi:
					{
						CKodi	Kodi(HwID);
						Kodi.SendCommand(idx, sAction);
						break;
					}
#ifdef ENABLE_PYTHON
					case HTYPE_PythonPlugin:
						Cmd_PluginCommand(session, req, root);
						break;
#endif
						// put other players here ...
					}
				}
			}
		}

	} // namespace server
} // namespace http
