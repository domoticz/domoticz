#pragma once

#include "../ASyncSerial.h"
#include <boost/asio.hpp>

namespace Plugins {

	class CPluginTransport
	{
	protected:
		int				m_HwdID;
		std::string		m_Port;

		bool			m_bConnected;
		long			m_iTotalBytes;

		unsigned char	m_Buffer[4096];

	public:
		CPluginTransport(int HwdID) : m_HwdID(HwdID), m_bConnected(false), m_iTotalBytes(0) {};
		virtual	bool		handleConnect() { return false; };
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
		virtual void		handleRead(const char *data, std::size_t bytes_transferred);
		virtual void		handleWrite(const std::vector<byte>&) = 0;
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
		virtual void		handleWrite(const std::vector<byte>& pMessage);
		virtual	bool		handleDisconnect();
	};

	class CPluginTransportUDP : CPluginTransportIP
	{
	public:
		CPluginTransportUDP(int HwdID, const std::string& Address, const std::string& Port) : CPluginTransportIP(HwdID, Address, Port) { };
		virtual void		handleRead(const boost::system::error_code& e, std::size_t bytes_transferred) {};
		virtual void		handleWrite(const std::vector<byte>&) {};
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
		virtual void		handleWrite(const std::vector<byte>&);
		virtual	bool		handleDisconnect();
	};

}