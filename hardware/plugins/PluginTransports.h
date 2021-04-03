#pragma once

#include "../ASyncSerial.h"
#include <boost/asio.hpp>
#include <ctime>

namespace Plugins {

	extern boost::asio::io_service ios;

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

		CConnection *	m_pConnection;

	protected:
		boost::asio::deadline_timer *m_Timer;
		virtual void configureTimeout();

	      public:
		CPluginTransport(int HwdID, CConnection *pConnection) : m_HwdID(HwdID), m_pConnection(pConnection), m_bDisconnectQueued(false), m_bConnecting(false), m_bConnected(false), m_iTotalBytes(0), m_tLastSeen(0), m_Timer(NULL)
	  {
		  Py_INCREF(m_pConnection);
	  };
		virtual	bool	handleConnect() { return false; };
		virtual	bool	handleListen() { return false; };
		virtual void	handleTimeout(const boost::system::error_code &);
		virtual void	handleRead(const boost::system::error_code &e, std::size_t bytes_transferred);
		virtual void	handleRead(const char *data, std::size_t bytes_transferred);
		virtual void	handleWrite(const std::vector<byte>&) = 0;
		virtual	bool	handleDisconnect() { return false; };
		virtual ~CPluginTransport()
		{
			Py_DECREF(m_pConnection);
		}

		bool				IsConnecting() { return m_bConnecting; };
		bool				IsConnected() { return m_bConnected; };
		time_t				LastSeen() { return m_tLastSeen; };
		virtual bool		AsyncDisconnect() { return false; };
		virtual bool		ThreadPoolRequired() { return false; };
		long				TotalBytes() { return m_iTotalBytes; };
		virtual void		VerifyConnection();
		CConnection *		Connection()
		{
			return m_pConnection;
		};
	};

	class CPluginTransportIP : public CPluginTransport
	{
	protected:
		std::string			m_IP;
	public:
		CPluginTransportIP(int HwdID, CConnection *pConnection, const std::string &Address, const std::string &Port)
			: CPluginTransport(HwdID, pConnection)
			, m_IP(Address)
		{
			m_Port = Port;
		};
		bool AsyncDisconnect() override
		{
			return IsConnected() || IsConnecting();
		};
	};

	class CPluginTransportTCP : public CPluginTransportIP, std::enable_shared_from_this<CPluginTransportTCP>
	{
	public:
		CPluginTransportTCP(int HwdID, CConnection *pConnection, const std::string &Address, const std::string &Port)
		  : CPluginTransportIP(HwdID, pConnection, Address, Port)
		  , m_Resolver(ios)
		  , m_Acceptor(nullptr)
		  , m_Socket(nullptr){};
	  bool handleConnect() override;
	  bool handleListen() override;
	  virtual void handleAsyncResolve(const boost::system::error_code &err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	  virtual void handleAsyncConnect(const boost::system::error_code &err, const boost::asio::ip::tcp::resolver::iterator &endpoint_iterator);
	  virtual void handleAsyncAccept(boost::asio::ip::tcp::socket *pSocket, const boost::system::error_code &error);
	  void handleRead(const boost::system::error_code &e, std::size_t bytes_transferred) override;
	  void handleWrite(const std::vector<byte> &pMessage) override;
	  bool handleDisconnect() override;
	  bool ThreadPoolRequired() override
	  {
		  return true;
	  };
		boost::asio::ip::tcp::socket& Socket() { return *m_Socket; };
		~CPluginTransportTCP() override;

	      protected:
		boost::asio::ip::tcp::resolver	m_Resolver;
		boost::asio::ip::tcp::acceptor	*m_Acceptor;
		boost::asio::ip::tcp::socket	*m_Socket;
	};

	class CPluginTransportTCPSecure : public CPluginTransportTCP
	{
	public:
		CPluginTransportTCPSecure(int HwdID, CConnection *pConnection, const std::string &Address, const std::string &Port)
		  : CPluginTransportTCP(HwdID, pConnection, Address, Port)
		  , m_Context(nullptr)
		  , m_TLSSock(nullptr){};
	  void handleAsyncConnect(const boost::system::error_code &err, const boost::asio::ip::tcp::resolver::iterator &endpoint_iterator) override;
	  void handleRead(const boost::system::error_code &e, std::size_t bytes_transferred) override;
	  void handleWrite(const std::vector<byte> &pMessage) override;
	  ~CPluginTransportTCPSecure() override;

	protected:
	  bool VerifyCertificate(bool preverified, boost::asio::ssl::verify_context &ctx);

	  boost::asio::ssl::context *m_Context;
	  boost::asio::ssl::stream<boost::asio::ip::tcp::socket &> *m_TLSSock;
	};

	class CPluginTransportUDP : CPluginTransportIP
	{
	public:
		CPluginTransportUDP(int HwdID, CConnection *pConnection, const std::string &Address, const std::string &Port)
		  : CPluginTransportIP(HwdID, pConnection, Address, Port)
		  , m_Resolver(ios)
		  , m_Socket(nullptr){};
	  bool handleListen() override;
	  void handleRead(const boost::system::error_code &e, std::size_t bytes_transferred) override;
	  void handleWrite(const std::vector<byte> &) override;
	  bool handleDisconnect() override;
	  ~CPluginTransportUDP() override;

	protected:
	  boost::asio::ip::udp::resolver m_Resolver;
	  boost::asio::ip::udp::socket *m_Socket;
	  boost::asio::ip::udp::endpoint m_remote_endpoint;
	};

	class CPluginTransportICMP : CPluginTransportIP
	{
	public:
		CPluginTransportICMP(int HwdID, CConnection *pConnection, const std::string &Address, const std::string &Port)
		  : CPluginTransportIP(HwdID, pConnection, Address, Port)
		  , m_Resolver(ios)
		  , m_Socket(nullptr)
		  , m_Timer(nullptr)
		  , m_SequenceNo(-1){};
	  void handleAsyncResolve(const boost::system::error_code &err, const boost::asio::ip::icmp::resolver::iterator &endpoint_iterator);
	  bool handleListen() override;
	  void handleTimeout(const boost::system::error_code &) override;
	  void handleRead(const boost::system::error_code &e, std::size_t bytes_transferred) override;
	  void handleWrite(const std::vector<byte> &) override;
	  bool handleDisconnect() override;
	  ~CPluginTransportICMP() override;

	protected:
	  boost::asio::ip::icmp::resolver m_Resolver;
	  boost::asio::ip::icmp::socket *m_Socket;
	  boost::asio::ip::icmp::endpoint m_Endpoint;
	  boost::asio::deadline_timer *m_Timer;

	  clock_t m_Clock;
	  int m_SequenceNo;
	};

	class CPluginTransportSerial : CPluginTransport, AsyncSerial
	{
	private:
		int					m_Baud;
	public:
		CPluginTransportSerial(int HwdID, CConnection *pConnection, const std::string &Port, int Baud);
		~CPluginTransportSerial() override = default;
		bool handleConnect() override;
		void handleRead(const char *data, std::size_t bytes_transferred) override;
		void handleWrite(const std::vector<byte> &) override;
		bool handleDisconnect() override;
	};

} // namespace Plugins
