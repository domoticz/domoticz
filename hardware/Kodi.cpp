#include "stdafx.h"
#include "Kodi.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"

#include <boost/algorithm/string.hpp>

#include <iostream>

#define round(a) ( int ) ( a + .5 )
#define MAX_TITLE_LEN 40
#define DEBUG_LOGGING false

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
	m_tLastOK = mytime(NULL);
}

std::string	CKodiNode::CKodiStatus::LogMessage()
{
	std::string	sLogText = "";
	if (m_sType != "")
	{
		if (m_sType == "episode")
		{
			if (m_sShowTitle.length()) sLogText = m_sShowTitle;
			if (m_iSeason) sLogText += " [S" + SSTR(m_iSeason) + "E" + SSTR(m_iEpisode) + "]";
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
			if ((m_sLabel != m_sTitle) && (m_sLabel.length())) sLogText += ", " + m_sLabel;
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
		int begin = sStatus.find_first_of("(",0);
		int end = sStatus.find_first_of(")", begin);
		if ((std::string::npos == begin) || (std::string::npos == end) || (begin >= end)) break;
		sStatus.erase(begin, end - begin + 1);
	}
	while (sStatus.length() > MAX_TITLE_LEN)
	{
		int end = sStatus.find_last_of(",");
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
	m_stoprequested = false;
	m_Busy = false;
	m_Stoppable = false;

	m_Ios = pIos;
	m_HwdID = pHwdID;
	m_DevID = atoi(pID.c_str());
	sprintf(m_szDevID, "%X%02X%02X%02X", 0, 0, (m_DevID & 0xFF00) >> 8, m_DevID & 0xFF);
	m_Name = pName;
	m_IP = pIP;
	int iPort = atoi(pPort.c_str());
	m_Port = (iPort > 0) ? iPort : 9090;
	m_iTimeoutSec = pTimeoutMs / 1000;
	m_iPollIntSec = PollIntervalsec;
	m_iMissedPongs = 0;

	m_Socket = NULL;

	if (DEBUG_LOGGING) _log.Log(LOG_STATUS, "Kodi: (%s) Created.", m_Name.c_str());

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

CKodiNode::~CKodiNode(void)
{
	handleDisconnect();
	if (DEBUG_LOGGING) _log.Log(LOG_STATUS, "Kodi: (%s) Destroyed.", m_Name.c_str());
}

void CKodiNode::handleMessage(std::string& pMessage)
{
	try
	{
		Json::Reader jReader;
		Json::Value root;
		std::string	sMessage;

		bool	bRetVal = jReader.parse(pMessage, root);
		if (!bRetVal)
		{
			_log.Log(LOG_ERROR, "Kodi: (%s) PARSE ERROR: '%s'", m_Name.c_str(), pMessage.c_str());
		}
		else if (root["error"].empty() != true)
		{
			/* e.g {"error":{"code":-32100,"message":"Failed to execute method."},"id":1,"jsonrpc":"2.0"}	*/
			_log.Log(LOG_ERROR, "Kodi: (%s) Code %d Text '%s' ID '%d' Request '%s'", m_Name.c_str(), root["error"]["code"].asInt(), root["error"]["message"].asCString(), root["id"].asInt(), m_sLastMessage.c_str());
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
							if ((root["method"] == "Player.OnStop") || (root["method"] == "Player.OnWake"))
							{
								m_CurrentStatus.Clear();
								m_CurrentStatus.Status(MSTAT_ON);
								UpdateStatus();
							}
							else if (root["method"] == "Player.OnPlay")
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
								else
								{
									_log.Log(LOG_ERROR, "Kodi: (%s) Message error, unknown type in OnPlay message: '%s' from '%s'", m_Name.c_str(), root["params"]["data"]["item"]["type"].asCString(), pMessage.c_str());
								}

								if (m_CurrentStatus.PlayerID() != "")  // if we now have a player id then request more details
								{
									sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetItem\",\"id\":3,\"params\":{\"playerid\":" + m_CurrentStatus.PlayerID() + ",\"properties\":[\"artist\",\"album\",\"year\",\"channel\",\"showtitle\",\"season\",\"episode\",\"title\"]}}";
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
								if (m_CurrentStatus.PlayerID() != "")
									sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetProperties\",\"id\":2,\"params\":{\"playerid\":" + m_CurrentStatus.PlayerID() + ",\"properties\":[\"live\",\"percentage\",\"speed\"]}}";
								else
									sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\",\"id\":5}";
								handleWrite(sMessage);
							}
							else if ((root["method"] == "Player.OnQuit") || (root["method"] == "Player.OnSleep"))
							{
								m_CurrentStatus.Clear();
								m_CurrentStatus.Status(MSTAT_OFF);
								UpdateStatus();
							}
							else if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Message warning, unhandled method: '%s'", m_Name.c_str(), root["method"].asCString());
						}
						else _log.Log(LOG_ERROR, "Kodi: (%s) Message error, params but no method: '%s'", m_Name.c_str(), pMessage.c_str());
					}
					else _log.Log(LOG_ERROR, "Kodi: (%s) Message error, invalid sender: '%s'", m_Name.c_str(), root["params"]["sender"].asCString());
				}
				else _log.Log(LOG_ERROR, "Kodi: (%s) Message error, params but no sender: '%s'", m_Name.c_str(), pMessage.c_str());
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
					case 1:		//Ping response
						if (root["result"] == "pong")
						{
							m_iMissedPongs = 0;
							m_CurrentStatus.Clear();
							m_CurrentStatus.Status(MSTAT_ON);
							UpdateStatus();
						}
						break;
					case 2:		//Poll response
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
						}
						UpdateStatus();
						break;
					case 3:		//OnPlay media details response
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
								UpdateStatus();
							}
							if (root["result"]["item"].isMember("title"))			m_CurrentStatus.Title(root["result"]["item"]["title"].asCString());
							if (root["result"]["item"].isMember("year"))			m_CurrentStatus.Year(root["result"]["item"]["year"].asInt());
							if (root["result"]["item"].isMember("label"))			m_CurrentStatus.Label(root["result"]["item"]["label"].asCString());
							if ((m_CurrentStatus.PlayerID() != "") && (m_CurrentStatus.Type() != "picture")) // request final details
							{
								std::string	sMessage;
								sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetProperties\",\"id\":2,\"params\":{\"playerid\":" + m_CurrentStatus.PlayerID() + ",\"properties\":[\"live\",\"percentage\",\"speed\"]}}";
								handleWrite(sMessage);
							}
						}
						break;
					case 4:		//Shutdown details response
						{
							std::string	sAction = "Nothing";
							if (root["result"].isMember("canshutdown"))
							{
								bCanShutdown = root["result"]["canshutdown"].asBool();
								if (bCanShutdown) sAction = "Shutdown";
							}
							if (root["result"].isMember("canhibernate"))
							{
								bCanHibernate = root["result"]["canhibernate"].asBool();
								if (bCanHibernate) sAction = "Hibernate";
							}
							if (root["result"].isMember("cansuspend"))
							{
								bCanSuspend = root["result"]["cansuspend"].asBool();
								if (bCanSuspend) sAction = "Suspend";
							}
							_log.Log(LOG_NORM, "Kodi: (%s) Switch Off: CanShutdown:%s, CanHibernate:%s, CanSuspend:%s. %s requested.", m_Name.c_str(),
								bCanShutdown ? "true" : "false", bCanHibernate ? "true" : "false", bCanSuspend ? "true" : "false", sAction.c_str());

							if (sAction != "Nothing")
							{
								std::string	sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"System." + sAction + "\",\"id\":5}";
								handleWrite(sMessage);
							}
						}
						break;
					case 5:		//GetPlayers response, normally requried when Domoticz starts up and media is already streaming
						if (root["result"][0].isMember("playerid"))
						{
							m_CurrentStatus.PlayerID(root["result"][0]["playerid"].asInt());
							sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetItem\",\"id\":3,\"params\":{\"playerid\":" + m_CurrentStatus.PlayerID() + ",\"properties\":[\"artist\",\"album\",\"year\",\"channel\",\"showtitle\",\"season\",\"episode\",\"title\"]}}";
							handleWrite(sMessage);
						}
						break;
					case 6:		//Remote Control response
						if (root["result"] != "OK")
							_log.Log(LOG_ERROR, "Kodi: (%s) Send Command Failed: '%s'", m_Name.c_str(), root["result"].asCString());
						break;
					case 7:		//Can Shutdown response (after connect)
						handleWrite(std::string("{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\",\"id\":5}"));
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
	std::vector<std::vector<std::string> > result;
	m_CurrentStatus.LastOK(mytime(NULL));

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
		result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue) VALUES (%d, %d, '%q')", m_ID, int(m_CurrentStatus.Status()), sLogText.c_str());
		_log.Log(LOG_NORM, "Kodi: (%s) Event: '%s'.", m_Name.c_str(), sLogText.c_str());
	}

	// 3:	Trigger On/Off actions
	if (m_CurrentStatus.OnOffRequired(m_PreviousStatus))
	{
		result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (ID = '%q') AND (Unit == 1)", m_HwdID, m_szDevID);
		if (result.size() > 0)
		{
			m_sql.HandleOnOffAction(m_CurrentStatus.IsOn(), result[0][0], result[0][1]);
		}
	}

	// 4:	Trigger Notifications on status change
	if (m_CurrentStatus.Status() != m_PreviousStatus.Status())
	{
		m_notifications.CheckAndHandleNotification(m_ID, m_Name, m_CurrentStatus.NotificationType(), sLogText);
	}

	m_PreviousStatus = m_CurrentStatus;
}

void CKodiNode::handleConnect()
{
	if (!m_stoprequested && !m_Socket)
	{
		m_iMissedPongs = 0;
		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_IP), m_Port);
		m_Socket = new boost::asio::ip::tcp::socket(*m_Ios);
		m_Socket->connect(endpoint, ec);
		if (!ec)
		{
			_log.Log(LOG_NORM, "Kodi: (%s) Connected.", m_Name.c_str());
			if (m_CurrentStatus.Status() == MSTAT_OFF)
			{
				m_CurrentStatus.Clear();
				m_CurrentStatus.Status(MSTAT_ON);
				UpdateStatus();
			}
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CKodiNode::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			handleWrite(std::string("{\"jsonrpc\":\"2.0\",\"method\":\"System.GetProperties\",\"params\":{\"properties\":[\"canhibernate\",\"cansuspend\",\"canshutdown\"]},\"id\":7}"));
		}
		else
		{
			if ((ec.value() != 113) && (ec.value() != 111) &&
				(ec.value() != 10060) && (ec.value() != 10061) && (ec.value() != 10064) && (ec.value() != 10061)) // Connection failed due to no response, no route or active refusal
				_log.Log(LOG_NORM, "Kodi: (%s) Connect failed: (%d) %s", m_Name.c_str(), ec.value(), ec.message().c_str());
			delete m_Socket;
			m_Socket = NULL;
			m_CurrentStatus.Clear();
			m_CurrentStatus.Status(MSTAT_OFF);
			UpdateStatus();
		}
	}
}

void CKodiNode::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if (!e)
	{
		//do something with the data
		std::string sData(m_Buffer.begin(), bytes_transferred);
		if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Parsing data: '%s'.", m_Name.c_str(), sData.c_str());
		int	iStart = 0;
		int iLength;
		do {
			iLength = sData.find("}{", iStart) + 1;		//  Look for message separater in case there is more than one
			if (!iLength) iLength = bytes_transferred;
			std::string sMessage = sData.substr(iStart, iLength);
			handleMessage(sMessage);
			iStart = iLength;
		} while (iLength != bytes_transferred);

		//ready for next read
		if (!m_stoprequested && m_Socket)
			m_Socket->async_read_some(	boost::asio::buffer(m_Buffer, sizeof m_Buffer),
										boost::bind(&CKodiNode::handleRead, 
										shared_from_this(),
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred));
	}
	else
	{
		if (e.value() != 1236)		// local disconnect cause by hardware reload
		{
			if (e.value() != 121)	// Semaphore tieout expiry aka 'lost contact'
				_log.Log(LOG_ERROR, "Kodi: (%s) Async Read Exception: %s", m_Name.c_str(), e.message().c_str());
			m_CurrentStatus.Clear();
			m_CurrentStatus.Status(MSTAT_OFF);
			UpdateStatus();
			handleDisconnect();
		}
	}
}

void CKodiNode::handleWrite(std::string pMessage)
{
	if (!m_stoprequested)
		if (m_Socket)
		{
			if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Sending data: '%s'", m_Name.c_str(), pMessage.c_str());
			m_Socket->write_some(boost::asio::buffer(pMessage.c_str(), pMessage.length()));
			m_sLastMessage = pMessage;
		}
		else _log.Log(LOG_ERROR, "Kodi: (%s) Data not sent to NULL socket: '%s'", m_Name.c_str(), pMessage.c_str());
}

void CKodiNode::handleDisconnect()
{
	if (m_Socket)
	{
		_log.Log(LOG_NORM, "Kodi: (%s) Disonnected.", m_Name.c_str());
		boost::system::error_code	ec;
		m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_Socket->close();
		delete m_Socket;
		m_Socket = NULL;
	}
}

void CKodiNode::Do_Work()
{
	m_Busy = true;
	if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Entering work loop.", m_Name.c_str());
	int	iPollCount = 2;

	try
	{
		while (!m_stoprequested)
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
					if (m_CurrentStatus.PlayerID() != "")
						sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetProperties\",\"id\":2,\"params\":{\"playerid\":" + m_CurrentStatus.PlayerID() + ",\"properties\":[\"live\",\"percentage\",\"speed\"]}}";
					else
						sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\",\"id\":5}";
				}
				else
				{
					sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"JSONRPC.Ping\",\"id\":1}";
					if (m_iMissedPongs++ > 3)
					{
						m_CurrentStatus.Clear();
						m_CurrentStatus.Status(MSTAT_OFF);
						UpdateStatus();
						handleDisconnect();
						continue;
					};
				}
				handleWrite(sMessage);
			}
			sleep_milliseconds(1000);
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "Kodi: (%s) Exception: %s", m_Name.c_str(), e.what());
	}
	_log.Log(LOG_NORM, "Kodi: (%s) Exiting work loop.", m_Name.c_str());
	m_Busy = false;
	delete m_Socket;
	m_Socket = NULL;
}

void CKodiNode::SendCommand(const std::string &command)
{
	std::string	sKodiCall;
	std::string	sKodiParam = "";
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
		//		{ "jsonrpc": "2.0", "method": "Input.ExecuteAction", "params": { "action": "stop" }, "id": 6 }
		std::string	sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"" + sKodiCall + "\",\"params\":{";
		if (sKodiParam.length()) sMessage += "\"action\":\"" + sKodiParam + "\"";
		sMessage += "},\"id\":6}";
		
		if (m_Socket != NULL)
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

bool CKodiNode::SendShutdown()
{
	//	http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22System.GetProperties%22,%22params%22:{%22properties%22:[%22canreboot%22,%22canhibernate%22,%22cansuspend%22,%22canshutdown%22]},%22id%22:1}
	//		{"id":1,"jsonrpc":"2.0","result":{"canhibernate":false,"canreboot":false,"canshutdown":false,"cansuspend":false}}

	std::string	sMessage = "{\"jsonrpc\":\"2.0\",\"method\":\"System.GetProperties\",\"params\":{\"properties\":[\"canhibernate\",\"cansuspend\",\"canshutdown\"]},\"id\":4}";
	handleWrite(sMessage);

	return m_Stoppable;
}

std::vector<boost::shared_ptr<CKodiNode> > CKodi::m_pNodes;

CKodi::CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms) : m_stoprequested(false)
{
	m_HwdID = ID;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CKodi::CKodi(const int ID) : m_stoprequested(false)
{
	m_HwdID = ID;
	SetSettings(10, 3000);
}

CKodi::~CKodi(void)
{
	m_bIsStarted = false;
}

bool CKodi::StartHardware()
{
	StopHardware();
	m_bIsStarted = true;
	sOnConnected(this);

	StartHeartbeatThread();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CKodi::Do_Work, this)));
	_log.Log(LOG_STATUS, "Kodi: Started");

	return true;
}

bool CKodi::StopHardware()
{
	StopHeartbeatThread();

	try {
		if (m_thread)
		{
			m_stoprequested = true;
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

void CKodi::Do_Work()
{
	int scounter = 0;

	ReloadNodes();

	while (!m_stoprequested)
	{
		if (scounter++ >= (m_iPollInterval*2))
		{
			boost::lock_guard<boost::mutex> l(m_mutex);

			scounter = 0;
			std::vector<boost::shared_ptr<CKodiNode> >::iterator itt;
			for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
			{
				if (!(*itt)->IsBusy())
				{
					_log.Log(LOG_NORM, "Kodi: (%s) - Restarting thread.", (*itt)->m_Name.c_str());
					boost::thread* tAsync = new boost::thread(&CKodiNode::Do_Work, (*itt));
					m_ios.stop();
				}
			}

			if (m_ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				m_ios.reset();
				// Note that this is the only thread that handles async i/o so we don't
				// need to worry about locking or concurrency issues when processing messages
				_log.Log(LOG_NORM, "Kodi: Restarting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &m_ios));
			}
		}
		sleep_milliseconds(500);
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

void CKodi::Restart()
{
	StopHardware();
	StartHardware();
}

bool CKodi::WriteToHardware(const char *pdata, const unsigned char length)
{
	//	http://<ip_address>:8080/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22method%22:%22System.GetProperties%22,%22params%22:{%22properties%22:[%22canreboot%22,%22canhibernate%22,%22cansuspend%22,%22canshutdown%22]},%22id%22:1}
	//		{"id":1,"jsonrpc":"2.0","result":{"canhibernate":false,"canreboot":false,"canshutdown":false,"cansuspend":false}}

	tRBUF *pSen = (tRBUF*)pdata;

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	if (pSen->LIGHTING2.cmnd != light2_sOff)
	{
		return true;
	}
	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;

	std::vector<boost::shared_ptr<CKodiNode> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_DevID == DevID)
		{
			return (*itt)->SendShutdown();
		}
	}

	_log.Log(LOG_ERROR, "Kodi: (%d) Shutdown. Device not found.", DevID);
	return false;
}

void CKodi::AddNode(const std::string &Name, const std::string &IPAddress, const int Port)
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

bool CKodi::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);
	if (result.size()<1)
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
	boost::lock_guard<boost::mutex> l(m_mutex);

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
	if (result.size() > 0)
	{
		boost::lock_guard<boost::mutex> l(m_mutex);

		// create a vector to hold the nodes
		for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			boost::shared_ptr<CKodiNode>	pNode = (boost::shared_ptr<CKodiNode>) new CKodiNode(&m_ios, m_HwdID, m_iPollInterval, m_iPingTimeoutms, sd[0], sd[1], sd[2], sd[3]);
			m_pNodes.push_back(pNode);
		}
		// start the threads to control each kodi
		for (std::vector<boost::shared_ptr<CKodiNode> >::iterator itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
		{
			if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Starting thread.", (*itt)->m_Name.c_str());
			boost::thread* tAsync = new boost::thread(&CKodiNode::Do_Work, (*itt));
		}
		sleep_milliseconds(100);
		if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: Starting I/O service thread.");
		boost::thread bt(boost::bind(&boost::asio::io_service::run, &m_ios));
	}
}

void CKodi::UnloadNodes()
{
	int iRetryCounter = 0;

	boost::lock_guard<boost::mutex> l(m_mutex);

	m_ios.stop();	// stop the service if it is running
	sleep_milliseconds(100);

	while ((!m_pNodes.empty()) || (!m_ios.stopped()) && (iRetryCounter < 15))
	{
		std::vector<boost::shared_ptr<CKodiNode> >::iterator itt;
		for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
		{
			(*itt)->StopRequest();
			if (!(*itt)->IsBusy())
			{
				if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Kodi: (%s) Removing device.", (*itt)->m_Name.c_str());
				m_pNodes.erase(itt);
				break;
			}
		}
		iRetryCounter++;
		sleep_milliseconds(500);
	}
	m_pNodes.clear();
}

void CKodi::SendCommand(const int ID, const std::string &command)
{
	std::vector<boost::shared_ptr<CKodiNode> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_ID == ID)
		{
			(*itt)->SendCommand(command);
			return;
		}
	}

	_log.Log(LOG_ERROR, "Kodi: (%d) Command: '%s'. Device not found.", ID, command.c_str());
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_KodiGetNodes(Json::Value &root)
		{
			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_Kodi)
				return;

			root["status"] = "OK";
			root["title"] = "KodiGetNodes";

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

		void CWebServer::Cmd_KodiSetMode(Json::Value &root)
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
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}

		void CWebServer::Cmd_KodiAddNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Port = atoi(m_pWebEm->FindValue("port").c_str());
			if (
				(hwid == "") ||
				(name == "") ||
				(ip == "") ||
				(Port == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiAddNode";
			pHardware->AddNode(name, ip, Port);
		}

		void CWebServer::Cmd_KodiUpdateNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			std::string name = m_pWebEm->FindValue("name");
			std::string ip = m_pWebEm->FindValue("ip");
			int Port = atoi(m_pWebEm->FindValue("port").c_str());
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(ip == "") ||
				(Port == 0)
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Port);
		}

		void CWebServer::Cmd_KodiRemoveNode(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			std::string nodeid = m_pWebEm->FindValue("nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "KodiRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_KodiClearNodes(Json::Value &root)
		{
			if (m_pWebEm->m_actualuser_rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string hwid = m_pWebEm->FindValue("idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_Kodi)
				return;
			CKodi *pHardware = (CKodi*)pBaseHardware;

			root["status"] = "OK";
			root["title"] = "KodiClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_KodiMediaCommand(Json::Value &root)
		{
			std::string sIdx = m_pWebEm->FindValue("idx");
			std::string sAction = m_pWebEm->FindValue("action");
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
						CKodi	Kodi(HwID);
						Kodi.SendCommand(idx, sAction);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}
