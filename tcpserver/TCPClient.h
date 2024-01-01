#pragma once

#include "../main/Noncopyable.h"
#include <boost/asio.hpp>

namespace tcp {
namespace server {

class CTCPServerIntBase;

class CTCPClientBase : 
	private domoticz::noncopyable
{
public:
	explicit CTCPClientBase(CTCPServerIntBase *pManager);
	~CTCPClientBase();

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void write(const char *pData, size_t Length) = 0;

	std::string m_username;
	std::string m_endpoint;
	bool m_bIsLoggedIn = false;

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
	~CTCPClient() = default;
	void start() override;
	void stop() override;
	void write(const char *pData, size_t Length) override;

      private:
	void handleRead(const boost::system::error_code& error, size_t length);
	void handleWrite(const boost::system::error_code& error);

	/// Buffer for incoming data.
	std::array<char, 8192> buffer_;
};

typedef std::shared_ptr<CTCPClientBase> CTCPClient_ptr;

} // namespace server
} // namespace tcp
