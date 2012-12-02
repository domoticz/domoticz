#pragma once

#include "TCPClient.h"
#include <set>

namespace tcp {
namespace server {

class CTCPServerInt
{
public:
	CTCPServerInt(const std::string& address, const std::string& port, const std::string username, const std::string password, const int rights);
	~CTCPServerInt(void);

	void start();
	void stop();

	void SendToAll(const char *pData, size_t Length);

private:
	/// Stop the specified connection.
	void stopClient(CTCPClient_ptr c);
	void stopAllClient();

	void handleAccept(const boost::system::error_code& error);

	bool HandleAuthentication(CTCPClient_ptr c, const std::string username, const std::string password);

	/// Handle a request to stop the server.
	void handle_stop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	boost::asio::ip::tcp::acceptor acceptor_;

	std::set<CTCPClient_ptr> connections_;
	CTCPClient_ptr new_connection_;
	boost::mutex connectionMutex;

	std::string m_username;
	std::string m_password;
	int m_rights;

	friend class CTCPClient;
};

class CTCPServer
{
public:
	CTCPServer();
	~CTCPServer(void);

	bool StartServer(const std::string address, const std::string port, const std::string username, const std::string password, const int rights);
	void StopServer();
	void SendToAll(const char *pData, size_t Length);
private:
	CTCPServerInt *m_pTCPServer;
	boost::shared_ptr<boost::thread> m_thread;

	void Do_Work();
};

} // namespace server
} // namespace tcp
