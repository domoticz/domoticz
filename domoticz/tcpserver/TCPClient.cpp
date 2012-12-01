#include "stdafx.h"
#include "TCPClient.h"
#include "TCPServer.h"

namespace tcp {
namespace server {

CTCPClient::CTCPClient(boost::asio::io_service& ios, CTCPServerInt *pManager)
	: socket_(ios), pConnectionManager(pManager)
{
	m_bIsLoggedIn=false;
}


CTCPClient::~CTCPClient(void)
{
}

void CTCPClient::start()
{
	socket_.async_read_some(boost::asio::buffer(buffer_),
		boost::bind(&CTCPClient::handleRead, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void CTCPClient::stop()
{
	 socket_.close();
}

void CTCPClient::handleRead(const boost::system::error_code& e,
	std::size_t bytes_transferred)
{
	if (!e)
	{
		//do something with the data
		//buffer_.data(), buffer_.data() + bytes_transferred

		//ready for next read
		socket_.async_read_some(boost::asio::buffer(buffer_),
			boost::bind(&CTCPClient::handleRead, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
	else if (e != boost::asio::error::operation_aborted)
	{
		pConnectionManager->stopClient(shared_from_this());
	}
}

void CTCPClient::write(const char *pData, size_t Length)
{
	boost::asio::async_write(socket_, boost::asio::buffer(pData, Length),
		boost::bind(&CTCPClient::handleWrite, shared_from_this(),
		boost::asio::placeholders::error));
}

void CTCPClient::handleWrite(const boost::system::error_code& error)
{
	if (error)
	{
		pConnectionManager->stopClient(shared_from_this());
	}
}

} // namespace server
} // namespace tcp
