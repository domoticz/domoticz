#pragma once

#include "DomoticzHardware.h"

#include "../main/localtime_r.h"
#include <string>
#include <vector>
#include "../json/json.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

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

class CPanasonic : public CDomoticzHardwareBase
{
public:
	CPanasonic(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	explicit CPanasonic(const int ID);
	~CPanasonic(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
	void SendCommand(const int ID, const std::string &command);
	bool SetExecuteCommand(const int ID, const std::string &command);
private:
	void Do_Work();

	bool StartHardware();
	bool StopHardware();

	void ReloadNodes();
	void UnloadNodes();

	static	std::vector<boost::shared_ptr<CPanasonicNode> > m_pNodes;

	int m_iPollInterval;
	int m_iPingTimeoutms;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
	boost::asio::io_service m_ios;
};
