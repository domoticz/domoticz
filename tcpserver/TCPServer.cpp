#include "stdafx.h"
#include <iostream>
#include "TCPServer.h"
#include "TCPClient.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
#include "../hardware/DomoticzTCP.h"
#include "../main/mainworker.h"

#include <boost/asio.hpp>
#include <algorithm>
#include <boost/bind.hpp>

extern MainWorker m_mainworker;

namespace tcp {
namespace server {

CTCPServerIntBase::CTCPServerIntBase(CTCPServer *pRoot)
{
	m_pRoot=pRoot;
}


CTCPServerIntBase::~CTCPServerIntBase(void)
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
		std::string s = new_connection_->socket()->remote_endpoint().address().to_string();
		new_connection_->m_endpoint=s;
		_log.Log(LOG_STATUS,"Incoming Domoticz connection from: %s", s.c_str());

		connections_.insert(new_connection_);
		new_connection_->start();

		new_connection_.reset(new CTCPClient(io_service_, this));

		acceptor_.async_accept(
			*(new_connection_->socket()),
			boost::bind(&CTCPServerInt::handleAccept, this,
			boost::asio::placeholders::error));
	}
}

_tRemoteShareUser* CTCPServerIntBase::FindUser(const std::string &username)
{
	std::vector<_tRemoteShareUser>::iterator itt;
	int ii=0;
	for (itt=m_users.begin(); itt!=m_users.end(); ++itt)
	{
		if (itt->Username==username)
			return &m_users[ii];
		ii++;
	}
	return NULL;
}

bool CTCPServerIntBase::HandleAuthentication(CTCPClient_ptr c, const std::string &username, const std::string &password)
{
	_tRemoteShareUser *pUser=FindUser(username);
	if (pUser==NULL)
		return false;

	return ((pUser->Username==username)&&(pUser->Password==password));
}

void CTCPServerIntBase::DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand)
{
	m_pRoot->DoDecodeMessage(pClient,pRXCommand);
}

void CTCPServerInt::stopClient(CTCPClient_ptr c)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	connections_.erase(c);
	c->stop();
}

void CTCPServerIntBase::stopAllClients()
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	if (connections_.empty())
		return;
	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt=connections_.begin(); itt!=connections_.end(); ++itt)
	{
		CTCPClientBase *pClient=itt->get();
		if (pClient)
			pClient->stop();
	}
	connections_.clear();
}

std::vector<_tRemoteShareUser> CTCPServerIntBase::GetRemoteUsers()
{
	return m_users;
}

void CTCPServerIntBase::SetRemoteUsers(const std::vector<_tRemoteShareUser> &users)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	m_users=users;
}

unsigned int CTCPServerIntBase::GetUserDevicesCount(const std::string &username)
{
	_tRemoteShareUser *pUser=FindUser(username);
	if (pUser==NULL)
		return 0;
	return (unsigned int) pUser->Devices.size();
}

void CTCPServerIntBase::SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore)
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
		CTCPClientBase *pClient=itt->get();
		if (pClient==pClient2Ignore)
			continue;

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

CTCPServerInt::CTCPServerInt(const std::string& address, const std::string& port, CTCPServer *pRoot) :
	CTCPServerIntBase(pRoot),
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

	new_connection_ = boost::shared_ptr<CTCPClient>(new CTCPClient(io_service_, this));

	acceptor_.async_accept(
		*(new_connection_->socket()),
		boost::bind(&CTCPServerInt::handleAccept, this,
			boost::asio::placeholders::error));
}

CTCPServerInt::~CTCPServerInt(void)
{

}

#ifndef NOCLOUD
// our proxied server
CTCPServerProxied::CTCPServerProxied(CTCPServer *pRoot, http::server::CProxyClient *proxy) : CTCPServerIntBase(pRoot)
{
	m_pProxyClient = proxy;
}

CTCPServerProxied::~CTCPServerProxied(void)
{
}

void CTCPServerProxied::start()
{
}

void CTCPServerProxied::stop()
{
	stopAllClients();
}

/// Stop the specified connection.
void CTCPServerProxied::stopClient(CTCPClient_ptr c)
{
	boost::lock_guard<boost::mutex> l(connectionMutex);
	c->stop();
	connections_.erase(c);
}

bool CTCPServerProxied::OnDisconnect(const std::string &token)
{
	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt = connections_.begin(); itt != connections_.end(); ++itt) {
		CSharedClient *pClient = dynamic_cast<CSharedClient *>(itt->get());
		if (pClient && pClient->CompareToken(token)) {
			pClient->stop();
			connections_.erase(itt);
			return true;
		}
	}
	return false;
}

bool CTCPServerProxied::OnNewConnection(const std::string &token, const std::string &username, const std::string &password)
{
	CSharedClient *new_client = new CSharedClient(this, m_pProxyClient, token, username);
	CTCPClient_ptr new_connection_ = boost::shared_ptr<CSharedClient>(new_client);
	if (!HandleAuthentication(new_connection_, username, password)) {
		new_connection_.reset(); // deletes new_client
		return false;
	}
	_log.Log(LOG_STATUS, "Incoming Domoticz connection via Proxy accepted for user %s.", username.c_str());
	connections_.insert(new_connection_);
	new_connection_->start();
	new_connection_.reset(); // invalidate dangling pointer
	return true;
}

bool CTCPServerProxied::OnIncomingData(const std::string &token, const unsigned char *data, size_t bytes_transferred)
{
	CSharedClient *client = FindClient(token);
	if (client == NULL) {
		return false;
	}
	client->OnIncomingData(data, bytes_transferred);
	return true;
}

CSharedClient *CTCPServerProxied::FindClient(const std::string &token)
{
	std::set<CTCPClient_ptr>::const_iterator itt;
	for (itt = connections_.begin(); itt != connections_.end(); ++itt) {
		CSharedClient *pClient = dynamic_cast<CSharedClient *>(itt->get());
		if (pClient && pClient->CompareToken(token)) {
			return pClient;
		}
	}
	return NULL;
}
#endif

//Out main (wrapper) server
CTCPServer::CTCPServer()
{
	m_pTCPServer = NULL;
#ifndef NOCLOUD
	m_pProxyServer = NULL;
#endif
}

CTCPServer::CTCPServer(const int ID)
{
	m_pTCPServer = NULL;
#ifndef NOCLOUD
	m_pProxyServer = NULL;
#endif
}

CTCPServer::~CTCPServer()
{
	StopServer();
#ifndef NOCLOUD
	if (m_pProxyServer != NULL) {
		m_pProxyServer->stop();
		delete m_pProxyServer;
		m_pProxyServer = NULL;
	}
#endif
}

bool CTCPServer::StartServer(const std::string &address, const std::string &port)
{
	int tries = 0;
	bool exception = false;
	std::string listen_address = address;

	do {
		try
		{
			exception = false;
			StopServer();
			if (m_pTCPServer != NULL) {
				_log.Log(LOG_ERROR, "Stopping TCPServer should delete resources !");
			}
			m_pTCPServer = new CTCPServerInt(listen_address, port, this);
		}
		catch (std::exception& e)
		{
			exception = true;
			switch (tries) {
			case 0:
				listen_address = "::";
				break;
			case 1:
				listen_address = "0.0.0.0";
				break;
			case 2:
				_log.Log(LOG_ERROR, "Exception starting shared server: %s", e.what());
				return false;
			}
			tries++;
		}
	} while (exception);
	_log.Log(LOG_NORM, "Starting shared server on: %s:%s", listen_address.c_str(), port.c_str());
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CTCPServer::Do_Work, this)));

	return (m_thread!=NULL);
}

#ifndef NOCLOUD
bool CTCPServer::StartServer(http::server::CProxyClient *proxy)
{
	_log.Log(LOG_NORM, "Accepting shared server connections via MyDomotiz (see settings menu).");
	m_pProxyServer = new CTCPServerProxied(this, proxy);
	// we load the remote users at this point, because this server was not started yet when
	// LoadSharedUsers() was called at startup.
	if (m_pTCPServer) {
		m_pProxyServer->SetRemoteUsers(m_pTCPServer->GetRemoteUsers());
	}
	else {
		m_mainworker.LoadSharedUsers();
	}
	return true;
}
#endif

void CTCPServer::StopServer()
{
	boost::lock_guard<boost::mutex> l(m_server_mutex);
	if (m_pTCPServer) {
		m_pTCPServer->stop();
	}
	if (m_thread) {
		m_thread->join();
	}
	// This is the only time to delete it
	if (m_pTCPServer) {
		delete m_pTCPServer;
		m_pTCPServer = NULL;
		_log.Log(LOG_STATUS, "TCPServer: shared server stopped");
	}
#ifndef NOCLOUD
	if (m_pProxyServer) {
		m_pProxyServer->stop();
	}
#endif
}

void CTCPServer::Do_Work()
{
	if (m_pTCPServer) {
		_log.Log(LOG_STATUS, "TCPServer: shared server started...");
		m_pTCPServer->start();
	}
}

void CTCPServer::SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore)
{
	boost::lock_guard<boost::mutex> l(m_server_mutex);
	if (m_pTCPServer)
		m_pTCPServer->SendToAll(DeviceRowID, pData, Length, pClient2Ignore);
#ifndef NOCLOUD
	if (m_pProxyServer)
		m_pProxyServer->SendToAll(DeviceRowID, pData, Length, pClient2Ignore);
#endif
}

void CTCPServer::SetRemoteUsers(const std::vector<_tRemoteShareUser> &users)
{
	boost::lock_guard<boost::mutex> l(m_server_mutex);
	if (m_pTCPServer)
		m_pTCPServer->SetRemoteUsers(users);
#ifndef NOCLOUD
	if (m_pProxyServer)
		m_pProxyServer->SetRemoteUsers(users);
#endif
}

unsigned int CTCPServer::GetUserDevicesCount(const std::string &username)
{
	boost::lock_guard<boost::mutex> l(m_server_mutex);
	if (m_pTCPServer) {
		return m_pTCPServer->GetUserDevicesCount(username);
	}
#ifndef NOCLOUD
	else if (m_pProxyServer) {
		return m_pProxyServer->GetUserDevicesCount(username);
	}
#endif
	return 0;
}

void CTCPServer::stopAllClients()
{
	if (m_pTCPServer)
		m_pTCPServer->stopAllClients();
#ifndef NOCLOUD
	if (m_pProxyServer)
		m_pProxyServer->stopAllClients();
#endif
}

void CTCPServer::DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand)
{
	HwdType = HTYPE_Domoticz;
	m_HwdID=8765;
	Name="DomoticzFromMaster";
	m_SeqNr=1;
	m_pUserData=(void*)pClient;
	sDecodeRXMessage(this, pRXCommand, NULL, -1);
}

#ifndef NOCLOUD
CTCPServerProxied *CTCPServer::GetProxiedServer()
{
	return m_pProxyServer;
}
#endif

} // namespace server
} // namespace tcp
