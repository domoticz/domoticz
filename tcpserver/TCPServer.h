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

#define RemoteMessage_id_Low 0xE2
#define RemoteMessage_id_High 0x2E
#define SECONDS_PER_DAY 60*60*24

struct _tRemoteMessage
{
	uint8_t ID_Low;
	uint8_t ID_High;
	int		Original_Hardware_ID;
	//data
};

class CTCPServerIntBase
{
public:
	explicit CTCPServerIntBase(CTCPServer *pRoot);
	~CTCPServerIntBase(void);

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void stopClient(CTCPClient_ptr c) = 0;
	virtual void stopAllClients();

	void SendToAll(const int HardwareID, const uint64_t DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore);

	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	std::vector<_tRemoteShareUser> GetRemoteUsers();
	unsigned int GetUserDevicesCount(const std::string &username);
protected:
	struct _tTCPLogInfo
	{
		time_t		time;
		std::string string;
	};

	_tRemoteShareUser* FindUser(const std::string &username);

	bool HandleAuthentication(CTCPClient_ptr c, const std::string &username, const std::string &password);
	void DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand);

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
	~CTCPServerInt(void);
	virtual void start() override;
	virtual void stop() override;
	/// Stop the specified connection.
	virtual void stopClient(CTCPClient_ptr c) override;
private:

	void handleAccept(const boost::system::error_code& error);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	boost::asio::ip::tcp::acceptor acceptor_;

	CTCPClient_ptr new_connection_;

	bool IsUserHereFirstTime(const std::string &ip_string);
	std::vector<_tTCPLogInfo> m_incoming_domoticz_history;
};

#ifndef NOCLOUD
class CTCPServerProxied : public CTCPServerIntBase {
public:
	CTCPServerProxied(CTCPServer *pRoot, std::shared_ptr<http::server::CProxyClient> proxy);
	~CTCPServerProxied(void);
	virtual void start() override;
	virtual void stop() override;
	/// Stop the specified connection.
	virtual void stopClient(CTCPClient_ptr c) override;

	bool OnNewConnection(const std::string &token, const std::string &username, const std::string &password);
	bool OnDisconnect(const std::string &token);
	bool OnIncomingData(const std::string &token, const unsigned char *data, size_t bytes_transferred);
	CSharedClient *FindClient(const std::string &token);
private:
	std::shared_ptr<http::server::CProxyClient> m_pProxyClient;
};
#endif

class CTCPServer : public CDomoticzHardwareBase
{
public:
	CTCPServer();
	explicit CTCPServer(const int ID);
	~CTCPServer(void);

	bool StartServer(const std::string &address, const std::string &port);
#ifndef NOCLOUD
	bool StartServer(std::shared_ptr<http::server::CProxyClient> proxy);
#endif
	void StopServer();
	void SendToAll(const int HardwareID, const uint64_t DeviceRowID, const char *pData, size_t Length, const CTCPClientBase* pClient2Ignore);
	void SetRemoteUsers(const std::vector<_tRemoteShareUser> &users);
	unsigned int GetUserDevicesCount(const std::string &username);
	void stopAllClients();
	boost::signals2::signal<void(CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)> sDecodeRXMessage;
	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) { return true; };
	void DoDecodeMessage(const CTCPClientBase *pClient, const unsigned char *pRXCommand);
#ifndef NOCLOUD
	CTCPServerProxied *GetProxiedServer();
#endif
private:
	std::mutex m_server_mutex;
	CTCPServerInt *m_pTCPServer;
#ifndef NOCLOUD
	CTCPServerProxied *m_pProxyServer;
#endif

	std::shared_ptr<std::thread> m_thread;
	bool StartHardware() { return false; };
	bool StopHardware() { return false; };

	void Do_Work();
};

} // namespace server
} // namespace tcp
