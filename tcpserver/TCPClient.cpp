#include "stdafx.h"
#include "TCPClient.h"
#include "TCPServer.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../webserver/proxyclient.h"

namespace tcp {
namespace server {

CTCPClientBase::CTCPClientBase(CTCPServerIntBase *pManager)
	: pConnectionManager(pManager)
{
	socket_ = nullptr;
	m_bIsLoggedIn = false;
}

CTCPClientBase::~CTCPClientBase()
{
	delete socket_;
}

CTCPClient::CTCPClient(boost::asio::io_service& ios, CTCPServerIntBase *pManager)
	: CTCPClientBase(pManager)
{
	socket_ = new boost::asio::ip::tcp::socket(ios);
}

void CTCPClient::start()
{
	socket_->async_read_some(boost::asio::buffer(buffer_), [self = shared_from_this()](auto &&err, auto &&bytes) { self->handleRead(err, bytes); });
}

void CTCPClient::stop()
{
	 socket_->close();
}

void CTCPClient::handleRead(const boost::system::error_code& e,
	std::size_t bytes_transferred)
{
	auto self = shared_from_this();
	if (!e)
	{
		//do something with the data
		//buffer_.data(), buffer_.data() + bytes_transferred
		if (bytes_transferred>7)
		{
			std::string recstr;
			recstr.append(buffer_.data(),bytes_transferred);
			if (recstr.find("AUTH")!=std::string::npos)
			{
				//Authentication
				std::vector<std::string> strarray;
				StringSplit(recstr, ";", strarray);
				if (strarray.size()==3)
				{
					m_bIsLoggedIn = pConnectionManager->HandleAuthentication(self, strarray[1], strarray[2]);
					if (!m_bIsLoggedIn)
					{
						//Wrong username/password
						boost::asio::async_write(*socket_, boost::asio::buffer("NOAUTH", 6), [self](auto &&err, auto) { self->handleWrite(err); });
						pConnectionManager->stopClient(self);
						return;
					}
					m_username=strarray[1];
				}
			}
			else
			{
				pConnectionManager->DoDecodeMessage(this,(const unsigned char *)buffer_.data());
			}
		}

		//ready for next read
		socket_->async_read_some(boost::asio::buffer(buffer_), [self](auto &&err, auto bytes) { self->handleRead(err, bytes); });
	}
	else if (e != boost::asio::error::operation_aborted)
	{
		pConnectionManager->stopClient(self);
	}
}

void CTCPClient::write(const char *pData, size_t Length)
{
	if (!m_bIsLoggedIn)
		return;
	boost::asio::async_write(*socket_, boost::asio::buffer(pData, Length), [self = shared_from_this()](auto &&err, auto) { self->handleWrite(err); });
}

void CTCPClient::handleWrite(const boost::system::error_code& error)
{
	if (error)
	{
		pConnectionManager->stopClient(shared_from_this());
	}
}

#ifndef NOCLOUD
/* shared server via proxy client class */
CSharedClient::CSharedClient(CTCPServerIntBase *pManager, http::server::CProxyClient *proxy, const std::string &token, const std::string &username) :
	CTCPClientBase(pManager),
	m_token(token)
{
	m_username = username;
	m_pProxyClient = proxy;
}

void CSharedClient::start()
{
	m_bIsLoggedIn = true;
}

void CSharedClient::stop()
{
	m_bIsLoggedIn = false;
}

void CSharedClient::OnIncomingData(const unsigned char *data, size_t bytes_transferred)
{
	if (!m_bIsLoggedIn) {
		return;
	}
	pConnectionManager->DoDecodeMessage(this, data);
}

void CSharedClient::write(const char *pData, size_t Length)
{
	if (!m_bIsLoggedIn)
		return;
	// RK, todo: m_pProxyClient is not valid after a reconnect
	m_pProxyClient->WriteSlaveData(m_token, pData, Length);
}

bool CSharedClient::CompareToken(const std::string &token)
{
	return (token == m_token);
}
#endif

} // namespace server
} // namespace tcp
