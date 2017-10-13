#pragma once

#include "../ASyncSerial.h"
#include <boost/asio.hpp>

namespace Plugins {

	class CPluginTransport
	{
	protected:
		int				m_HwdID;
		std::string		m_Port;

		bool			m_bDisconnectQueued;
		bool			m_bConnecting;
		bool			m_bConnected;
		long			m_iTotalBytes;
		time_t			m_tLastSeen;

		unsigned char	m_Buffer[4096];

		PyObject*		m_pConnection;

	public:
		CPluginTransport(int HwdID, PyObject* pConnection) : m_HwdID(HwdID), m_pConnection(pConnection), m_bConnecting(false), m_bConnected(false), m_bDisconnectQueued(false), m_iTotalBytes(0), m_tLastSeen(0)
		{
			Py_INCREF(m_pConnection);
		};
		virtual	bool		handleConnect() { return false; };
		virtual	bool		handleListen() { return false; };
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleRead(const char *data, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>&) = 0;
		virtual	bool		handleDisconnect() { return false; };
		virtual ~CPluginTransport()
		{
			Py_DECREF(m_pConnection);
		}

		bool				IsConnecting() { return m_bConnecting; };
		bool				IsConnected() { return m_bConnected; };
		time_t				LastSeen() { return m_tLastSeen; };
		virtual bool		ThreadPoolRequired() { return false; };
		long				TotalBytes() { return m_iTotalBytes; };
		virtual void		VerifyConnection();
		PyObject*			Connection() { return m_pConnection; };
	};

	class CPluginTransportIP : public CPluginTransport
	{
	protected:
		std::string			m_IP;
	public:
		CPluginTransportIP(int HwdID, PyObject* pConnection, const std::string& Address, const std::string& Port) : CPluginTransport(HwdID, pConnection), m_IP(Address) { m_Port = Port; };
	};

	class CPluginTransportTCP : public CPluginTransportIP, boost::enable_shared_from_this<CPluginTransportTCP>
	{
	public:
		CPluginTransportTCP(int HwdID, PyObject* pConnection, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, pConnection, Address, Port), m_Socket(NULL), m_Resolver(NULL), m_Acceptor(NULL) { };
		virtual	bool		handleConnect();
		virtual	bool		handleListen();
		virtual	void		handleAsyncResolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		virtual	void		handleAsyncConnect(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
		virtual void		handleAsyncAccept(boost::asio::ip::tcp::socket* pSocket, const boost::system::error_code & error);
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>& pMessage);
		virtual	bool		handleDisconnect();
		virtual bool		ThreadPoolRequired() { return true; };
		boost::asio::ip::tcp::socket& Socket() { return *m_Socket; };
		~CPluginTransportTCP();

	protected:
		boost::asio::ip::tcp::resolver	*m_Resolver;
		boost::asio::ip::tcp::acceptor	*m_Acceptor;
		boost::asio::ip::tcp::socket	*m_Socket;
	};

	class CPluginTransportUDP : CPluginTransportIP
	{
	public:
		CPluginTransportUDP(int HwdID, PyObject* pConnection, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, pConnection, Address, Port), m_Socket(NULL), m_Resolver(NULL) { };
		virtual	bool		handleListen();
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>&);
		virtual	bool		handleDisconnect();
		~CPluginTransportUDP();
	protected:
		boost::asio::ip::udp::resolver	*m_Resolver;
		boost::asio::ip::udp::socket	*m_Socket;
		boost::asio::ip::udp::endpoint	m_remote_endpoint;
	};

	class CPluginTransportICMP : CPluginTransportIP
	{
	public:
		CPluginTransportICMP(int HwdID, PyObject* pConnection, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, pConnection, Address, Port), m_Socket(NULL), m_Resolver(NULL), m_Timer(NULL), m_SequenceNo(0), m_Initialised(false) { };
		virtual	void		handleAsyncResolve(const boost::system::error_code& err, boost::asio::ip::icmp::resolver::iterator endpoint_iterator);
		virtual	bool		handleListen();
		virtual void		handleTimeout(const boost::system::error_code&);
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>&);
		virtual	bool		handleDisconnect();
		~CPluginTransportICMP();
	protected:
		boost::asio::ip::icmp::resolver*	m_Resolver;
		boost::asio::ip::icmp::socket*		m_Socket;
		boost::asio::ip::icmp::endpoint		m_Endpoint;
		boost::asio::deadline_timer*		m_Timer;

		int									m_SequenceNo;
		bool								m_Initialised;
	};

	class CPluginTransportSerial : CPluginTransport, AsyncSerial
	{
	private:
		int					m_Baud;
	public:
		CPluginTransportSerial(int HwdID, PyObject* pConnection, const std::string& Port, int Baud);
		~CPluginTransportSerial(void);
		virtual	bool		handleConnect();
		virtual void		handleRead(const char *data, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>&);
		virtual	bool		handleDisconnect();
	};

}