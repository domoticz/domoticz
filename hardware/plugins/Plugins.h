#pragma once

#include "../DomoticzHardware.h"
#include "../ASyncSerial.h"
#include <boost/asio.hpp>

namespace Plugins {

	enum ePluginMessageType {
		PMT_Initialise = 0,
		PMT_Start,
		PMT_Directive,
		PMT_Connected,
		PMT_Read,
		PMT_Message,
		PMT_Heartbeat,
		PMT_Disconnect,
		PMT_Command,
		PMT_Notification,
		PMT_Stop
	};

	enum ePluginDirectiveType {
		PDT_PollInterval = 0,
		PDT_Transport,
		PDT_Protocol,
		PDT_Connect,
		PDT_Write,
		PDT_Disconnect
	};

	class CPluginMessage
	{
	public:
		CPluginMessage() : m_Type(PMT_Start), m_HwdID(-1), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message("") { };
		CPluginMessage(ePluginMessageType Type, int HwdID, const std::string & Message) : m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, int HwdID, int Unit, const std::string & Message, const int level, const int hue) : m_Type(Type), m_HwdID(HwdID), m_Unit(Unit), m_iLevel(level), m_iHue(hue), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, const std::string & Message) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message) { };
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1) {};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, int Value) : m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(Value) {};
		CPluginMessage(ePluginMessageType Type, int HwdID) : m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1) {	};
		void operator=(const CPluginMessage& m)
		{
			m_Type = m.m_Type;
			m_Directive = m.m_Directive;
			m_HwdID = m.m_HwdID;
			m_Unit = m.m_Unit;
			m_Message = m.m_Message;
			m_iValue = m.m_iValue;
			m_iLevel = m.m_iLevel;
			m_iHue = m.m_iHue;
			m_Address = m.m_Address;
			m_Port = m.m_Port;
		}

		~CPluginMessage(void) {};

		ePluginMessageType		m_Type;
		ePluginDirectiveType	m_Directive;
		int						m_HwdID;
		int						m_Unit;
		std::string				m_Message;
		int						m_iValue;
		int						m_iLevel;
		int						m_iHue;
		std::string				m_Address;
		std::string				m_Port;
	};

	class CPluginProtocol
	{
	protected:
		std::string		m_sRetainedData;

	public:
		virtual void	ProcessMessage(const int HwdID, std::string& ReadData);
		virtual void	Flush(const int HwdID);
		virtual int		Length() { return m_sRetainedData.length(); };
	};

	class CPluginProtocolLine : CPluginProtocol
	{
		virtual void	ProcessMessage(const int HwdID, std::string& ReadData);
	};

	class CPluginProtocolXML : CPluginProtocol
	{
	private:
		std::string		m_Tag;
	public:
		virtual void	ProcessMessage(const int HwdID, std::string& ReadData);
	};

	class CPluginProtocolJSON : CPluginProtocol
	{
		virtual void	ProcessMessage(const int HwdID, std::string& ReadData);
	};

	class CPluginProtocolHTML : CPluginProtocol {};

	class CPluginProtocolMQTT : CPluginProtocol {}; // Maybe?

	class CPluginTransport
	{
	protected:
		int				m_HwdID;
		std::string		m_Port;

		bool			m_bConnected;
		long			m_iTotalBytes;

		char			m_Buffer[4096];

	public:
		CPluginTransport(int HwdID) : m_HwdID(HwdID), m_bConnected(false), m_iTotalBytes(0) {};
		virtual	bool		handleConnect() { return false; };
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleRead(const char *data, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::string&) = 0;
		virtual	bool		handleDisconnect() { return false; };
		~CPluginTransport() {}

		bool				IsConnected() { return m_bConnected; };
		virtual bool		ThreadPoolRequired() { return false; };
		long				TotalBytes() { return m_iTotalBytes; };
	};

	class CPluginTransportIP : public CPluginTransport
	{
	protected:
		std::string			m_IP;
		boost::asio::ip::tcp::resolver	*m_Resolver;
		boost::asio::ip::tcp::socket	*m_Socket;
	public:
		CPluginTransportIP(int HwdID, const std::string& Address, const std::string& Port) : CPluginTransport(HwdID), m_IP(Address), m_Socket(NULL), m_Resolver(NULL) { m_Port = Port; };
		~CPluginTransportIP()
		{
			if (m_Socket)
			{
				handleDisconnect();
				delete m_Socket;
			}
			if (m_Resolver) delete m_Resolver;
		}
		virtual bool		ThreadPoolRequired() { return true; };
	};

	class CPluginTransportTCP : public CPluginTransportIP, boost::enable_shared_from_this<CPluginTransportTCP>
	{
	public:
		CPluginTransportTCP(int HwdID, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, Address, Port) { };
		virtual	bool		handleConnect();
		virtual	void		handleAsyncResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		virtual	void		handleAsyncConnect(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::string& pMessage);
		virtual	bool		handleDisconnect();
	};

	class CPluginTransportUDP : CPluginTransportIP
	{
	public:
		CPluginTransportUDP(int HwdID, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, Address, Port) { };
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred) {};
		virtual void		handleWrite(const std::string&) {};
	};

	class CPluginTransportSerial : CPluginTransport, AsyncSerial
	{
	private:
		int					m_Baud;
	public:
		CPluginTransportSerial(int HwdID, const std::string& Port, int Baud);
		~CPluginTransportSerial(void);
		virtual	bool		handleConnect();
		virtual void		handleRead(const char *data, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::string&);
		virtual	bool		handleDisconnect();
	};

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		int				m_iPollInterval;

		void*			m_PyInterpreter;
		void*			m_PyModule;

		boost::shared_ptr<boost::thread> m_thread;

		bool StartHardware();
		void Do_Work();
		bool StopHardware();
		void LogPythonException();
		void LogPythonException(const std::string &);
		bool HandleInitialise();
		bool HandleStart();

	protected:
		bool			m_stoprequested;

	public:
		CPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		~CPlugin(void);

		void HandleMessage(const CPluginMessage& Message);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void Restart();
		void SendCommand(const int Unit, const std::string &command, const int level, const int hue);

		std::string			m_PluginKey;
		CPluginProtocol*	m_pProtocol;
		CPluginTransport*	m_pTransport;
		void*				m_DeviceDict;
		bool				m_bDebug;
	};

}