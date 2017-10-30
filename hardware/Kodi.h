#pragma once

#include "DomoticzHardware.h"

#include "../main/localtime_r.h"
#include <string>
#include <vector>
#include "../json/json.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

class CKodiNode : public boost::enable_shared_from_this<CKodiNode>
{
	class CKodiStatus
	{
	public:
		CKodiStatus() { Clear(); };
		_eMediaStatus	Status() { return m_nStatus; };
		_eNotificationTypes	NotificationType();
		std::string		StatusText() { return Media_Player_States(m_nStatus); };
		void			Status(_eMediaStatus pStatus) { m_nStatus = pStatus; };
		void			Status(const std::string &pStatus) { m_sStatus = pStatus; };
		void			PlayerID(int pPlayerID) { m_iPlayerID = pPlayerID; };
		std::string		PlayerID() { if (m_iPlayerID >= 0) return SSTR(m_iPlayerID); else return ""; };
		void			Type(const char*	pType) { m_sType = pType; };
		std::string		Type() { return m_sType; };
		void			Title(const char*	pTitle) { m_sTitle = pTitle; };
		void			ShowTitle(const char*	pShowTitle) { m_sShowTitle = pShowTitle; };
		void			Artist(const char*	pArtist) { m_sArtist = pArtist; };
		void			Album(const char*	pAlbum) { m_sAlbum = pAlbum; };
		void			Channel(const char*	pChannel) { m_sChannel = pChannel; };
		void			Season(int pSeason) { m_iSeason = pSeason; };
		void			Episode(int pEpisode) { m_iEpisode = pEpisode; };
		void			Label(const char*	pLabel) { m_sLabel = pLabel; };
		void			Percent(float	fPercent) { m_sPercent = ""; if (fPercent > 1.0) m_sPercent = SSTR((int)round(fPercent)) + "%"; };
		void			Year(int pYear) { m_sYear = SSTR(pYear); if (m_sYear.length() > 2) m_sYear = "(" + m_sYear + ")"; else m_sYear = ""; };
		void			Live(bool	pLive) { m_sLive = "";  if (pLive) m_sLive = "(Live)"; };
		void			LastOK(time_t pLastOK) { m_tLastOK = pLastOK; };
		std::string		LastOK() { std::string sRetVal;  tm ltime; localtime_r(&m_tLastOK, &ltime); char szLastUpdate[40]; sprintf(szLastUpdate, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec); sRetVal = szLastUpdate; return sRetVal; };
		void			Clear();
		std::string		LogMessage();
		std::string		StatusMessage();
		bool			LogRequired(CKodiStatus&);
		bool			UpdateRequired(CKodiStatus&);
		bool			OnOffRequired(CKodiStatus&);
		bool			IsOn() { return (m_nStatus != MSTAT_OFF); };
		bool			IsStreaming() { return ((m_nStatus == MSTAT_VIDEO) || (m_nStatus == MSTAT_AUDIO) || (m_nStatus == MSTAT_PAUSED) || (m_nStatus == MSTAT_PHOTO)); };
	private:
		_eMediaStatus	m_nStatus;
		std::string		m_sStatus;
		int				m_iPlayerID;
		std::string		m_sType;
		std::string		m_sShowTitle;
		std::string		m_sTitle;
		std::string		m_sArtist;
		std::string		m_sAlbum;
		std::string		m_sChannel;
		int				m_iSeason;
		int				m_iEpisode;
		std::string		m_sLabel;
		std::string		m_sPercent;
		std::string		m_sYear;
		std::string		m_sLive;
		time_t			m_tLastOK;
	};

public:
	CKodiNode(boost::asio::io_service*, const int, const int, const int, const std::string&, const std::string&, const std::string&, const std::string&);
	~CKodiNode(void);
	void			Do_Work();
	void			SendCommand(const std::string&);
	void			SendCommand(const std::string&, const int iValue);
	void			SetPlaylist(const std::string& playlist);
	void			SetExecuteCommand(const std::string& command);
	bool			SendShutdown();
	void			StopRequest() { m_stoprequested = true; };
	bool			IsBusy() { return m_Busy; };
	bool			IsOn() { return (m_CurrentStatus.Status() != MSTAT_OFF); };

	int				m_ID;
	int				m_DevID;
	std::string		m_Name;

protected:
	bool			m_stoprequested;
	bool			m_Busy;
	bool			m_Stoppable;

private:
	void			handleConnect();
	void			handleRead(const boost::system::error_code&, std::size_t);
	void			handleWrite(std::string);
	void			handleDisconnect();
	void			handleMessage(std::string&);


	int				m_HwdID;
	char			m_szDevID[40];
	std::string		m_IP;
	std::string		m_Port;

	CKodiStatus		m_PreviousStatus;
	CKodiStatus		m_CurrentStatus;
	void			UpdateStatus();

	std::string		m_PlaylistType;
	std::string		m_Playlist;
	int				m_PlaylistPosition;

	std::string		m_ExecuteCommand;

	std::string		m_RetainedData;

	int				m_iTimeoutCnt;
	int				m_iPollIntSec;
	int				m_iMissedPongs;
	std::string		m_sLastMessage;
	boost::asio::io_service *m_Ios;
	boost::asio::ip::tcp::socket *m_Socket;
	boost::array<char, 256> m_Buffer;
};

class CKodi : public CDomoticzHardwareBase
{
public:
	CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	explicit CKodi(const int ID);
	~CKodi(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
	void SendCommand(const int ID, const std::string &command);
	bool SetPlaylist(const int ID, const std::string &playlist);
	bool SetExecuteCommand(const int ID, const std::string &command);
private:
	void Do_Work();

	bool StartHardware();
	bool StopHardware();

	void ReloadNodes();
	void UnloadNodes();

	static	std::vector<boost::shared_ptr<CKodiNode> > m_pNodes;

	int m_iPollInterval;
	int m_iPingTimeoutms;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
	boost::asio::io_service m_ios;
};

