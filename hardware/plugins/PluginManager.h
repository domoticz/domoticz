#pragma once

//
//	Domotoicz Plugin System - Dnpwwo, 2016
//

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
		PDT_Debug = 0,
		PDT_PollInterval,
		PDT_Transport,
		PDT_Protocol
	};

	class CPluginMessage
	{
	public:
		CPluginMessage(ePluginMessageType Type, int HwdID, const std::string & Message) : m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, int HwdID, int Unit, const std::string & Message, const int level, const int hue) : m_Type(Type), m_HwdID(HwdID), m_Unit(Unit), m_iLevel(level), m_iHue(hue), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, const std::string & Message) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1) {};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, int Value) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(Value) {};
		CPluginMessage(ePluginMessageType Type, int HwdID) : m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1) {	};
		~CPluginMessage(void) {};
	
		ePluginMessageType		m_Type;
		ePluginDirectiveType	m_Directive;
		int						m_HwdID;
		int						m_Unit;
		std::string				m_Message;
		int						m_iValue;
		int						m_iLevel;
		int						m_iHue;
	};

	class CPluginProtocol {	};

	class CPluginProtocolLine : CPluginProtocol {};

	class CPluginProtocolXML : CPluginProtocol {};

	class CPluginProtocolJSON : CPluginProtocol {};

	class CPluginProtocolHTML : CPluginProtocol {};

	class CPluginProtocolMQTT : CPluginProtocol {}; // Maybe?

	class CPluginTransport {	};

	class CPluginTransportTCP : CPluginTransport {};

	class CPluginTransportUDP : CPluginTransport {};

	class CPluginTransportSerial : CPluginTransport {};

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		int				m_iPollInterval;

		PyThreadState*	m_PyInterpreter;
		PyObject*		m_PyModule;

		boost::shared_ptr<boost::thread> m_thread;

		bool StartHardware();
		void Do_Work();
		bool StopHardware();

		void HandleStart();

	protected:
		bool			m_stoprequested;

	public:
		CPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		~CPlugin(void);

		void HandleMessage(const CPluginMessage& Message);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void Restart();
		void SendCommand(const int Unit, const std::string &command, const int level, const int hue);

		std::string		m_PluginKey;
		bool			m_bDebug;
	};

	class CPluginSystem
	{
	private:
		bool	m_bEnabled;
		bool	m_bAllPluginsStarted;
		int		m_iPollInterval;

		std::map<int, CPlugin*>	m_pPlugins;

		boost::shared_ptr<boost::thread> m_thread;
		volatile bool m_stoprequested;
		boost::mutex m_mutex;
		boost::asio::io_service m_ios;

		void Do_Work();

	public:
		CPluginSystem();
		~CPluginSystem(void);

		bool StartPluginSystem();
		CPlugin* RegisterPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		bool StopPluginSystem();
		void AllPluginsStarted() { m_bAllPluginsStarted = true; };
	};
};

