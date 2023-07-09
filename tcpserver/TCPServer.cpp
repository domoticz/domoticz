#include "stdafx.h"
#include <iostream>
#include "TCPServer.h"
#include "TCPClient.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../hardware/DomoticzTCP.h"
#include "../main/mainworker.h"
#include <boost/asio.hpp>
#include <algorithm>

#define SECONDS_PER_DAY 60*60*24

namespace tcp {
namespace server {

CTCPServerInt::CTCPServerInt(const std::string& address, const std::string& port, CTCPServer* pRoot) :
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
	// bind to both ipv6 and ipv4 sockets for the "::" address only
	if (address == "::")
	{
		acceptor_.set_option(boost::asio::ip::v6_only(false));
	}
	acceptor_.bind(endpoint);
	acceptor_.listen();

	new_connection_ = std::make_shared<CTCPClient>(io_service_, this);
	if (new_connection_ == nullptr)
	{
		_log.Log(LOG_ERROR, "Error creating new client!");
		return;
	}

	acceptor_.async_accept(*(new_connection_->socket()), [this](auto&& err) { handleAccept(err); });
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
	io_service_.post([this] { handle_stop(); });
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
	if (error)
		return;
	std::lock_guard<std::mutex> l(connectionMutex);
	std::string s = new_connection_->socket()->remote_endpoint().address().to_string();

	if (s.substr(0, 7) == "::ffff:") {
		s = s.substr(7);
	}

	new_connection_->m_endpoint=s;

	_log.Log(LOG_STATUS, "Incoming Domoticz connection from: %s", s.c_str());

	connections_.insert(new_connection_);
	new_connection_->start();

	new_connection_.reset(new CTCPClient(io_service_, this));

	acceptor_.async_accept(*(new_connection_->socket()), [this](auto &&err) { handleAccept(err); });
}

void CTCPServerInt::stopClient(CTCPClient_ptr c)
{
	std::lock_guard<std::mutex> l(connectionMutex);
	connections_.erase(c);
	c->stop();
}

CTCPServerIntBase::CTCPServerIntBase(CTCPServer* pRoot)
{
	m_pRoot = pRoot;
}

_tRemoteShareUser* CTCPServerIntBase::FindUser(const std::string &username)
{
	int ii=0;
	for (const auto &user : m_users)
	{
		if (user.Username == username)
			return &m_users[ii];
		ii++;
	}
	return nullptr;
}

bool CTCPServerIntBase::HandleAuthentication(const CTCPClient_ptr &c, const std::string &username, const std::string &password)
{
	_tRemoteShareUser *pUser=FindUser(username);
	if (pUser == nullptr)
		return false;

	return (pUser->Password == password);
}

void CTCPServerIntBase::DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand)
{
	m_pRoot->DoDecodeMessage(pClient,pRXCommand);
}

void CTCPServerIntBase::stopAllClients()
{
	std::lock_guard<std::mutex> l(connectionMutex);
	if (connections_.empty())
		return;
	for (const auto &c : connections_)
	{
		CTCPClientBase *pClient = c.get();
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
	std::lock_guard<std::mutex> l(connectionMutex);
	m_users=users;
}

unsigned int CTCPServerIntBase::GetUserDevicesCount(const std::string &username)
{
	_tRemoteShareUser *pUser=FindUser(username);
	if (pUser == nullptr)
		return 0;
	return (unsigned int) pUser->Devices.size();
}

void CTCPServerIntBase::SendToAll(const int /*HardwareID*/, const uint64_t DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore)
{
	std::lock_guard<std::mutex> l(connectionMutex);

	//do not share Interface Messages
	if (
		(pData[1]==pTypeInterfaceMessage)||
		(pData[1]==pTypeRecXmitMessage)
		)
		return;

	for (const auto &c : connections_)
	{
		CTCPClientBase *pClient = c.get();
		if (pClient==pClient2Ignore)
			continue;

		if (pClient)
		{
			_tRemoteShareUser *pUser=FindUser(pClient->m_username);
			if (pUser != nullptr)
			{
				//check if we are allowed to get this device
				bool bOk2Send=false;
				if (pUser->Devices.empty())
					bOk2Send=true;
				else
					bOk2Send = std::any_of(pUser->Devices.begin(), pUser->Devices.end(), [DeviceRowID](uint64_t d) { return d == DeviceRowID; });

				if (bOk2Send)
					pClient->write(pData,Length);
			}
		}
	}
}

//Out main (wrapper) server
CTCPServer::CTCPServer()
{
	m_pTCPServer = nullptr;
}

CTCPServer::CTCPServer(const int /*ID*/)
{
	m_pTCPServer = nullptr;
}

CTCPServer::~CTCPServer()
{
	StopServer();
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
			if (m_pTCPServer != nullptr)
			{
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
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadName(m_thread->native_handle(), "TCPServer");
	return (m_thread != nullptr);
}

void CTCPServer::StopServer()
{
	std::lock_guard<std::mutex> l(m_server_mutex);
	if (m_pTCPServer) {
		m_pTCPServer->stop();
	}
	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}
	// This is the only time to delete it
	if (m_pTCPServer) {
		delete m_pTCPServer;
		m_pTCPServer = nullptr;
		_log.Log(LOG_STATUS, "TCPServer: shared server stopped");
	}
}

void CTCPServer::Do_Work()
{
	if (m_pTCPServer) {
		_log.Log(LOG_STATUS, "TCPServer: shared server started...");
		m_pTCPServer->start();
	}
}

void CTCPServer::SendToAll(const int HardwareID, const uint64_t DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore)
{
	std::lock_guard<std::mutex> l(m_server_mutex);
	if (m_pTCPServer)
		m_pTCPServer->SendToAll(HardwareID, DeviceRowID, pData, Length, pClient2Ignore);
}

void CTCPServer::SetRemoteUsers(const std::vector<_tRemoteShareUser> &users)
{
	std::lock_guard<std::mutex> l(m_server_mutex);
	if (m_pTCPServer)
		m_pTCPServer->SetRemoteUsers(users);
}

unsigned int CTCPServer::GetUserDevicesCount(const std::string &username)
{
	std::lock_guard<std::mutex> l(m_server_mutex);
	if (m_pTCPServer) {
		return m_pTCPServer->GetUserDevicesCount(username);
	}
	return 0;
}

void CTCPServer::stopAllClients()
{
	if (m_pTCPServer)
		m_pTCPServer->stopAllClients();
}

void CTCPServer::DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand)
{
	HwdType = HTYPE_Domoticz;
	m_HwdID=8765;
	m_Name="DomoticzFromMaster";
	m_SeqNr=1;
	m_pUserData=(void*)pClient;
	sDecodeRXMessage(this, pRXCommand, nullptr, -1, m_Name.c_str());
}

} // namespace server
} // namespace tcp
