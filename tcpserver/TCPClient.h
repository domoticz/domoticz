#pragma once

#include "../main/Noncopyable.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>

namespace http {
	namespace server {
		class CProxyClient;
	}
};

namespace tcp {
namespace server {

class CTCPServerIntBase;

class CTCPClientBase : 
	private domoticz::noncopyable
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
	public std::enable_shared_from_this<CTCPClient>
{
public:
	CTCPClient(boost::asio::io_service& ios, CTCPServerIntBase *pManager);
	~CTCPClient();
	virtual void start() override;
	virtual void stop() override;
	virtual void write(const char *pData, size_t Length) override;
private:
	void handleRead(const boost::system::error_code& error, size_t length);
	void handleWrite(const boost::system::error_code& error);

	/// Buffer for incoming data.
	boost::array<char, 8192> buffer_;

};

#ifndef NOCLOUD
class CSharedClient : public CTCPClientBase,
	public std::enable_shared_from_this<CSharedClient>
{
public:
	CSharedClient(CTCPServerIntBase *pManager, std::shared_ptr<http::server::CProxyClient> proxy, const std::string &token, const std::string &username);
	~CSharedClient();
	virtual void start() override;
	virtual void stop() override;
	virtual void write(const char *pData, size_t Length) override;
	void OnIncomingData(const unsigned char *data, size_t bytes_transferred);
	bool CompareToken(const std::string &token);
private:
	std::shared_ptr<http::server::CProxyClient> m_pProxyClient;
	std::string m_token;
};
#endif

typedef std::shared_ptr<CTCPClientBase> CTCPClient_ptr;

} // namespace server
} // namespace tcp
