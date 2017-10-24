#pragma once
#ifndef NOCLOUD
#include <iosfwd>
#include <queue>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <list>
#include <boost/thread/mutex.hpp>
#include "proxycommon.h"
#include "cWebem.h"
#include "request_handler.hpp"
// todo: do we need this?
#include "connection_manager.hpp"
#include "../main/Logger.h"
#include "../hardware/DomoticzTCP.h"

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

		class CProxyClient : public boost::enable_shared_from_this<CProxyClient> {
		public:
			CProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, http::server::cWebem *webEm);
			~CProxyClient();

			void Reconnect();
			void ContinueConnect(const boost::system::error_code& error);
			void Stop();
			void WriteMasterData(const std::string &token, const char *pData, size_t Length);
			void WriteSlaveData(const std::string &token, const char *pData, size_t Length);
			bool SharedServerAllowed();
			void ConnectToDomoticz(std::string instancekey, std::string username, std::string password, DomoticzTCP *master, int protocol_version);
			void DisconnectFromDomoticz(const std::string &token, DomoticzTCP *master);
			void SetSharedServer(tcp::server::CTCPServerProxied *domserv);
		private:

			void handle_connect(const boost::system::error_code& error,
				boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

			void handle_handshake(const boost::system::error_code& error);

			void handle_read(const boost::system::error_code& error,
				size_t bytes_transferred);
			void handle_write(const boost::system::error_code& error,
				size_t bytes_transferred);

			void ReadMore();

			void MyWrite(pdu_type type, CValueLengthPart &parameters);
			void SocketWrite(ProxyPdu *pdu);
			std::vector<boost::asio::const_buffer> _writebuf;
			ProxyPdu *writePdu;
			/// make sure we only write one packet at a time
			boost::mutex writeMutex;
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
			void GetRequest(const std::string &originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_);
			void SendServDisconnect(const std::string &token, int reason);

			void PduHandler(ProxyPdu &pdu);

			int _allowed_subsystems;
			std::string GetResponseHeaders(const http::server::reply &reply_);
			boost::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > _socket;
			std::string _apikey;
			std::string _password;
			boost::asio::streambuf _readbuf;
			boost::asio::io_service& _io_service;
			boost::asio::ssl::context& _context;
			bool doStop;
			http::server::cWebem *m_pWebEm;
			tcp::server::CTCPServerProxied *m_pDomServ;
			bool we_locked_prefs_mutex;
			/// read timeout timer
			boost::asio::deadline_timer timer_;
			void handle_timeout(const boost::system::error_code& error);
			/// timeouts (persistent and other) connections in seconds
			int timeout_;
			bool b_Connected;

			// is protected by writeMutex
			std::queue<ProxyPdu *> writeQ;
		};

		class CProxyManager : public boost::enable_shared_from_this<CProxyManager> {
		public:
			CProxyManager(const std::string& doc_root, http::server::cWebem *webEm, tcp::server::CTCPServer *domServ);
			~CProxyManager();
			int Start(bool first);
			void Stop();
			boost::shared_ptr<CProxyClient> GetProxyForMaster(DomoticzTCP *master);
		private:
			void StartThread();
			boost::asio::io_service io_service;
			boost::shared_ptr<CProxyClient> proxyclient;
			boost::thread* m_thread;
			std::string m_pDocRoot;
			http::server::cWebem *m_pWebEm;
			tcp::server::CTCPServer *m_pDomServ;
			bool _first;
		};

		class CProxySharedData {
		public:
			CProxySharedData() {};
			void SetInstanceId(std::string instanceid);
			std::string GetInstanceId();
			void LockPrefsMutex();
			void UnlockPrefsMutex();
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
			boost::mutex prefs_mutex;
			std::set<std::string> connectedips_;
			std::set<std::string> connectedservers_;
			std::vector<DomoticzTCP *>TCPClients;
		};

	}
}
#endif
