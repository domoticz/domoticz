#include "stdafx.h"
#include <iostream>
#include "TCPServer.h"
#include "TCPClient.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"

#include <boost/asio.hpp>
#include <algorithm>
#include <boost/bind.hpp>

namespace tcp {
namespace server {

CTCPServerInt::CTCPServerInt(const std::string& address, const std::string& port):
	io_service_(),
	acceptor_(io_service_)
{
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
//	stopAllClients();
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
	stopAllClients();
}

void CTCPServerInt::handleAccept(const boost::system::error_code& error)
{
	if(!error) // 1.
	{
		boost::lock_guard<boost::mutex> l(connectionMutex);
		std::string s = new_connection_->socket().remote_endpoint().address().to_string();
		_log.Log(LOG_NORM,"Incoming connection from: %s", s.c_str());

		connections_.insert(new_connection_);
		new_connection_->start();

		new_connection_.reset(new CTCPClient(io_service_, this));

		acceptor_.async_accept(
			new_connection_->socket(),
			boost::bind(&CTCPServerInt::handleAccept, this,
			boost::asio::placeholders::error));
	}
}

CTCPServerInt::_tRemoteShareUser* CTCPServerInt::FindUser(const std::string username)
{
	std::vector<CTCPServerInt::_tRemoteShareUser>::iterator itt;
	int ii=0;
	for (itt=m_users.begin(); itt!=m_users.end(); ++itt)
	{
		if (itt->Username==username)
			return &m_users[ii];
		ii++;
	}
	return NULL;
}

bool CTCPServerInt::HandleAuthentication(CTCPClient_ptr c, const std::string username, const std::string password)
{
	_tRemoteShareUser *pUser=FindUser(username);
	if (pUser==NULL)
		return false;

	return ((pUser->Username==username)&&(pUser->Password==password));
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

void CTCPServerInt::stopAllClients()
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

void CTCPServerInt::SetRemoteUsers(const std::vector<_tRemoteShareUser> users)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	m_users=users;
}

void CTCPServerInt::SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);

	//do not share Interface Messages
	if (
		(pData[1]==pTypeInterfaceMessage)||
		(pData[1]==pTypeRecXmitMessage)
		)
		return;

	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt=connections_.begin(); itt!=connections_.end(); ++itt)
	{
		CTCPClient *pClient=itt->get();
		if (pClient)
		{
			_tRemoteShareUser *pUser=FindUser(pClient->m_username);
			if (pUser!=NULL)
			{
				//check if we are allowed to get this device
				bool bOk2Send=false;
				if (pUser->Devices.size()==0)
					bOk2Send=true;
				else
				{
					int tdevices=pUser->Devices.size();
					for (int ii=0; ii<tdevices; ii++)
					{
						if (pUser->Devices[ii]==DeviceRowID)
						{
							bOk2Send=true;
							break;
						}
					}
				}
				if (bOk2Send)
					pClient->write(pData,Length);
			}
		}
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

bool CTCPServer::StartServer(const std::string address, const std::string port)
{
	try
	{
		StopServer();
		if (m_pTCPServer!=NULL)
			delete m_pTCPServer;
		m_pTCPServer=new CTCPServerInt(address,port);
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

void CTCPServer::SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length)
{
	if (m_pTCPServer)
		m_pTCPServer->SendToAll(DeviceRowID,pData,Length);
}

void CTCPServer::SetRemoteUsers(const std::vector<CTCPServerInt::_tRemoteShareUser> users)
{
	if (m_pTCPServer)
		m_pTCPServer->SetRemoteUsers(users);
}

void CTCPServer::stopAllClients()
{
	if (m_pTCPServer)
		m_pTCPServer->stopAllClients();
}

} // namespace server
} // namespace tcp
