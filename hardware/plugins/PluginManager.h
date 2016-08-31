#pragma once

#include "..\DomoticzHardware.h"

#include "../main/localtime_r.h"
#include <string>
#include <vector>
#include "../tinyxpath/tinyxml.h"
#include "../json/json.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

namespace Plugins {

	enum ePluginMessageType {
		PMT_Start = 0,
		PMT_Connected,
		PMT_Read,
		PMT_Write,
		PMT_Heartbeat,
		PMT_Disconnect,
		PMT_Command,
		PMT_Stop
	};

	class CPluginMessage
	{
	public:
		CPluginMessage(ePluginMessageType Type, int HwdID, std::string& Message);
		CPluginMessage(ePluginMessageType Type, int HwdID);
		~CPluginMessage(void) {};
	
		ePluginMessageType	m_Type;
		int					m_HwdID;
		std::string			m_Message;
	};

	class CPluginBase : public boost::enable_shared_from_this<CPluginBase>
	{
	private:
		int				m_HwdID;

	public:
		CPluginBase(const int);
		~CPluginBase(void);

		int				m_ID;
		int				m_DevID;

	protected:
		bool			m_stoprequested;
		bool			m_Busy;
		bool			m_Stoppable;

	private:
		void			handleConnect() {};
		void			handleRead(const boost::system::error_code&, std::size_t) {};
		void			handleWrite(std::string) {};
		void			handleDisconnect() {};
		void			handleMessage(std::string&) {};

		char			m_szDevID[40];
		std::string		m_IP;
		std::string		m_Port;

		int				m_iTimeoutCnt;
		int				m_iPollIntSec;
		int				m_iMissedPongs;
		std::string		m_sLastMessage;
		boost::asio::io_service *m_Ios;
		boost::asio::ip::tcp::socket *m_Socket;
		boost::array<char, 256> m_Buffer;
	};

	class CPluginManager : public CDomoticzHardwareBase
	{
	private:
		std::string			m_PluginKey;
		std::string			m_ManifestFile;
		std::string			m_ManifestEntry;

		int					m_iPollInterval;
		int					m_iPingTimeoutms;

		CPluginBase*		m_pPluginDevice;

		bool StartHardware();
		void Do_Work();
		bool StopHardware();

	public:
		CPluginManager(const int HwdID, const std::string &Name, const std::string &PluginKey);
		~CPluginManager(void);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void Restart();
		void SendCommand(const int ID, const std::string &command);

	private:
		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;
		boost::mutex m_mutex;
		boost::asio::io_service m_ios;
	};
};

