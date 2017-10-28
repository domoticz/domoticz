#include "stdafx.h"
#include "PanasonicTV.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/EventSystem.h"
#include "../hardware/hardwaretypes.h"
#include <iostream>

#define round(a) ( int ) ( a + .5 )
#define DEBUG_LOGGING (m_Port[0] == '-')

/*

Possible Commands:

"NRC_CH_DOWN-ONOFF"   => "Channel down",
"NRC_CH_UP-ONOFF"     => "Channel up",
"NRC_VOLUP-ONOFF"     => "Volume up",
"NRC_VOLDOWN-ONOFF"   => "Volume down",
"NRC_MUTE-ONOFF"      => "Mute",
"NRC_TV-ONOFF"        => "TV",
"NRC_CHG_INPUT-ONOFF" => "AV",
"NRC_RED-ONOFF"       => "Red",
"NRC_GREEN-ONOFF"     => "Green",
"NRC_YELLOW-ONOFF"    => "Yellow",
"NRC_BLUE-ONOFF"      => "Blue",
"NRC_VTOOLS-ONOFF"    => "VIERA tools",
"NRC_CANCEL-ONOFF"    => "Cancel / Exit",
"NRC_SUBMENU-ONOFF"   => "Option",
"NRC_RETURN-ONOFF"    => "Return",
"NRC_ENTER-ONOFF"     => "Control Center click / enter",
"NRC_RIGHT-ONOFF"     => "Control RIGHT",
"NRC_LEFT-ONOFF"      => "Control LEFT",
"NRC_UP-ONOFF"        => "Control UP",
"NRC_DOWN-ONOFF"      => "Control DOWN",
"NRC_3D-ONOFF"        => "3D button",
"NRC_SD_CARD-ONOFF"   => "SD-card",
"NRC_DISP_MODE-ONOFF" => "Display mode / Aspect ratio",
"NRC_MENU-ONOFF"      => "Menu",
"NRC_INTERNET-ONOFF"  => "VIERA connect",
"NRC_VIERA_LINK-ONOFF"=> "VIERA link",
"NRC_EPG-ONOFF"       => "Guide / EPG",
"NRC_TEXT-ONOFF"      => "Text / TTV",
"NRC_STTL-ONOFF"      => "STTL / Subtitles",
"NRC_INFO-ONOFF"      => "Info",
"NRC_INDEX-ONOFF"     => "TTV index",
"NRC_HOLD-ONOFF"      => "TTV hold / image freeze",
"NRC_R_TUNE-ONOFF"    => "Last view",
"NRC_POWER-ONOFF"     => "Power off",
"NRC_REW-ONOFF"       => "Rewind",
"NRC_PLAY-ONOFF"      => "Play",
"NRC_FF-ONOFF"        => "Fast forward",
"NRC_SKIP_PREV-ONOFF" => "Skip previous",
"NRC_PAUSE-ONOFF"     => "Pause",
"NRC_SKIP_NEXT-ONOFF" => "Skip next",
"NRC_STOP-ONOFF"      => "Stop",
"NRC_REC-ONOFF"       => "Record",
"NRC_D1-ONOFF"        => "Digit 1",
"NRC_D2-ONOFF"        => "Digit 2",
"NRC_D3-ONOFF"        => "Digit 3",
"NRC_D4-ONOFF"        => "Digit 4",
"NRC_D5-ONOFF"        => "Digit 5",
"NRC_D6-ONOFF"        => "Digit 6",
"NRC_D7-ONOFF"        => "Digit 7",
"NRC_D8-ONOFF"        => "Digit 8",
"NRC_D9-ONOFF"        => "Digit 9",
"NRC_D0-ONOFF"        => "Digit 0",
"NRC_P_NR-ONOFF"      => "P-NR (Noise reduction)",
"NRC_R_TUNE-ONOFF"    => "Seems to do the same as INFO",
"NRC_HDMI1"           => "Switch to HDMI input 1",
"NRC_HDMI2"           => "Switch to HDMI input 2",
"NRC_HDMI3"           => "Switch to HDMI input 3",
"NRC_HDMI4"           => "Switch to HDMI input 4",


*/

class CPanasonicNode //: public boost::enable_shared_from_this<CPanasonicNode>
{
	class CPanasonicStatus
	{
	public:
		CPanasonicStatus() { Clear(); };
		_eMediaStatus	Status() { return m_nStatus; };
		_eNotificationTypes	NotificationType();
		std::string		StatusText() { return Media_Player_States(m_nStatus); };
		void			Status(_eMediaStatus pStatus) { m_nStatus = pStatus; };
		void			Status(std::string pStatus) { m_sStatus = pStatus; };
		void			LastOK(time_t pLastOK) { m_tLastOK = pLastOK; };
		std::string		LastOK() { std::string sRetVal;  tm ltime; localtime_r(&m_tLastOK, &ltime); char szLastUpdate[40]; sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec); sRetVal = szLastUpdate; return sRetVal; };
		void			Clear();
		std::string		LogMessage();
		std::string		StatusMessage();
		bool			LogRequired(CPanasonicStatus&);
		bool			UpdateRequired(CPanasonicStatus&);
		bool			OnOffRequired(CPanasonicStatus&);
		bool			IsOn() { return (m_nStatus != MSTAT_OFF); };
		void			Volume(int pVolume) { m_VolumeLevel = pVolume;};
		void			Muted(bool pMuted) { m_Muted = pMuted; };
	private:
		_eMediaStatus	m_nStatus;
		std::string		m_sStatus;
		time_t			m_tLastOK;
		bool			m_Muted;
		int				m_VolumeLevel;
	};

public:
	CPanasonicNode(const int, const int, const int, const std::string&, const std::string&, const std::string&, const std::string&);
	~CPanasonicNode(void);
	void			Do_Work();
	void			SendCommand(const std::string &command);
	void			SendCommand(const std::string &command, const int iValue);
	void			SetExecuteCommand(const std::string &command);
	bool			SendShutdown();
	void			StopThread();
	bool			StartThread();
	bool			IsBusy() { return m_Busy; };
	bool			IsOn() { return (m_CurrentStatus.Status() == MSTAT_ON); };

	int				m_ID;
	int				m_DevID;
	std::string		m_Name;

	bool			m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
protected:
	bool			m_Busy;
	bool			m_Stoppable;

private:
	bool			handleConnect(boost::asio::ip::tcp::socket&, boost::asio::ip::tcp::endpoint, boost::system::error_code&);
	std::string		handleWriteAndRead(std::string);
	int				handleMessage(std::string);
	std::string		buildXMLStringRendCtl(std::string, std::string);
	std::string		buildXMLStringRendCtl(std::string, std::string, std::string);
	std::string		buildXMLStringNetCtl(std::string);
	
	int				m_HwdID;
	char			m_szDevID[40];
	std::string		m_IP;
	std::string		m_Port;
	bool			m_PowerOnSupported;

	CPanasonicStatus		m_PreviousStatus;
	CPanasonicStatus		m_CurrentStatus;
	//void			UpdateStatus();
	void			UpdateStatus(bool force = false);

	std::string		m_ExecuteCommand;

	std::string		m_RetainedData;

	int				m_iTimeoutCnt;
	int				m_iPollIntSec;
	int				m_iMissedPongs;
	std::string		m_sLastMessage;
	inline bool isInteger(const std::string & s)
	{
		if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

		char * p;
		strtol(s.c_str(), &p, 10);

		return (*p == 0);
	}
};


void CPanasonicNode::CPanasonicStatus::Clear()
{
	m_nStatus = MSTAT_UNKNOWN;
	m_sStatus = "";
	m_tLastOK = mytime(NULL);
	m_VolumeLevel = -1;
	m_Muted = false;
}

void CPanasonicNode::StopThread()
{
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
}

bool CPanasonicNode::StartThread()
{
	StopThread();
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPanasonicNode::Do_Work, this)));
	return (m_thread != NULL);
}

std::string	CPanasonicNode::CPanasonicStatus::LogMessage()
{
	std::string	sLogText;
	if (m_nStatus == MSTAT_OFF)
		return sLogText;
	if (m_VolumeLevel != -1)
		sLogText = "Volume: " + boost::to_string(m_VolumeLevel) + (m_Muted ? " - Muted" : "");
	return sLogText;
}

std::string	CPanasonicNode::CPanasonicStatus::StatusMessage()
{
	return LogMessage();
}

bool CPanasonicNode::CPanasonicStatus::LogRequired(CPanasonicStatus& pPrevious)
{
	return ((LogMessage() != pPrevious.LogMessage()) || (m_nStatus != pPrevious.Status()));
}

bool CPanasonicNode::CPanasonicStatus::UpdateRequired(CPanasonicStatus& pPrevious)
{
	return ((StatusMessage() != pPrevious.StatusMessage()) || (m_nStatus != pPrevious.Status()));
}

bool CPanasonicNode::CPanasonicStatus::OnOffRequired(CPanasonicStatus& pPrevious)
{
	return ((m_nStatus == MSTAT_OFF) || (pPrevious.Status() == MSTAT_OFF)) && (m_nStatus != pPrevious.Status());
}

_eNotificationTypes	CPanasonicNode::CPanasonicStatus::NotificationType()
{
	switch (m_nStatus)
	{
	case MSTAT_OFF:		return NTYPE_SWITCH_OFF;
	case MSTAT_ON:		return NTYPE_SWITCH_ON;
	default:			return NTYPE_SWITCH_OFF;
	}
}

CPanasonicNode::CPanasonicNode(const int pHwdID, const int PollIntervalsec, const int pTimeoutMs,
	const std::string& pID, const std::string& pName, const std::string& pIP, const std::string& pPort)
{
	m_stoprequested = false;
	m_Busy = false;
	m_Stoppable = false;
	m_PowerOnSupported = false;

	m_HwdID = pHwdID;
	m_DevID = atoi(pID.c_str());
	sprintf(m_szDevID, "%X%02X%02X%02X", 0, 0, (m_DevID & 0xFF00) >> 8, m_DevID & 0xFF);
	m_Name = pName;
	m_IP = pIP;
	m_Port = pPort;
	m_iTimeoutCnt = (pTimeoutMs > 999) ? pTimeoutMs / 1000 : pTimeoutMs;
	m_iPollIntSec = PollIntervalsec;
	m_iMissedPongs = 0;

	if (DEBUG_LOGGING) _log.Log(LOG_STATUS, "Panasonic Plugin: (%s) Created.", m_Name.c_str());

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

CPanasonicNode::~CPanasonicNode(void)
{
	StopThread();
	if (DEBUG_LOGGING) _log.Log(LOG_STATUS, "Panasonic Plugin: (%s) Destroyed.", m_Name.c_str());
}

void CPanasonicNode::UpdateStatus(bool forceupdate)
{
	std::vector<std::vector<std::string> > result;
	m_CurrentStatus.LastOK(mytime(NULL));

	// 1:	Update the DeviceStatus
	
	if (m_CurrentStatus.UpdateRequired(m_PreviousStatus) || forceupdate)
	{
		result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d, sValue='%q', LastUpdate='%q' WHERE (HardwareID == %d) AND (DeviceID == '%q') AND (Unit == 1) AND (SwitchType == %d)",
			int(m_CurrentStatus.Status()), m_CurrentStatus.StatusMessage().c_str(), m_CurrentStatus.LastOK().c_str(), m_HwdID, m_szDevID, STYPE_Media);
	}

	// 2:	Log the event if the actual status has changed
	std::string	sLogText = m_CurrentStatus.StatusText();

	if (m_CurrentStatus.LogRequired(m_PreviousStatus) || forceupdate)
	{
		if (m_CurrentStatus.IsOn()) sLogText += " - " + m_CurrentStatus.LogMessage();
		result = m_sql.safe_query("INSERT INTO LightingLog (DeviceRowID, nValue, sValue) VALUES (%d, %d, '%q')", m_ID, int(m_CurrentStatus.Status()), sLogText.c_str());
		_log.Log(LOG_NORM, "Panasonic: (%s) Event: '%s'.", m_Name.c_str(), sLogText.c_str());
	}

	// 3:	Trigger On/Off actions
	if (m_CurrentStatus.OnOffRequired(m_PreviousStatus) || forceupdate)
	{
		result = m_sql.safe_query("SELECT StrParam1,StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (ID = '%q') AND (Unit == 1)", m_HwdID, m_szDevID);
		if (result.size() > 0)
		{
			m_sql.HandleOnOffAction(m_CurrentStatus.IsOn(), result[0][0], result[0][1]);
		}
	}

	// 4:	Trigger Notifications & events on status change
	
	if (m_CurrentStatus.Status() != m_PreviousStatus.Status() || forceupdate)
	{
		m_notifications.CheckAndHandleNotification(m_ID, m_Name, m_CurrentStatus.NotificationType(), sLogText);
		m_mainworker.m_eventsystem.ProcessDevice(m_HwdID, m_ID, 1, int(pTypeLighting2), int(sTypeAC), 12, 100, int(m_CurrentStatus.Status()), m_CurrentStatus.StatusMessage().c_str(), m_Name.c_str(), 0);
	}

	m_PreviousStatus = m_CurrentStatus;
}

bool CPanasonicNode::handleConnect(boost::asio::ip::tcp::socket& socket, boost::asio::ip::tcp::endpoint endpoint, boost::system::error_code& ec)
{
	try
	{
		if (!m_stoprequested)
		{
			socket.connect(endpoint, ec);
			if (!ec)
			{
				if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Connected to '%s:%s'.", m_Name.c_str(), m_IP.c_str(), (m_Port[0] != '-' ? m_Port.c_str() : m_Port.substr(1).c_str()));
				return true;
			}
			else
			{
				if (DEBUG_LOGGING)
					if ((
						(ec.value() != 113) &&
						(ec.value() != 111) &&
						(ec.value() != 10060) &&
						(ec.value() != 10061) &&
						(ec.value() != 10064) //&&
						//(ec.value() != 10061)
						)
						) // Connection failed due to no response, no route or active refusal
					{
						_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Connect to '%s:%s' failed: (%d) %s", m_Name.c_str(), m_IP.c_str(), (m_Port[0] != '-' ? m_Port.c_str() : m_Port.substr(1).c_str()), ec.value(), ec.message().c_str());
					}
				return false;
			}
		}
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Exception: '%s' connecting to '%s'", m_Name.c_str(), e.what(), m_IP.c_str());
		return false;
	}

	return true;

}


std::string CPanasonicNode::handleWriteAndRead(std::string pMessageToSend)
{

	if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Handling message: '%s'.", m_Name.c_str(), pMessageToSend.c_str());
	boost::asio::io_service io_service;
	// Get a list of endpoints corresponding to the server name.
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(m_IP, (m_Port[0] != '-' ? m_Port : m_Port.substr(1)));
	boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
	boost::asio::ip::tcp::endpoint endpoint = *iter;
	boost::asio::ip::tcp::resolver::iterator end;

	// Try each endpoint until we successfully establish a connection.
	boost::asio::ip::tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && iter != end)
	{
		socket.close();
		if (handleConnect(socket, *iter, error)) 
		{
			if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Connected.", m_Name.c_str());
			break;
		}
		else
			iter++;
	}
	if (error)
	{
		if (DEBUG_LOGGING) _log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Error trying to connect.", m_Name.c_str());
		socket.close();
		return "ERROR";
	}

	boost::array<char, 512> _Buffer;
	size_t request_length = std::strlen(pMessageToSend.c_str());
	if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Attemping write.", m_Name.c_str());
	
	try
	{
		boost::asio::write(socket, boost::asio::buffer(pMessageToSend.c_str(), request_length));
		if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Attemping read.", m_Name.c_str());
		size_t reply_length = boost::asio::read(socket, boost::asio::buffer(_Buffer, request_length));
		//_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Error code: (%i).'.", m_Name.c_str(),error);
		socket.close();
		std::string pReceived(_Buffer.begin(), reply_length);
		return pReceived;
	}
	catch (...)
	{
		//_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Exception in Write/Read message: %s", m_Name.c_str(), e.what());
		socket.close();
		std::string error = "ERROR";
		return error;
	}
}

int CPanasonicNode::handleMessage(std::string pMessage)
{
	int iPosBegin = 0;
	int iPosEnd = 0;
	std::string begin(">");
	std::string end("<");
	std::string ResponseOK("HTTP/1.1 200");
	std::string ResponseOff("HTTP/1.1 400");

	if (pMessage == "ERROR" || pMessage == "")
	{
		_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) handleMessage passed error or empty string!", m_Name.c_str());
		return -1;
	}

	// Look for a 200 response as this indicates that the TV was ok with last command.
	iPosBegin = pMessage.find(ResponseOK);
	if (iPosBegin != std::string::npos)
	{
		if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Last command response OK", m_Name.c_str());
	}

	iPosBegin = 0;

	// Look for a 400 response as this indicates that the TV supports powering on.
	iPosBegin = pMessage.find(ResponseOff);
	if (iPosBegin != std::string::npos)
	{
		if (!m_PowerOnSupported)
		{
			if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) TV Supports Powering on by Network", m_Name.c_str());
			m_PowerOnSupported = true;
		}
	}

	// Reset for next Search
	iPosBegin = 0;
	iPosEnd = 0;

	while (1)
	{
		iPosBegin = pMessage.find(begin, iPosBegin);
		if (iPosBegin == std::string::npos)
			break;
		iPosEnd = pMessage.find(end, iPosBegin+1);
		if (iPosEnd != std::string::npos)
		{
			std::string sFound = pMessage.substr(iPosBegin + 1, ((iPosEnd - iPosBegin) - 1));
			if (is_number(sFound))
			{
				return atoi(sFound.c_str());
			}
		}
		iPosBegin++;
	}

	// If we got here we didn't find a return string
	return -1;
}

std::string CPanasonicNode::buildXMLStringRendCtl(std::string action, std::string command)
{
	return buildXMLStringRendCtl(action, command, "");
}

std::string CPanasonicNode::buildXMLStringRendCtl(std::string action, std::string command, std::string value)
{
	std::string head, body;
	int size;

	body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n";
	body += "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\r\n";
	body += "<s:Body>\r\n";
	body += "<u:" + action + command + " xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">\r\n";
	body += "<InstanceID>0</InstanceID>\r\n";
	body += "<Channel>Master</Channel>\r\n";
	body += "<Desired" + command + ">" + value + "</Desired" + command + ">\r\n"; // value is optional, only used for Set action
	body += "</u:" + action + command + ">\r\n";
	body += "</s:Body>\r\n";
	body += "</s:Envelope>\r\n";
	
	size = body.length();

	head = "POST /dmr/control_0 HTTP/1.1\r\n";
	head += "Host: " + m_IP + ":" + m_Port + "\r\n";
	head += "SOAPACTION: \"urn:schemas-upnp-org:service:RenderingControl:1#" + action + command + "\"\r\n";
	head += "Content-Type: text/xml; charset=\"utf-8\"\r\n";
	head += "Content-Length: " + boost::to_string(size) + "\r\n\r\n";

	return head + body;

}

std::string CPanasonicNode::buildXMLStringNetCtl(std::string command)
{
	std::string head, body;
	int size;

	body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	body += "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">";
	body += "<s:Body>";
	body += "<u:X_SendKey xmlns:u=\"urn:panasonic-com:service:p00NetworkControl:1\">";
	body += "<X_KeyEvent>NRC_" + command + "</X_KeyEvent>";
	body += "</u:X_SendKey>";
	body += "</s:Body>";
	body += "</s:Envelope>";

	size = body.length();

	head += "POST /nrc/control_0 HTTP/1.1\r\n";
	head += "Host: " + m_IP + ":" + m_Port + "\r\n";
	head += "SOAPACTION: \"urn:panasonic-com:service:p00NetworkControl:1#X_SendKey\"\r\n";
	head += "Content-Type: text/xml; charset=\"utf-8\"\r\n";
	head += "Content-Length: " + boost::to_string(size) + "\r\n";
	head += "\r\n";

	return head + body;

}


void CPanasonicNode::Do_Work()
{
	m_Busy = true;
	if (DEBUG_LOGGING) _log.Log(LOG_NORM, "Panasonic Plugin: (%s) Entering work loop.", m_Name.c_str());
	int	iPollCount = 9;

	while (!m_stoprequested)
	{
		sleep_milliseconds(500);

		iPollCount++;
		if (iPollCount >= 10)
		{
			iPollCount = 0;
			try
			{
				std::string _volReply = handleWriteAndRead(buildXMLStringRendCtl("Get", "Volume"));
				if (_volReply != "ERROR")
				{
					int iVol = handleMessage(_volReply);
					m_CurrentStatus.Volume(iVol);
					if (m_CurrentStatus.Status() != MSTAT_ON && iVol > -1)
					{
						m_CurrentStatus.Status(MSTAT_ON);
						UpdateStatus();
					}
				}
				else
				{
					if (m_CurrentStatus.Status() != MSTAT_OFF)
					{
						m_CurrentStatus.Clear();
						m_CurrentStatus.Status(MSTAT_OFF);
						UpdateStatus();
					}
				}

				//_std::string _muteReply = handleWriteAndRead(buildXMLStringRendCtl("Get", "Mute"));
				//_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Mute reply - \r\n", m_Name.c_str(), _muteReply.c_str());
				//if (_muteReply != "ERROR")
				//{
				//	m_CurrentStatus.Muted((handleMessage(_muteReply)==0) ? false : true);
				//}
				UpdateStatus();
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Exception: %s", m_Name.c_str(), e.what());
			}
		}
	}
	_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Exiting work loop.", m_Name.c_str());
	m_Busy = false;
}

void CPanasonicNode::SendCommand(const std::string &command)
{
	std::string	sPanasonicCall = "";
	
	if (m_CurrentStatus.Status() == MSTAT_OFF && !m_PowerOnSupported)
	{
		// no point trying to send a command if we know the device is off
		// if we get a 400 response when TV is off then Power toggle can be sent
		_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Device is Off, cannot send command.", m_Name.c_str());
		return;
	}

	if (command == "Home")
		sPanasonicCall = buildXMLStringNetCtl("CANCEL-ONOFF");
	else if (command == "Up")
		sPanasonicCall = buildXMLStringNetCtl("UP-ONOFF");
	else if (command == "Down")
		sPanasonicCall = buildXMLStringNetCtl("DOWN-ONOFF");
	else if (command == "Left")
		sPanasonicCall = buildXMLStringNetCtl("LEFT-ONOFF");
	else if (command == "Right")
		sPanasonicCall = buildXMLStringNetCtl("RIGHT-ONOFF");
	else if (command == "ChannelUp")
		sPanasonicCall = buildXMLStringNetCtl("CH_UP-ONOFF");
	else if (command == "ChannelDown")
		sPanasonicCall = buildXMLStringNetCtl("CH_DOWN-ONOFF");
	else if (command == "VolumeUp")
		sPanasonicCall = buildXMLStringNetCtl("VOLUP-ONOFF");
	else if (command == "VolumeDown")
		sPanasonicCall = buildXMLStringNetCtl("VOLDOWN-ONOFF");
	else if (command == "Mute")
		sPanasonicCall = buildXMLStringNetCtl("MUTE-ONOFF");
	else if (command == "inputtv")
		sPanasonicCall = buildXMLStringNetCtl("TV-ONOFF");
	else if (command == "inputav")
		sPanasonicCall = buildXMLStringNetCtl("CHG_INPUT-ONOFF");
	else if (command == "Red")
		sPanasonicCall = buildXMLStringNetCtl("RED-ONOFF");
	else if (command == "Green")
		sPanasonicCall = buildXMLStringNetCtl("GREEN-ONOFF");
	else if (command == "Blue")
		sPanasonicCall = buildXMLStringNetCtl("BLUE-ONOFF");
	else if (command == "Yellow")
		sPanasonicCall = buildXMLStringNetCtl("YELLOW-ONOFF");
	else if (command == "ContextMenu")
		sPanasonicCall = buildXMLStringNetCtl("MENU-ONOFF");
	else if (command == "Channels" || command == "guide")
		sPanasonicCall = buildXMLStringNetCtl("EPG-ONOFF");
	else if (command == "Back" || command == "Return")
		sPanasonicCall = buildXMLStringNetCtl("RETURN-ONOFF");
	else if (command == "Select")
		sPanasonicCall = buildXMLStringNetCtl("ENTER-ONOFF");
	else if (command == "Info")
		sPanasonicCall = buildXMLStringNetCtl("INFO-ONOFF");
	else if (command == "1")
		sPanasonicCall = buildXMLStringNetCtl("D1-ONOFF");
	else if (command == "2")
		sPanasonicCall = buildXMLStringNetCtl("D2-ONOFF");
	else if (command == "3")
		sPanasonicCall = buildXMLStringNetCtl("D3-ONOFF");
	else if (command == "4")
		sPanasonicCall = buildXMLStringNetCtl("D4-ONOFF");
	else if (command == "5")
		sPanasonicCall = buildXMLStringNetCtl("D5-ONOFF");
	else if (command == "6")
		sPanasonicCall = buildXMLStringNetCtl("D6-ONOFF");
	else if (command == "7")
		sPanasonicCall = buildXMLStringNetCtl("D7-ONOFF");
	else if (command == "8")
		sPanasonicCall = buildXMLStringNetCtl("D8-ONOFF");
	else if (command == "9")
		sPanasonicCall = buildXMLStringNetCtl("D9-ONOFF");
	else if (command == "0")
		sPanasonicCall = buildXMLStringNetCtl("D0-ONOFF");
	else if (command == "hdmi1")
		sPanasonicCall = buildXMLStringNetCtl("HDMI1");
	else if (command == "hdmi2")
		sPanasonicCall = buildXMLStringNetCtl("HDMI2");
	else if (command == "hdmi3")
		sPanasonicCall = buildXMLStringNetCtl("HDMI3");
	else if (command == "hdmi4")
		sPanasonicCall = buildXMLStringNetCtl("HDMI4");
	else if (command == "Rewind")
		sPanasonicCall = buildXMLStringNetCtl("REW-ONOFF");
	else if (command == "FastForward")
		sPanasonicCall = buildXMLStringNetCtl("FF-ONOFF");
	else if (command == "PlayPause")
		sPanasonicCall = buildXMLStringNetCtl("PLAY-ONOFF");
	else if (command == "Stop")
		sPanasonicCall = buildXMLStringNetCtl("STOP-ONOFF");
	else if (command == "pause")
		sPanasonicCall = buildXMLStringNetCtl("PAUSE-ONOFF");
	else if (command == "Power" || command == "power" || command == "off" || command == "Off")
		sPanasonicCall = buildXMLStringNetCtl("POWER-ONOFF");
	else if (command == "ShowSubtitles")
		sPanasonicCall = buildXMLStringNetCtl("STTL-ONOFF");
	else if (command == "On" || command == "on")
	{
		if (m_PowerOnSupported)
			sPanasonicCall = buildXMLStringNetCtl("POWER-ONOFF");
		else
		{
			_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Can't use command: '%s'.", m_Name.c_str(), command.c_str());
			// Workaround to get the plugin to reset device status, otherwise the UI goes out of sync with plugin
			m_CurrentStatus.Clear();
			m_CurrentStatus.Status(MSTAT_UNKNOWN);
			UpdateStatus(true);
		}
	}
	else
		_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Unknown command: '%s'.", m_Name.c_str(), command.c_str());

	if (sPanasonicCall.length())
	{
		if (handleWriteAndRead(sPanasonicCall) != "ERROR")
			_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Sent command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
		//else
		//	_log.Log(LOG_NORM, "Panasonic Plugin: (%s) can't send command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
	}
	
}

void CPanasonicNode::SendCommand(const std::string &command, const int iValue)
{
	std::string	sPanasonicCall;
	if (command == "setvolume")
		sPanasonicCall = buildXMLStringRendCtl("Set", "Volume", boost::to_string(iValue));
	else
		_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) Command: '%s'. Unknown command.", m_Name.c_str(), command.c_str());
	
	if (sPanasonicCall.length())
	{
		if (handleWriteAndRead(sPanasonicCall) != "ERROR")
			_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Sent command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
		else
			_log.Log(LOG_NORM, "Panasonic Plugin: (%s) can't send command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
	}

}

bool CPanasonicNode::SendShutdown()
{
	std::string sPanasonicCall = buildXMLStringNetCtl("POWER-ONOFF");
	if (handleWriteAndRead(sPanasonicCall) != "ERROR") {
		_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Sent command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
		return true;
	}
	else
		_log.Log(LOG_NORM, "Panasonic Plugin: (%s) can't send command: '%s'.", m_Name.c_str(), sPanasonicCall.c_str());
	return false;
}

void CPanasonicNode::SetExecuteCommand(const std::string &command)
{
	_log.Log(LOG_ERROR, "Panasonic Plugin: (%s) SetExecuteCommand called with: '%s.", m_Name.c_str(), command.c_str());
}

std::vector<boost::shared_ptr<CPanasonicNode> > CPanasonic::m_pNodes;

CPanasonic::CPanasonic(const int ID, const int PollIntervalsec, const int PingTimeoutms)
{
	m_HwdID = ID;
	m_stoprequested = false;
	SetSettings(PollIntervalsec, PingTimeoutms);
}

CPanasonic::CPanasonic(const int ID)
{
	m_HwdID = ID;
	m_stoprequested = false;
	SetSettings(10, 3000);
}

CPanasonic::~CPanasonic(void)
{
	m_bIsStarted = false;
}

bool CPanasonic::StartHardware()
{
	StopHardware();
	m_bIsStarted = true;
	sOnConnected(this);

	StartHeartbeatThread();

	//Start worker thread
	m_stoprequested = false;
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPanasonic::Do_Work, this)));
	_log.Log(LOG_STATUS, "Panasonic Plugin: Started");

	return true;
}

bool CPanasonic::StopHardware()
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

void CPanasonic::Do_Work()
{
	int scounter = 0;

	ReloadNodes();

	while (!m_stoprequested)
	{
		if (scounter++ >= (m_iPollInterval * 2))
		{
			boost::lock_guard<boost::mutex> l(m_mutex);

			scounter = 0;
			bool bWorkToDo = false;
			std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt;
			for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
			{
				if (!(*itt)->IsBusy())
				{
					_log.Log(LOG_NORM, "Panasonic Plugin: (%s) - Restarting thread.", (*itt)->m_Name.c_str());
					(*itt)->StartThread();
				}
				if ((*itt)->IsOn()) bWorkToDo = true;
			}
		}
		sleep_milliseconds(500);
	}
	UnloadNodes();
	_log.Log(LOG_STATUS, "Panasonic Plugin: Worker stopped...");
}

void CPanasonic::SetSettings(const int PollIntervalsec, const int PingTimeoutms)
{
	//Defaults
	m_iPollInterval = 30;
	m_iPingTimeoutms = 1000;

	if (PollIntervalsec > 1)
		m_iPollInterval = PollIntervalsec;
	if ((PingTimeoutms / 1000 < m_iPollInterval) && (PingTimeoutms != 0))
		m_iPingTimeoutms = PingTimeoutms;
}

void CPanasonic::Restart()
{
	StopHardware();
	StartHardware();
}

bool CPanasonic::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;

	if (packettype != pTypeLighting2)
		return false;

	long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;

	std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_DevID == DevID)
		{
			int iParam = pSen->LIGHTING2.level;
			switch (pSen->LIGHTING2.cmnd)
			{
			case light2_sOn:
				(*itt)->SendCommand("On");
				return true;
			case light2_sOff:
				(*itt)->SendCommand("Off");
				return true;
			case light2_sGroupOff:
				return (*itt)->SendShutdown();
			case gswitch_sStop:
				(*itt)->SendCommand("Stop");
				return true;
			case gswitch_sPlay:
				(*itt)->SendCommand("PlayPause");
				return true;
			case gswitch_sPause:
				(*itt)->SendCommand("PlayPause");
				return true;
			case gswitch_sSetVolume:
				(*itt)->SendCommand("setvolume", iParam);
				return true;
			//case gswitch_sPlayPlaylist:
			//	(*itt)->SendCommand("playlist", iParam);
			//	return true;
			//case gswitch_sExecute:
			//	(*itt)->SendCommand("execute", iParam);
			//	return true;
			default:
				return true;
			}
		}
	}

	_log.Log(LOG_ERROR, "Panasonic Plugin: (%d) Shutdown. Device not found.", DevID);
	return false;
}

void CPanasonic::AddNode(const std::string &Name, const std::string &IPAddress, const int Port)
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

bool CPanasonic::UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port)
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

void CPanasonic::RemoveNode(const int ID)
{
	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)", m_HwdID, ID);

	//Also delete the switch
	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, szID);
	ReloadNodes();
}

void CPanasonic::RemoveAllNodes()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);

	//Also delete the all switches
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	ReloadNodes();
}

void CPanasonic::ReloadNodes()
{
	UnloadNodes();

	//m_ios.reset();	// in case this is not the first time in

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,Name,MacAddress,Timeout FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);
	if (result.size() > 0)
	{
		boost::lock_guard<boost::mutex> l(m_mutex);

		// create a vector to hold the nodes
		for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			boost::shared_ptr<CPanasonicNode>	pNode = (boost::shared_ptr<CPanasonicNode>) new CPanasonicNode(m_HwdID, m_iPollInterval, m_iPingTimeoutms, sd[0], sd[1], sd[2], sd[3]);
			m_pNodes.push_back(pNode);
		}
		// start the threads to control each Panasonic TV
		for (std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
		{
			_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Starting thread.", (*itt)->m_Name.c_str());
			(*itt)->StartThread();
		}
		sleep_milliseconds(100);
		//_log.Log(LOG_NORM, "Panasonic Plugin: Starting I/O service thread.");
		//boost::thread bt(boost::bind(&boost::asio::io_service::run, &m_ios));
	}
}

void CPanasonic::UnloadNodes()
{
	int iRetryCounter = 0;

	boost::lock_guard<boost::mutex> l(m_mutex);

	m_ios.stop();	// stop the service if it is running
	sleep_milliseconds(100);

	while (((!m_pNodes.empty()) || (!m_ios.stopped())) && (iRetryCounter < 15))
	{
		std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt;
		for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
		{
			(*itt)->StopThread();
			if (!(*itt)->IsBusy())
			{
				_log.Log(LOG_NORM, "Panasonic Plugin: (%s) Removing device.", (*itt)->m_Name.c_str());
				m_pNodes.erase(itt);
				break;
			}
		}
		iRetryCounter++;
		sleep_milliseconds(500);
	}
	m_pNodes.clear();
}

void CPanasonic::SendCommand(const int ID, const std::string &command)
{
	std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_ID == ID)
		{
			(*itt)->SendCommand(command);
			return;
		}
	}

	_log.Log(LOG_ERROR, "Panasonic Plugin: (%d) Command: '%s'. Device not found.", ID, command.c_str());
}


bool CPanasonic::SetExecuteCommand(const int ID, const std::string &command)
{
	std::vector<boost::shared_ptr<CPanasonicNode> >::iterator itt;
	for (itt = m_pNodes.begin(); itt != m_pNodes.end(); ++itt)
	{
		if ((*itt)->m_ID == ID)
		{
			(*itt)->SetExecuteCommand(command);
			return true;
		}
	}
	return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_PanasonicGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_PanasonicTV)
				return;

			root["status"] = "OK";
			root["title"] = "PanasonicGetNodes";

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

		
		void CWebServer::Cmd_PanasonicSetMode(WebEmSession & session, const request& req, Json::Value &root)
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
			if (pBaseHardware->HwdType != HTYPE_PanasonicTV)
				return;
			CPanasonic *pHardware = reinterpret_cast<CPanasonic*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PanasonicSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}


		void CWebServer::Cmd_PanasonicAddNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = request::findValue(&req, "name");
			std::string ip = request::findValue(&req, "ip");
			int Port = atoi(request::findValue(&req, "port").c_str());
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
			if (pBaseHardware->HwdType != HTYPE_PanasonicTV)
				return;
			CPanasonic *pHardware = reinterpret_cast<CPanasonic*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PanasonicAddNode";
			pHardware->AddNode(name, ip, Port);
		}

		void CWebServer::Cmd_PanasonicUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = request::findValue(&req, "name");
			std::string ip = request::findValue(&req, "ip");
			int Port = atoi(request::findValue(&req, "port").c_str());
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
			if (pBaseHardware->HwdType != HTYPE_PanasonicTV)
				return;
			CPanasonic *pHardware = reinterpret_cast<CPanasonic*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PanasonicUpdateNode";
			pHardware->UpdateNode(NodeID, name, ip, Port);
		}

		void CWebServer::Cmd_PanasonicRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_PanasonicTV)
				return;
			CPanasonic *pHardware = reinterpret_cast<CPanasonic*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "PanasonicRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_PanasonicClearNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_PanasonicTV)
				return;
			CPanasonic *pHardware = reinterpret_cast<CPanasonic*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "PanasonicClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_PanasonicMediaCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			int idx = atoi(sIdx.c_str());
			root["status"] = "OK";
			root["title"] = "PanasonicMediaCommand";

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
					case HTYPE_PanasonicTV:
						CPanasonic	Panasonic(HwID);
						Panasonic.SendCommand(idx, sAction);
						break;
						// put other players here ...
					}
				}
			}
		}

	}
}
