#pragma once
#ifndef NOCLOUD
#include <queue>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include "proxycereal.hpp"
#include "cWebem.h"
#include "../hardware/ASyncTCP.h"

namespace tcp {
	namespace server {
		class CTCPServerProxied;
		class CTCPServer;
	}
}

class DomoticzTCP;

namespace http {
	namespace server {

#define PDUFUNCTION(type) void CProxyClient::OnPduReceived(std::shared_ptr<ProxyPdu_##type> pdu)

		class CProxyClient : ASyncTCP {
		public:
			CProxyClient();
			~CProxyClient();
			void Reset();
			void WriteMasterData(const std::string &token, const char *pData, size_t Length);
			void WriteSlaveData(const std::string &token, const char *pData, size_t Length);
			bool SharedServerAllowed();
			void ConnectToDomoticz(std::string instancekey, std::string username, std::string password, DomoticzTCP *master, int protocol_version);
			void DisconnectFromDomoticz(const std::string &token, DomoticzTCP *master);
			void SetSharedServer(tcp::server::CTCPServerProxied *domserv);
			void Connect(http::server::cWebem *webEm);
			void Disconnect();
			bool Enabled();
		private:
			void WebsocketGetRequest();
			bool parse_response(const char *data, size_t len);
			std::string websocket_key;
			std::string compute_accept_header(const std::string &websocket_key);
			std::string readbuf;
			void MyWrite(const std::string &msg);
			void WS_Write(unsigned long long handlerid, const std::string &packet_data);
			void LoginToService();

			void GetRequest(const std::string &originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_);
			void SendServDisconnect(const std::string &token, int reason);

			void PduHandler(const CProxyPduBase *pdu);
			/* Algorithm execution class */
#define PDUSTRING(name)
#define PDULONG(name)
#define PROXYPDU(name, members) void OnPduReceived(std::shared_ptr<ProxyPdu_##name> pdu);
#include "proxydef.def"
#undef PDUSTRING
#undef PDULONG
#undef PROXYPDU

			int _allowed_subsystems;
			std::string GetResponseHeaders(const http::server::reply &reply_);
			std::string _apikey;
			std::string _password;
			http::server::cWebem *m_pWebEm;
			tcp::server::CTCPServerProxied *m_pDomServ;
			enum status {
				status_httpmode,
				status_connected
			} connection_status;
			std::map<unsigned long long, CWebsocketHandler *> websocket_handlers;
		protected:
			virtual void OnConnect();
			virtual void OnData(const unsigned char *pData, size_t length);
			virtual void OnDisconnect();
			virtual void OnError(const std::exception e) {}; // todo
			virtual void OnError(const boost::system::error_code& error);
		};

		class CProxyManager {
		public:
			CProxyManager();
			~CProxyManager();
			bool Start(http::server::cWebem *webEm, tcp::server::CTCPServer *domServ);
			void Stop();
			void SetWebRoot(const std::string& doc_root);
			CProxyClient *GetProxyForMaster(DomoticzTCP *master);
		private:
			CProxyClient proxyclient;
			std::string m_pDocRoot;
			tcp::server::CTCPServer *m_pDomServ;
		};

		class CProxySharedData {
		public:
			CProxySharedData() {};
			void SetInstanceId(const std::string &instanceid);
			std::string GetInstanceId();
			bool AddConnectedIp(std::string ip);
			bool AddConnectedServer(std::string ip);
			void AddTCPClient(DomoticzTCP *master);
			void RemoveTCPClient(DomoticzTCP *master);
			void RestartTCPClients();
			void StopTCPClients();
			DomoticzTCP *findSlaveConnection(const std::string &token);
			DomoticzTCP *findSlaveById(const std::string &instanceid);
		private:
			std::string _instanceid;
			std::set<std::string> connectedips_;
			std::set<std::string> connectedservers_;
			std::vector<DomoticzTCP *>TCPClients;
		};

	}
}
#endif
