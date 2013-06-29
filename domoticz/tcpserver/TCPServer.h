#pragma once

#include "../main/RFXNames.h"
#include "../hardware/DomoticzHardware.h"
#include "TCPClient.h"
#include <set>
#include <vector>

namespace tcp {
namespace server {

class CTCPServer;

class CTCPServerInt
{
public:
	struct _tRemoteShareUser
	{
		std::string Username;
		std::string Password;
		std::vector<unsigned long long> Devices;
	};
	CTCPServerInt(const std::string& address, const std::string& port, CTCPServer *pRoot);
	~CTCPServerInt(void);

	void start();
	void stop();
	void stopAllClients();

	void SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length, const void* pClient2Ignore);

	void SetRemoteUsers(const std::vector<_tRemoteShareUser> users);
	unsigned int GetUserDevicesCount(const std::string username);
private:
	/// Stop the specified connection.
	void stopClient(CTCPClient_ptr c);

	_tRemoteShareUser* FindUser(const std::string username);

	void handleAccept(const boost::system::error_code& error);

	bool HandleAuthentication(CTCPClient_ptr c, const std::string username, const std::string password);
	void DoDecodeMessage(const unsigned char *pRXCommand);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	boost::asio::ip::tcp::acceptor acceptor_;

	std::set<CTCPClient_ptr> connections_;
	CTCPClient_ptr new_connection_;
	boost::mutex connectionMutex;

	std::vector<_tRemoteShareUser> m_users;
	CTCPServer *m_pRoot;

	friend class CTCPClient;
};

class CTCPServer : public CDomoticzHardwareBase
{
public:
	CTCPServer();
	CTCPServer(const int ID);
	~CTCPServer(void);

	bool StartServer(const std::string address, const std::string port);
	void StopServer();
	void SendToAll(const unsigned long long DeviceRowID, const char *pData, size_t Length, const void* pClient2Ignore);
	void SetRemoteUsers(const std::vector<CTCPServerInt::_tRemoteShareUser> users);
	unsigned int GetUserDevicesCount(const std::string username);
	void stopAllClients();
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand)> sDecodeRXMessage;
	void WriteToHardware(const char *pdata, const unsigned char length) {};
	void DoDecodeMessage(const unsigned char *pRXCommand);

private:
	CTCPServerInt *m_pTCPServer;
	boost::shared_ptr<boost::thread> m_thread;
	bool StartHardware() { return false; };
	bool StopHardware() { return false; };

	void Do_Work();
};

} // namespace server
} // namespace tcp
