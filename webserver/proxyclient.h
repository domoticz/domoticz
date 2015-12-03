#pragma once
#ifndef NOCLOUD
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
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

		class CProxyClient {
		public:
			CProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, http::server::cWebem *webEm);
			~CProxyClient();

			void Reconnect();
			void Stop();
			void WriteMasterData(const std::string &token, const char *pData, size_t Length);
			void WriteSlaveData(const std::string &token, const char *pData, size_t Length);
			bool SharedServerAllowed();
			void ConnectToDomoticz(std::string instancekey, std::string username, std::string password, DomoticzTCP *master, int version);
			void DisconnectFromDomoticz(const std::string &token, DomoticzTCP *master);
			void SetSharedServer(tcp::server::CTCPServerProxied *domserv);
		private:

			void handle_connect(const boost::system::error_code& error,
				boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

			void handle_handshake(const boost::system::error_code& error);

			void handle_write(const boost::system::error_code& error,
				size_t bytes_transferred);

			void handle_read(const boost::system::error_code& error,
				size_t bytes_transferred);

			void ReadMore();

			void MyWrite(pdu_type type, CValueLengthPart *parameters, bool single_write);
			void LoginToService();

			void HandleRequest(ProxyPdu *pdu);
			void GetRequest(const std::string originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_);
			void HandleAssignkey(ProxyPdu *pdu);
			void HandleEnquire(ProxyPdu *pdu);
			void HandleAuthresp(ProxyPdu *pdu);

			void HandleServConnect(ProxyPdu *pdu);
			void HandleServConnectResp(ProxyPdu *pdu);
			void HandleServDisconnect(ProxyPdu *pdu);
			void HandleServReceive(ProxyPdu *pdu);
			void HandleServSend(ProxyPdu *pdu);
			void HandleServRosterInd(ProxyPdu *pdu);
			void SendServDisconnect(const std::string &token, int reason);

			int _allowed_subsystems;
			std::string GetResponseHeaders(const http::server::reply &reply_);
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _socket;
			std::string _apikey;
			std::string _password;
			boost::asio::streambuf _readbuf;
			std::vector<boost::asio::const_buffer> _writebuf;
			boost::asio::io_service& _io_service;
			bool doStop;
			http::server::cWebem *m_pWebEm;
			tcp::server::CTCPServerProxied *m_pDomServ;
			ProxyPdu *writePdu;
			bool mSingleWrite;
			bool we_locked_prefs_mutex;
			/// read timeout timer
			boost::asio::deadline_timer timer_;
			void handle_timeout(const boost::system::error_code& error);
			/// timeouts (persistent and other) connections in seconds
			int timeout_;
			/// make sure we only write one packet at a time
			boost::mutex write_mutex;
			bool b_Connected;
		};

		class CProxyManager {
		public:
			CProxyManager(const std::string& doc_root, http::server::cWebem *webEm, tcp::server::CTCPServer *domServ);
			~CProxyManager();
			int Start(bool first);
			void Stop();
			CProxyClient *GetProxyForMaster(DomoticzTCP *master);
		private:
			void StartThread();
			boost::asio::io_service io_service;
			CProxyClient *proxyclient;
			boost::thread* m_thread;
			http::server::cWebem *m_pWebEm;
			tcp::server::CTCPServer *m_pDomServ;
			bool _first;
		};

		class CProxySharedData {
		public:
			CProxySharedData() : _instanceid("") {};
			void SetInstanceId(std::string instanceid);
			std::string GetInstanceId();
			void LockPrefsMutex();
			void UnlockPrefsMutex();
			bool AddConnectedIp(std::string ip);
			bool AddConnectedServer(std::string ip);
			void AddTCPClient(DomoticzTCP *master);
			void RemoveTCPClient(DomoticzTCP *master);
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
