#include "stdafx.h"
#ifndef NOCLOUD
#include "proxyclient.h"
#include "request.hpp"
#include "reply.hpp"
#include "request_parser.hpp"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "../tcpserver/TCPServer.h"
#include "sha1.hpp"

extern std::string szAppVersion;

#define TIMEOUT 60
#define ADDPDUSTRING(value) parameters.AddPart(value)
#define ADDPDUSTRINGBINARY(value) parameters.AddPart(value, false)
#define ADDPDULONG(value) parameters.AddLong(value)
#define ADDPDUBINARY(value, length) parameters.AddPart((void *)value, length)
#define GETPDUSTRING(value) std::string value; if (!part.GetNextPart(value)) { _log.Log(LOG_ERROR, "PROXY: Invalid request reading %s", pduname); return; }
#define GETPDUSTRINGBINARY(value) std::string value; if (!part.GetNextPart(value, false)) { _log.Log(LOG_ERROR, "PROXY: Invalid request reading %s", pduname); return; }
#define GETPDULONG(value) long value; if (!part.GetNextLong(value)) { _log.Log(LOG_ERROR, "PROXY: Invalid request reading %s", pduname); return; }
#define GETPDUBINARY(value, length) unsigned char *value; size_t length; if (!part.GetNextPart((void **)&value, &length)) { _log.Log(LOG_ERROR, "PROXY: Invalid request reading %s", pduname); return; }

namespace http {
	namespace server {

		CProxySharedData sharedData;

		CProxyClient::CProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, http::server::cWebem *webEm)
			: _context(context),
			_io_service(io_service),
			doStop(false),
			connection_status(status_connecting),
			we_locked_prefs_mutex(false),
			timeout_(TIMEOUT),
			timer_(io_service, boost::posix_time::seconds(TIMEOUT)),
			write_in_progress(false)
		{
			_allowed_subsystems = 0;
			m_sql.GetPreferencesVar("MyDomoticzUserId", _apikey);
			m_sql.GetPreferencesVar("MyDomoticzPassword", _password);
			m_sql.GetPreferencesVar("MyDomoticzSubsystems", _allowed_subsystems);
			if (_password != "") {
				_password = base64_decode(_password);
			}
			if (_apikey == "" || _password == "" || _allowed_subsystems == 0) {
				doStop = true;
				return;
			}
			m_pWebEm = webEm;
			m_pDomServ = NULL;
			_socket.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(_io_service, _context));
		}

		void CProxyClient::WriteSlaveData(const std::string &token, const char *pData, size_t Length)
		{
			/* data from slave to master */
			CValueLengthPart parameters;

			ADDPDUSTRING(token);
			ADDPDUBINARY(pData, Length);

			MyWrite(PDU_SERV_RECEIVE, parameters);
		}

		void CProxyClient::WriteMasterData(const std::string &token, const char *pData, size_t Length)
		{
			/* data from master to slave */
			CValueLengthPart parameters;

			ADDPDUSTRING(token);
			ADDPDUBINARY(pData, Length);

			MyWrite(PDU_SERV_SEND, parameters);
		}

		void CProxyClient::Reconnect()
		{
			if (we_locked_prefs_mutex) {
				// avoid deadlock if we got a read or write error in between handle_handshake() and HandleAuthresp()
				we_locked_prefs_mutex = false;
				sharedData.UnlockPrefsMutex();
			}
			if (doStop) {
				return;
			}
			connection_status = status_connecting;
			int wait = 1;
			if (_socket->lowest_layer().is_open()) {
				wait = 15;
				boost::system::error_code ec;
				_socket->lowest_layer().cancel(ec);
				_socket->lowest_layer().shutdown(boost::asio::socket_base::shutdown_both, ec);
				_socket->lowest_layer().close(ec);
			}
			timer_.expires_from_now(boost::posix_time::seconds(wait));
			timer_.async_wait(boost::bind(&CProxyClient::ContinueConnect, shared_from_this(), boost::asio::placeholders::error));
		}

		void CProxyClient::ContinueConnect(const boost::system::error_code& error)
		{
				std::string address = "domoproxy.domoticz.com";
				std::string port = "443";
				boost::system::error_code ec;

			if (error) {
				// timer was cancelled?
				return;
			}
			_socket.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(_io_service, _context));
			// set timeout timer
			timer_.expires_from_now(boost::posix_time::seconds(timeout_));
			timer_.async_wait(boost::bind(&CProxyClient::handle_timeout, shared_from_this(), boost::asio::placeholders::error));
			boost::asio::ip::tcp::resolver resolver(_io_service);
			boost::asio::ip::tcp::resolver::query query(address, port);
			boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query, ec);
			if (ec) {
				_log.Log(LOG_ERROR, "PROXY: Error resolving %s: %s", address.c_str(), ec.message().c_str());
				Reconnect();
				return;
			}
			boost::asio::ip::tcp::endpoint endpoint = *iterator;
			_socket->lowest_layer().async_connect(endpoint,
				boost::bind(&CProxyClient::handle_connect, shared_from_this(),
					boost::asio::placeholders::error, ++iterator));
		}

		void CProxyClient::handle_connect(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
		{
			if (doStop) {
				return;
			}
			if (!error)
			{
				_socket->async_handshake(boost::asio::ssl::stream_base::client,
					boost::bind(&CProxyClient::handle_handshake, shared_from_this(),
						boost::asio::placeholders::error));
			}
		else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
			{
				_socket->lowest_layer().close();
				boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
				_socket->lowest_layer().async_connect(endpoint,
					boost::bind(&CProxyClient::handle_connect, shared_from_this(),
						boost::asio::placeholders::error, ++endpoint_iterator));
			}
			else
			{
				if (!doStop) {
					_log.Log(LOG_ERROR, "PROXY: Connect failed, reconnecting: %s", error.message().c_str());
					Reconnect();
				}
			}
		}

		void CProxyClient::handle_timeout(const boost::system::error_code& error)
		{
			if (error != boost::asio::error::operation_aborted) {
				_log.Log(LOG_ERROR, "PROXY: timeout occurred, reconnecting");
				Reconnect();
			}
		}

		void CProxyClient::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
		{
			if (doStop) {
				return;
			}
			boost::unique_lock<boost::mutex>(writeMutex);
			if (bytes_transferred != SockWriteBuf.length()) {
				_log.Log(LOG_ERROR, "Only wrote %d of %d bytes.", bytes_transferred, SockWriteBuf.length());
			}
			SockWriteBuf.clear();
			ProxyPdu *pdu;
			switch (connection_status) {
			case status_connecting:
				// a write occurred meanwhile a reconnect has been issued
				break;
			case status_httpmode:
				// we now wait for the webserver response
				break;
			case status_connected:
				if (bytes_transferred < SockWriteBuf.length()) {
					_log.Log(LOG_ERROR, "PROXY: Only wrote %ld of %ld bytes.", bytes_transferred, SockWriteBuf.length());
				}
			if (error) {
				_log.Log(LOG_ERROR, "PROXY: Write failed, code = %d, %s", error.value(), error.message().c_str());
			}
				write_in_progress = false;
			if (!writeQ.empty()) {
					pdu = writeQ.front();
				writeQ.pop();
					SocketWrite(pdu);
			}
				break;
		}
		}

		void CProxyClient::SocketWrite(ProxyPdu *pdu)
		{
			if (connection_status == status_connecting) {
				return;
			}
			// do not call directly, use MyWrite()
			if (write_in_progress) {
				// something went wrong, this shouldnt happen
				_log.Log(LOG_ERROR, "write_in_progress error"); // debug
			}
			write_in_progress = true;
			CWebsocketFrame frame;
			SockWriteBuf = frame.Create(opcode_binary, std::string((char *)pdu->content(), pdu->length()), true);
			delete pdu;
			boost::asio::async_write(*_socket, boost::asio::buffer(SockWriteBuf), boost::bind(&CProxyClient::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}

		void CProxyClient::WS_Write(long requestid, const std::string &packet_data)
		{
			CValueLengthPart parameters;
			ADDPDUSTRINGBINARY(packet_data);
			ADDPDULONG(requestid);
			MyWrite(PDU_WS_SEND, parameters);
		}

		void CProxyClient::MyWrite(pdu_type type, CValueLengthPart &parameters)
		{
			boost::unique_lock<boost::mutex>(writeMutex);
			if (connection_status != status_connected) {
				return;
			}
			ProxyPdu *pdu = new ProxyPdu(type, &parameters);
			if (write_in_progress) {
				// write in progress, add to queue
				writeQ.push(pdu);
			}
			else {
				SocketWrite(pdu);
			}
		}

		void CProxyClient::LoginToService()
		{
			std::string instanceid = sharedData.GetInstanceId();
			const long protocol_version = 2;

			// send authenticate pdu
			CValueLengthPart parameters;

			ADDPDUSTRING(_apikey);
			ADDPDUSTRING(instanceid);
			ADDPDUSTRING(_password);
			ADDPDUSTRING(szAppVersion);
			ADDPDULONG(_allowed_subsystems);
			ADDPDULONG(protocol_version);

			MyWrite(PDU_AUTHENTICATE, parameters);
		}

		void CProxyClient::handle_handshake(const boost::system::error_code& error)
		{
			if (doStop) {
				return;
			}
			if (!error)
			{
				// lock until we have a valid api id
				sharedData.LockPrefsMutex();
				we_locked_prefs_mutex = true;
				connection_status = status_httpmode;
				WebsocketGetRequest();
			}
			else
			{
				_log.Log(LOG_ERROR, "PROXY: Handshake failed, reconnecting: %s", error.message().c_str());
				Reconnect();
			}
		}

		void CProxyClient::WebsocketGetRequest()
		{
			// start read thread
			ReadMore();
			// generate random websocket key
			unsigned char random[16];
			for (int i = 0; i < sizeof(random); i++) {
				random[i] = rand();
			}
			websocket_key = base64_encode(random, sizeof(random));
			SockWriteBuf = "GET /proxyrequest HTTP/1.1\r\nHost: my.domoticz.com\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nOrigin: Domoticz\r\nSec-Websocket-Version: 13\r\nSec-Websocket-Protocol: MyDomoticz\r\nSec-Websocket-Key: " + websocket_key + "\r\n\r\n";
			boost::asio::async_write(*_socket, boost::asio::buffer(SockWriteBuf), boost::bind(&CProxyClient::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}

		void CProxyClient::ReadMore()
		{
			// read chunks of max 4 KB
			boost::asio::streambuf::mutable_buffers_type buf = _readbuf.prepare(4096);

			// set timeout timer
			timer_.expires_from_now(boost::posix_time::seconds(timeout_));
			timer_.async_wait(boost::bind(&CProxyClient::handle_timeout, shared_from_this(), boost::asio::placeholders::error));

			_socket->async_read_some(buf,
				boost::bind(&CProxyClient::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
				);
		}

		void CProxyClient::GetRequest(const std::string &originatingip, boost::asio::mutable_buffers_1 _buf, http::server::reply &reply_)
		{
			/// The parser for the incoming request.
			http::server::request_parser request_parser_;
			http::server::request request_;

			boost::tribool result = boost::indeterminate;
			try
			{
				size_t bufsize = boost::asio::buffer_size(_buf);
				const char *begin = boost::asio::buffer_cast<const char*>(_buf);
				boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
					request_, begin, begin + bufsize);
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "PROXY: Exception during request parsing");
			}

			if (result)
			{
				request_.host_address = originatingip;
				m_pWebEm->myRequestHandler.handle_request(request_, reply_);
			}
			else if (!result)
			{
				reply_ = http::server::reply::stock_reply(http::server::reply::bad_request);
			}
			else
			{
				_log.Log(LOG_ERROR, "PROXY: Could not parse request.");
			}
		}

		std::string CProxyClient::GetResponseHeaders(const http::server::reply &reply_)
		{
			std::string result = "";
			for (std::size_t i = 0; i < reply_.headers.size(); ++i) {
				result += reply_.headers[i].name;
				result += ": ";
				result += reply_.headers[i].value;
				result += "\r\n";
			}
			return result;
		}

		PDUFUNCTION(PDU_REQUEST)
		{
			if (!(_allowed_subsystems & SUBSYSTEM_HTTP)) {
				_log.Log(LOG_ERROR, "PROXY: HTTP access disallowed, denying request.");
				return;
			}
			// response variables
			boost::asio::mutable_buffers_1 _buf(NULL, 0);
			/// The reply to be sent back to the client.
			http::server::reply reply_;

			std::string request, responseheaders;
			CValueLengthPart parameters; // response parameters

			GETPDUSTRING(originatingip);
			GETPDULONG(subsystem);

			if (subsystem == SUBSYSTEM_HTTP) {
				// "normal web request", get parameters
				GETPDUSTRING(requesturl);
				GETPDUSTRING(requestheaders);
				GETPDUSTRINGBINARY(requestbody);
				// we number the request, because we can send back asynchronously
				GETPDULONG(requestid);

				if (requestbody.size() > 0) {
					request = "POST ";
				}
				else {
					request = "GET ";
				}
				request += requesturl;
				request += " HTTP/1.1\r\n";
				request += requestheaders;
				request += "\r\n";
				request += requestbody;

				_buf = boost::asio::buffer((void *)request.c_str(), request.length());

				sharedData.AddConnectedIp(originatingip);

				GetRequest(originatingip, _buf, reply_);
				// assemble response
				responseheaders = GetResponseHeaders(reply_);

				ADDPDULONG(reply_.status);
				ADDPDUSTRING(responseheaders);
				ADDPDUSTRINGBINARY(reply_.content);
				ADDPDULONG(requestid);

				// send response to proxy
				MyWrite(PDU_RESPONSE, parameters);
			}
			else {
				// unknown subsystem
				_log.Log(LOG_ERROR, "PROXY: Got Request pdu for unknown subsystem %d.", subsystem);
			}
		}

		PDUFUNCTION(PDU_ASSIGNKEY)
		{
			// get our new api key
			GETPDUSTRING(newapi);
			_log.Log(LOG_STATUS, "PROXY: We were assigned an instance id: %s.\n", newapi.c_str());
			sharedData.SetInstanceId(newapi);
			// re-login with the new instance id
			LoginToService();
		}

		PDUFUNCTION(PDU_ENQUIRE)
		{
			// assemble response
			CValueLengthPart parameters;

			// send response to proxy
			MyWrite(PDU_ENQUIRE, parameters);
		}

		PDUFUNCTION(PDU_SERV_ROSTERIND)
		{
			GETPDUSTRING(c_slave);

			_log.Log(LOG_STATUS, "PROXY: Notification received: slave %s online now.", c_slave.c_str());
			DomoticzTCP *slave = sharedData.findSlaveById(c_slave);
			if (slave) {
				slave->SetConnected(true);
			}
		}

		PDUFUNCTION(PDU_SERV_DISCONNECT)
		{
			bool success;

			GETPDUSTRING(tokenparam);
			GETPDULONG(reason);
			// see if we are slave
			success = m_pDomServ->OnDisconnect(tokenparam);
			if (success) {
				_log.Log(LOG_STATUS, "PROXY: Master disconnected");
			}
			// see if we are master
			DomoticzTCP *slave = sharedData.findSlaveConnection(tokenparam);
			if (slave) {
				if (slave->isConnected()) {
					_log.Log(LOG_STATUS, "PROXY: Stopping slave connection.");
					slave->SetConnected(false);
				}
				if (reason == 50) {
					// proxy was restarted, try to reconnect to slave
					slave->SetConnected(true);
				}
			}
		}

		PDUFUNCTION(PDU_SERV_CONNECT)
		{
			/* incoming connect from master */
			if (!(_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ)) {
				_log.Log(LOG_ERROR, "PROXY: Shared Server access disallowed, denying connect request.");
				return;
			}
			CValueLengthPart parameters;
			long authenticated;
			std::string reason = "";

			GETPDUSTRING(tokenparam);
			GETPDUSTRING(usernameparam);
			GETPDUSTRING(passwordparam);
			GETPDULONG(protocol_version);
			GETPDUSTRING(ipparam);

			sharedData.AddConnectedServer(ipparam);
			if (protocol_version > 3) {
				authenticated = 0;
				reason = "Protocol version not supported";
			}
			else {
				authenticated = m_pDomServ->OnNewConnection(tokenparam, usernameparam, passwordparam) ? 1 : 0;
				reason = authenticated ? "Success" : "Invalid user/password";
			}

			ADDPDUSTRING(tokenparam);
			ADDPDUSTRING(sharedData.GetInstanceId());
			ADDPDULONG(authenticated);
			ADDPDUSTRING(reason);

			MyWrite(PDU_SERV_CONNECTRESP, parameters);
		}

		PDUFUNCTION(PDU_SERV_CONNECTRESP)
		{
			GETPDUSTRING(tokenparam);
			GETPDUSTRING(instanceparam);
			GETPDULONG(authenticated);
			GETPDUSTRING(reason);

			if (!authenticated) {
				_log.Log(LOG_ERROR, "PROXY: Could not log in to slave: %s", reason.c_str());
			}
			DomoticzTCP *slave = sharedData.findSlaveConnection(instanceparam);
			if (slave) {
				slave->Authenticated(tokenparam, authenticated == 1);
			}
		}

		PDUFUNCTION(PDU_SERV_SEND)
		{
			/* data from master to slave */
			bool success;

			GETPDUSTRING(tokenparam);
			GETPDUBINARY(data, datalen);

			success = m_pDomServ->OnIncomingData(tokenparam, data, datalen);
			free(data);
			if (!success) {
				SendServDisconnect(tokenparam, 1);
			}
		}

		PDUFUNCTION(PDU_SERV_RECEIVE)
		{
			if (!(_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ)) {
				_log.Log(LOG_ERROR, "PROXY: Shared Server access disallowed, denying receive data request.");
				return;
			}
			/* data from slave to master */

			GETPDUSTRING(tokenparam);
			GETPDUBINARY(data, datalen);

			DomoticzTCP *slave = sharedData.findSlaveConnection(tokenparam);
			if (slave && slave->isConnected()) {
				slave->FromProxy(data, datalen);
			}
			free(data);
		}

		void CProxyClient::SendServDisconnect(const std::string &token, int reason)
		{
			CValueLengthPart parameters;

			ADDPDUSTRING(token);
			ADDPDULONG(reason);

			MyWrite(PDU_SERV_DISCONNECT, parameters);
		}

		PDUFUNCTION(PDU_AUTHRESP)
		{
			// get auth response (0 or 1)

			if (we_locked_prefs_mutex) {
				// unlock prefs mutex
				we_locked_prefs_mutex = false;
				sharedData.UnlockPrefsMutex();
			}

			GETPDULONG(auth);
			GETPDUSTRING(reason);
			_log.Log(LOG_STATUS, "PROXY: Authenticate result: %s.", auth ? "success" : reason.c_str());
			if (auth) {
				sharedData.RestartTCPClients();
			}
			else {
				Stop();
			}
		}

		PDUFUNCTION(PDU_WS_OPEN)
		{
			// todo: make a map of websocket connections. There can be more than one.
			GETPDUSTRING(request_url);
			GETPDULONG(requestid);

			// open new virtual websocket connection
			// todo: different request_url's can have different websocket handlers
			websocket_handlers[requestid] = new CWebsocketHandler(m_pWebEm, boost::bind(&CProxyClient::WS_Write, this, requestid, _1));
			websocket_handlers[requestid]->Start();
		}

		PDUFUNCTION(PDU_WS_CLOSE)
		{
			GETPDULONG(requestid);

			CWebsocketHandler *handler = websocket_handlers[requestid];
			if (handler) {
				handler->Stop();
				delete handler;
				websocket_handlers.erase(requestid);
			}
		}

		PDUFUNCTION(PDU_WS_RECEIVE)
		{
			GETPDUSTRING(packet_data);
			GETPDULONG(requestid);

			CWebsocketHandler *handler = websocket_handlers[requestid];
			if (handler) {
				boost::tribool result = handler->Handle(packet_data);
			}
		}

		void CProxyClient::PduHandler(ProxyPdu &pdu)
		{
			CValueLengthPart part(pdu);

			switch (pdu._type) {
			ONPDU(PDU_REQUEST)
			ONPDU(PDU_ASSIGNKEY)
			ONPDU(PDU_ENQUIRE)
			ONPDU(PDU_AUTHRESP)
			ONPDU(PDU_SERV_CONNECT)
			ONPDU(PDU_SERV_DISCONNECT)
			ONPDU(PDU_SERV_CONNECTRESP)
			ONPDU(PDU_SERV_RECEIVE)
			ONPDU(PDU_SERV_SEND)
			ONPDU(PDU_SERV_ROSTERIND)
					ONPDU(PDU_WS_OPEN)
					ONPDU(PDU_WS_CLOSE)
					ONPDU(PDU_WS_RECEIVE)
			default:
				_log.Log(LOG_ERROR, "PROXY: pdu type: %d not expected.", pdu._type);
				break;
			}
		}

		std::string CProxyClient::compute_accept_header(const std::string &websocket_key)
		{
			// the length of an sha1 hash
			const int sha1len = 20;
			// the GUID as specified in RFC 6455
			const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			std::string combined = websocket_key + GUID;
			unsigned char sha1result[sha1len];
			sha1::calc((void *)combined.c_str(), combined.length(), sha1result);
			std::string accept = base64_encode(sha1result, sha1len);
			return accept;
		}

		bool CProxyClient::parse_response(const char *data, size_t len)
		{
			// todo: status code 101
			// connection: upgrade
			// upgrade: websocket
			// sec-websocket-protocol: domoproxy
			// Sec-Websocket-Accept: compute_accept_header(websocket_key)
			std::string response = std::string(data, len);
			if (response.find("\r\n\r\n") == std::string::npos) {
				return false;
			}
			return true;
		}

		void CProxyClient::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
		{
			const char *data;
			// data read, no need for timeouts anymore
			if (connection_status == status_connecting || doStop) {
				return;
			}
			timer_.cancel();
			if (!error)
			{
				_readbuf.commit(bytes_transferred);
				switch (connection_status) {
				case status_httpmode:
					data = boost::asio::buffer_cast<const char*>(_readbuf.data());
					if (parse_response(data, _readbuf.size())) {
						_readbuf.consume(_readbuf.size());
						connection_status = status_connected;
						LoginToService();
				}
					break;
				case status_connected:
					data = boost::asio::buffer_cast<const char*>(_readbuf.data());
					CWebsocketFrame frame;
					if (frame.Parse((const unsigned char *)data, _readbuf.size())) {
						ProxyPdu pdu(frame.Payload().c_str(), frame.Payload().length());
						if (!pdu.Disconnected()) {
				PduHandler(pdu);
							_readbuf.consume(frame.Consumed());
						}
					}
					break;
				}
				ReadMore();
			}
			else
			{
				if (!doStop) {
					_log.Log(LOG_ERROR, "PROXY: Read failed, code = %d. Reconnecting: %s", error.value(), error.message().c_str());
					// we are disconnected, reconnect
					Reconnect();
				}
			}
		}

		bool CProxyClient::SharedServerAllowed()
		{
			return ((_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ) > 0);
		}

		void CProxyClient::Stop()
		{
			timer_.cancel(); // to be sure
			if (we_locked_prefs_mutex) {
				we_locked_prefs_mutex = false;
				sharedData.UnlockPrefsMutex();
			}
			// stop and destroy all open websocket handlers
			for (std::map<long, CWebsocketHandler *>::iterator it = websocket_handlers.begin(); it != websocket_handlers.end(); ++it) {
				it->second->Stop();
				delete it->second;
			}
			websocket_handlers.clear();

			doStop = true;
			// signal end of WriteThread
			boost::system::error_code ec;
			_socket->shutdown(ec);
			_socket->lowest_layer().close(ec);
		}

		void CProxyClient::SetSharedServer(tcp::server::CTCPServerProxied *domserv)
		{
			m_pDomServ = domserv;
		}

		CProxyClient::~CProxyClient()
		{
		}

		CProxyManager::CProxyManager(const std::string& doc_root, http::server::cWebem *webEm, tcp::server::CTCPServer *domServ)
		{
			m_pDocRoot = doc_root;
			m_pWebEm = webEm;
			m_pDomServ = domServ;
			m_thread = NULL;
			_first = true;
		}

		CProxyManager::~CProxyManager()
		{
			if (m_thread) {
				delete m_thread;
			}
		}

		int CProxyManager::Start(bool first)
		{
			_first = first;
			m_thread = new boost::thread(boost::bind(&CProxyManager::StartThread, shared_from_this()));
			return 1;
		}

		void CProxyManager::StartThread()
		{
			try {
				//boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::tlsv12_client);
				boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::sslv23);
				ctx.set_verify_mode(boost::asio::ssl::verify_none);

				proxyclient.reset(new CProxyClient(io_service, ctx, m_pWebEm));
				proxyclient->Reconnect();
				if (_first && proxyclient->SharedServerAllowed()) {
					m_pDomServ->StartServer(proxyclient);
				}
				proxyclient->SetSharedServer(m_pDomServ->GetProxiedServer());
				io_service.run();
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "PROXY: StartThread(): Exception: %s", e.what());
			}
		}

		void CProxyManager::Stop()
		{
			if (m_thread) {
				io_service.post(boost::bind(&CProxyClient::Stop, proxyclient));
				delete m_thread;
				m_thread = NULL;
			}
		}

		boost::shared_ptr<CProxyClient> CProxyManager::GetProxyForMaster(DomoticzTCP *master) {
			sharedData.AddTCPClient(master);
			return proxyclient;
		}

		void CProxyClient::ConnectToDomoticz(std::string instancekey, std::string username, std::string password, DomoticzTCP *master, int protocol_version)
		{
			CValueLengthPart parameters;

			ADDPDUSTRING(instancekey);
			ADDPDUSTRING(username);
			ADDPDUSTRING(password);
			ADDPDULONG(protocol_version);
			MyWrite(PDU_SERV_CONNECT, parameters);
		}

		void CProxyClient::DisconnectFromDomoticz(const std::string &token, DomoticzTCP *master)
		{
			CValueLengthPart parameters;
			int reason = 2;

			ADDPDUSTRING(token);
			ADDPDULONG(reason);

			MyWrite(PDU_SERV_DISCONNECT, parameters);
			DomoticzTCP *slave = sharedData.findSlaveConnection(token);
			if (slave) {
				_log.Log(LOG_STATUS, "PROXY: Stopping slave.");
				slave->SetConnected(false);
			}
		}

		void CProxySharedData::SetInstanceId(std::string instanceid)
		{
			_instanceid = instanceid;
			m_sql.UpdatePreferencesVar("MyDomoticzInstanceId", _instanceid);
		}

		std::string CProxySharedData::GetInstanceId()
		{
			m_sql.GetPreferencesVar("MyDomoticzInstanceId", _instanceid);
			return _instanceid;
		}

		void CProxySharedData::LockPrefsMutex()
		{
			// todo: make this a boost::condition?
			prefs_mutex.lock();
		}

		void CProxySharedData::UnlockPrefsMutex()
		{
			prefs_mutex.unlock();
		}

		bool CProxySharedData::AddConnectedIp(std::string ip)
		{
			if (connectedips_.find(ip) == connectedips_.end())
			{
				//ok, this could get a very long list when running for years
				connectedips_.insert(ip);
				_log.Log(LOG_STATUS, "PROXY: Incoming connection from: %s", ip.c_str());
				return true;
			}
			return false;
		}

		bool CProxySharedData::AddConnectedServer(std::string ip)
		{
			if (connectedservers_.find(ip) == connectedservers_.end())
			{
				//ok, this could get a very long list when running for years
				connectedservers_.insert(ip);
				_log.Log(LOG_STATUS, "PROXY: Incoming Domoticz connection from: %s", ip.c_str());
				return true;
			}
			return false;
		}

		void CProxySharedData::AddTCPClient(DomoticzTCP *client)
		{
			for (std::vector<DomoticzTCP *>::iterator it = TCPClients.begin(); it != TCPClients.end(); it++) {
				if ((*it) == client) {
					return;
				}
			}
			int size = TCPClients.size();
			TCPClients.resize(size + 1);
			TCPClients[size] = client;
		}

		void CProxySharedData::RemoveTCPClient(DomoticzTCP *client)
		{
			// fast remove from vector
			std::vector<DomoticzTCP *>::iterator it = std::find(TCPClients.begin(), TCPClients.end(), client);
			if (it != TCPClients.end()) {
				using std::swap;
				// swap the one to be removed with the last element
				// and remove the item at the end of the container
				// to prevent moving all items after 'client' by one
				swap(*it, TCPClients.back());
				TCPClients.pop_back();
			}
		}

		DomoticzTCP *CProxySharedData::findSlaveConnection(const std::string &token)
		{
			for (unsigned int i = 0; i < TCPClients.size(); i++) {
				if (TCPClients[i]->CompareToken(token)) {
					return TCPClients[i];
				}
			}
			return NULL;
		}

		DomoticzTCP *CProxySharedData::findSlaveById(const std::string &instanceid)
		{
			for (unsigned int i = 0; i < TCPClients.size(); i++) {
				if (TCPClients[i]->CompareId(instanceid)) {
					return TCPClients[i];
				}
			}
			return NULL;
		}

		void CProxySharedData::RestartTCPClients()
		{
			for (unsigned int i = 0; i < TCPClients.size(); i++) {
				if (!TCPClients[i]->isConnected()) {
					TCPClients[i]->ConnectInternalProxy();
				}
			}
		}

		void CProxySharedData::StopTCPClients()
		{
			for (unsigned int i = 0; i < TCPClients.size(); i++) {
				TCPClients[i]->DisconnectProxy();
			}
		}
	}
}
#endif
