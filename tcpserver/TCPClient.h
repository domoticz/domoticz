#pragma once

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../webserver/proxyclient.h"

namespace tcp {
namespace server {

class CTCPServerIntBase;

class CTCPClientBase : 
	private boost::noncopyable
{
public:
	explicit CTCPClientBase(CTCPServerIntBase *pManager);
	~CTCPClientBase(void);

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void write(const char *pData, size_t Length) = 0;

	std::string m_username;
	std::string m_endpoint;
	bool m_bIsLoggedIn;

	// usual tcp parameters
	boost::asio::ip::tcp::socket *socket() { return socket_; }
protected:
	/// The manager for this connection.
	CTCPServerIntBase *pConnectionManager;

	// usual tcp parameters
	boost::asio::ip::tcp::socket *socket_;
};

class CTCPClient : public CTCPClientBase,
	public boost::enable_shared_from_this<CTCPClient>
{
public:
	CTCPClient(boost::asio::io_service& ios, CTCPServerIntBase *pManager);
	~CTCPClient();
	virtual void start();
	virtual void stop();
	virtual void write(const char *pData, size_t Length);
private:
	void handleRead(const boost::system::error_code& error, size_t length);
	void handleWrite(const boost::system::error_code& error);

	/// Buffer for incoming data.
	boost::array<char, 8192> buffer_;

};

#ifndef NOCLOUD
class CSharedClient : public CTCPClientBase,
	public boost::enable_shared_from_this<CSharedClient>
{
public:
	CSharedClient(CTCPServerIntBase *pManager, boost::shared_ptr<http::server::CProxyClient> proxy, const std::string &token, const std::string &username);
	~CSharedClient();
	virtual void start();
	virtual void stop();
	virtual void write(const char *pData, size_t Length);
	void OnIncomingData(const unsigned char *data, size_t bytes_transferred);
	bool CompareToken(const std::string &token);
private:
	boost::shared_ptr<http::server::CProxyClient> m_pProxyClient;
	std::string m_token;
};
#endif

typedef boost::shared_ptr<CTCPClientBase> CTCPClient_ptr;

} // namespace server
} // namespace tcp
