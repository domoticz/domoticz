#pragma once

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace tcp {
namespace server {

class CTCPServerInt;

class CTCPClient : public boost::enable_shared_from_this<CTCPClient>,
	private boost::noncopyable
{
public:
	explicit CTCPClient(boost::asio::io_service& ios, CTCPServerInt *pManager);
	~CTCPClient(void);

	void start();
	void stop();

	void write(const char *pData, size_t Length);

	boost::asio::ip::tcp::socket& socket() { return socket_; }
	bool m_bIsLoggedIn;
private:
	void handleRead(const boost::system::error_code& error, size_t length);
	void handleWrite(const boost::system::error_code& error);

	boost::asio::ip::tcp::socket socket_;
	/// Buffer for incoming data.
	boost::array<char, 8192> buffer_;

	/// The manager for this connection.
	CTCPServerInt *pConnectionManager;

};

typedef boost::shared_ptr<CTCPClient> CTCPClient_ptr;

} // namespace server
} // namespace tcp
