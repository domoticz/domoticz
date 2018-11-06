#pragma once
#ifndef NOCLOUD
#include <queue>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include "proxycommon.h"
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

#define ONPDU(type) case type: Handle##type(#type, part); break;
#define PDUPROTO(type) virtual void Handle##type(const char *pduname, CValueLengthPart &part);
#define PDUFUNCTION(type) void CProxyClient::Handle##type(const char *pduname, CValueLengthPart &part)

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
		private:
			void WebsocketGetRequest();
			bool parse_response(const char *data, size_t len);
			std::string websocket_key;
			std::string compute_accept_header(const std::string &websocket_key);
			std::string readbuf;
			void MyWrite(pdu_type type, CValueLengthPart &parameters);
			void WS_Write(long handlerid, const std::string &packet_data);
			void LoginToService();

			PDUPROTO(PDU_REQUEST)
			PDUPROTO(PDU_ASSIGNKEY)
			PDUPROTO(PDU_ENQUIRE)
			PDUPROTO(PDU_AUTHRESP)
			PDUPROTO(PDU_SERV_CONNECT)
			PDUPROTO(PDU_SERV_DISCONNECT)
			PDUPROTO(PDU_SERV_CONNECTRESP)
			PDUPROTO(PDU_SERV_RECEIVE)
			PDUPROTO(PDU_SERV_SEND)
			PDUPROTO(PDU_SERV_ROSTERIND)
			PDUPROTO(PDU_WS_OPEN)
			PDUPROTO(PDU_WS_CLOSE)
			PDUPROTO(PDU_WS_RECEIVE)

			void GetRequest(const std::string &originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_);
			void SendServDisconnect(const std::string &token, int reason);

			void PduHandler(ProxyPdu &pdu);

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
			std::map<long, CWebsocketHandler *> websocket_handlers;
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
			int Start(http::server::cWebem *webEm, tcp::server::CTCPServer *domServ);
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
