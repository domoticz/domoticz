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

namespace http {
	namespace server {

		class CProxyClient {
		public:
			CProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, http::server::cWebem *webEm);
			~CProxyClient();

			void Reconnect();

			void handle_connect(const boost::system::error_code& error,
				boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

			void handle_handshake(const boost::system::error_code& error);

			void handle_write(const boost::system::error_code& error,
				size_t bytes_transferred);

			void handle_read(const boost::system::error_code& error,
				size_t bytes_transferred);

			void ReadMore();

			void MyWrite(pdu_type type, CValueLengthPart *parameters);
			void LoginToService();

			void HandleRequest(ProxyPdu *pdu);
			void GetRequest(const std::string originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_);
			void HandleAssignkey(ProxyPdu *pdu);
			void HandleEnquire(ProxyPdu *pdu);
			void HandleAuthresp(ProxyPdu *pdu);

			void Stop();

		private:
			std::string GetResponseHeaders(const http::server::reply &reply_);
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _socket;
			std::string _apikey;
			std::string _password;
			boost::asio::streambuf _readbuf;
			std::vector<boost::asio::const_buffer> _writebuf;
			boost::asio::io_service& _io_service;
			bool doStop;
			http::server::cWebem *m_pWebEm;
			ProxyPdu *writePdu;
		};

		class CProxyManager {
		public:
			CProxyManager(const std::string& doc_root, http::server::cWebem *webEm);
			~CProxyManager();
			int Start();
			void Stop();
		private:
			void StartThread();
			boost::asio::io_service io_service;
			CProxyClient *proxyclient;
			boost::thread* m_thread;
			http::server::cWebem *m_pWebEm;
			boost::mutex end_mutex;
		};

	}
}