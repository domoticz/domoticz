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
#include "Include/Python.h"

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

namespace Plugins {

	enum ePluginMessageType {
		PMT_Start = 0,
		PMT_Directive,
		PMT_Connected,
		PMT_Read,
		PMT_Write,
		PMT_Heartbeat,
		PMT_Disconnect,
		PMT_Command,
		PMT_Stop
	};

	enum ePluginDirectiveType {
		PMT_Debug = 0,
		PMT_Transport,
		PMT_Protocol
	};

	class CPluginMessage
	{
	public:
		CPluginMessage(ePluginMessageType Type, int HwdID, std::string& Message);
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, std::string& Message);
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID);
		CPluginMessage(ePluginMessageType Type, int HwdID);
		~CPluginMessage(void) {};
	
		ePluginMessageType		m_Type;
		ePluginDirectiveType	m_Directive;
		int						m_HwdID;
		std::string				m_Message;
	};

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		std::string			m_PluginKey;

		int					m_iPollInterval;
		int					m_iPingTimeoutms;

		PyThreadState*		m_PyInterpreter;
		PyObject*			m_PyModule;

		bool				m_bDebug;

		bool StartHardware();
		void Do_Work();
		bool StopHardware();

	public:
		CPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		~CPlugin(void);

		void HandleMessage(const CPluginMessage& Message);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void Restart();
		void SendCommand(const int ID, const std::string &command);

	protected:
		bool			m_stoprequested;
		bool			m_Busy;
		bool			m_Stoppable;

	private:
		boost::mutex m_mutex;
		boost::shared_ptr<boost::thread> m_thread;
	};

	class CPluginSystem
	{
	private:
		bool	m_bEnabled;
		bool	m_bAllPluginsStarted;
		int		m_iPollInterval;

		std::map<int, CPlugin*>	m_pPlugins;

		void Do_Work();

	public:
		CPluginSystem();
		~CPluginSystem(void);

		bool StartPluginSystem();
		CPlugin* RegisterPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		bool StopPluginSystem();
		void AllPluginsStarted() { m_bAllPluginsStarted = true; };

	private:
		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;
		boost::mutex m_mutex;
		boost::asio::io_service m_ios;
	};
};

