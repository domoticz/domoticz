#include "stdafx.h"
#include <iostream>
#include "TCPServer.h"
#include "TCPClient.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"

#include <boost/asio.hpp>
#include <algorithm>
#include <boost/bind.hpp>

namespace tcp {
namespace server {

CTCPServerInt::CTCPServerInt(const std::string& address, const std::string& port, const std::string username, const std::string password, const _eShareRights rights):
	io_service_(),
	acceptor_(io_service_)
{
	m_username=username;
	m_password=password;
	m_rights=rights;

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	new_connection_= boost::shared_ptr<CTCPClient>(new CTCPClient(io_service_, this));

	acceptor_.async_accept(
		new_connection_->socket(),
		boost::bind(&CTCPServerInt::handleAccept, this,
		boost::asio::placeholders::error));
}


CTCPServerInt::~CTCPServerInt(void)
{
//	stopAllClient();
}

void CTCPServerInt::start()
{
	// The io_service::run() call will block until all asynchronous operations
	// have finished. While the server is running, there is always at least one
	// asynchronous operation outstanding: the asynchronous accept call waiting
	// for new incoming connections.
	io_service_.run();
}

void CTCPServerInt::stop()
{
	// Post a call to the stop function so that server::stop() is safe to call
	// from any thread.
	io_service_.post(boost::bind(&CTCPServerInt::handle_stop, this));
}

void CTCPServerInt::handle_stop()
{
	// The server is stopped by cancelling all outstanding asynchronous
	// operations. Once all operations have finished the io_service::run() call
	// will exit.
	acceptor_.close();
	stopAllClient();
}

void CTCPServerInt::handleAccept(const boost::system::error_code& error)
{
	if(!error) // 1.
	{
		boost::lock_guard<boost::mutex> l(connectionMutex);
		std::string s = new_connection_->socket().remote_endpoint().address().to_string();
		_log.Log(LOG_NORM,"Incoming connection from: %s", s.c_str());

		connections_.insert(new_connection_);
		if (m_username=="")
			new_connection_->m_bIsLoggedIn=true;
		new_connection_->start();

		new_connection_.reset(new CTCPClient(io_service_, this));

		acceptor_.async_accept(
			new_connection_->socket(),
			boost::bind(&CTCPServerInt::handleAccept, this,
			boost::asio::placeholders::error));
	}
}

bool CTCPServerInt::HandleAuthentication(CTCPClient_ptr c, const std::string username, const std::string password)
{
	return ((username==m_username)&&(password==m_password));
}

void CTCPServerInt::stopClient(CTCPClient_ptr c)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);

	//std::string s = c->socket().remote_endpoint().address().to_string();
	//_log.Log(LOG_NORM,"Closing connection from: %s", s.c_str());
	if (connections_.find(c)!=connections_.end())
	{
		connections_.erase(c);
		c->stop();
	}
}

void CTCPServerInt::stopAllClient()
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	if (connections_.size()<1)
		return;
	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt=connections_.begin(); itt!=connections_.end(); ++itt)
	{
		CTCPClient *pClient=itt->get();
		if (pClient)
			pClient->stop();
	}
	connections_.clear();
}

void CTCPServerInt::SendToAll(const char *pData, size_t Length)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);

	//do not share Interface Messages
	if (
		(pData[1]==pTypeInterfaceMessage)||
		(pData[1]==pTypeRecXmitMessage)
		)
		return;

	if (m_rights == SHARE_SENSORS)
	{
		switch (pData[1])
		{
		case pTypeLighting1:
		case pTypeLighting2:
		case pTypeLighting3:
		case pTypeLighting4:
		case pTypeLighting5:
		case pTypeLighting6:
		case pTypeSecurity1:
			return;	//not shared!!
		}
	}

	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt=connections_.begin(); itt!=connections_.end(); ++itt)
	{
		CTCPClient *pClient=itt->get();
		if (pClient)
			pClient->write(pData,Length);
	}
}

//Out main (wrapper) server
CTCPServer::CTCPServer()
{
	m_pTCPServer=NULL;
}

CTCPServer::~CTCPServer()
{
	StopServer();
	if (m_pTCPServer!=NULL)
		delete m_pTCPServer;
	m_pTCPServer=NULL;
}

bool CTCPServer::StartServer(const std::string address, const std::string port, const std::string username, const std::string password, const _eShareRights rights)
{
	try
	{
		StopServer();
		if (m_pTCPServer!=NULL)
			delete m_pTCPServer;
		m_pTCPServer=new CTCPServerInt(address,port,username,password,rights);
		if (!m_pTCPServer)
			return false;
	}
	catch(std::exception& e)
	{
		_log.Log(LOG_ERROR,"Exception: %s",e.what());
		return false;
	}
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CTCPServer::Do_Work, this)));

	return (m_thread!=NULL);
}

void CTCPServer::StopServer()
{
	if (m_pTCPServer)
		m_pTCPServer->stop();
	if (m_thread)
		m_thread->join();
}

void CTCPServer::Do_Work()
{
	if (m_pTCPServer)
		m_pTCPServer->start();
	//_log.Log(LOG_NORM,"TCPServer stopped...");
}

void CTCPServer::SendToAll(const char *pData, size_t Length)
{
	if (m_pTCPServer)
		m_pTCPServer->SendToAll(pData,Length);
}

} // namespace server
} // namespace tcp
