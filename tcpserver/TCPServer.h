#pragma once

#include "../hardware/DomoticzHardware.h"
#include "TCPClient.h"
#include <set>

namespace tcp {
namespace server {

class CTCPServer;

struct _tRemoteShareUser
{
	std::string Username;
	std::string Password;
	std::vector<uint64_t> Devices;
};

class CTCPServerIntBase
{
public:
	explicit CTCPServerIntBase(CTCPServer *pRoot);
	~CTCPServerIntBase() = default;

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void stopClient(CTCPClient_ptr c) = 0;
	virtual void stopAllClients();

	std::string AssambleDeviceInfo(int HardwareID, uint64_t DeviceRowID);
	void SendToAll(int HardwareID, uint64_t DeviceRowID, const CTCPClientBase *pClient2Ignore);
	void SendToClient(CTCPClientBase* pClient, std::string &szData);

	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	std::vector<_tRemoteShareUser> GetRemoteUsers();
	unsigned int GetUserDevicesCount(const std::string &username);
	_tRemoteShareUser* FindUser(const std::string& username);
protected:
	struct _tTCPLogInfo
	{
		time_t		time;
		std::string string;
	};

	bool HandleAuthentication(const CTCPClient_ptr &c, const std::string &username, const std::string &password);
	void DoDecodeMessage(const CTCPClientBase *pClient, const uint8_t *pData, size_t len);

	std::vector<_tRemoteShareUser> m_users;
	CTCPServer *m_pRoot;

	std::set<CTCPClient_ptr> connections_;
	std::mutex connectionMutex;

	friend class CTCPClient;
	friend class CSharedClient;
};

class CTCPServerInt : public CTCPServerIntBase {
public:
	CTCPServerInt(const std::string& address, const std::string& port, CTCPServer *pRoot);
	~CTCPServerInt() = default;
	void start() override;
	void stop() override;
	/// Stop the specified connection.
	void stopClient(CTCPClient_ptr c) override;

private:
	void handleAccept(const boost::system::error_code& error);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	boost::asio::ip::tcp::acceptor acceptor_;

	CTCPClient_ptr new_connection_;
};

class CTCPServer : public CDomoticzHardwareBase
{
public:
	CTCPServer();
	explicit CTCPServer(int ID);
	~CTCPServer() override;

	bool StartServer(const std::string &address, const std::string &port);
	void StopServer();
	void SendToAll(int HardwareID, uint64_t DeviceRowID, const char *pData, size_t Length, const CTCPClientBase *pClient2Ignore);
	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	unsigned int GetUserDevicesCount(const std::string &username);
	void stopAllClients();
	bool WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/) override
	{
		return true;
	};
	void DoDecodeMessage(const CTCPClientBase *pClient, const uint8_t* pData, size_t len);
private:
	std::mutex m_server_mutex;
	CTCPServerInt *m_pTCPServer;
	std::shared_ptr<std::thread> m_thread;
	bool StartHardware() override
	{
		return false;
	};
	bool StopHardware() override
	{
		return false;
	};

	void Do_Work();
};

} // namespace server
} // namespace tcp
