#include "stdafx.h"
#ifndef NOCLOUD
#include "proxyclient.h"
#include "../hardware/DomoticzTCP.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "../tcpserver/TCPServer.h"
#include "sha1.hpp"

extern std::string szAppVersion;

#define TIMEOUT 60
#define PONG "PONG"

namespace http {
	namespace server {

		CProxySharedData sharedData;

		CProxyClient::CProxyClient() : ASyncTCP(true)
		{
			m_pDomServ = NULL;
		}

		void CProxyClient::Reset()
		{
			connection_status = status_httpmode;
			_allowed_subsystems = 0;
			m_sql.GetPreferencesVar("MyDomoticzUserId", _apikey);
			m_sql.GetPreferencesVar("MyDomoticzPassword", _password);
			m_sql.GetPreferencesVar("MyDomoticzSubsystems", _allowed_subsystems);
			if (_password != "") {
				_password = base64_decode(_password);
			}
			readbuf.clear();
		}

		void CProxyClient::WriteSlaveData(const std::string &token, const char *pData, size_t Length)
		{
			/* data from slave to master */
			ProxyPdu_SERV_RECEIVE response;
			response.m_tokenparam = token;
			response.m_data = std::string(pData, Length);
			MyWrite(response.ToBinary());
		}

		void CProxyClient::WriteMasterData(const std::string &token, const char *pData, size_t Length)
		{
			/* data from master to slave */
			ProxyPdu_SERV_SEND response;
			response.m_token = token;
			response.m_data = std::string(pData, Length);
			MyWrite(response.ToBinary());
		}

		void CProxyClient::WS_Write(unsigned long long requestid, const std::string &packet_data)
		{
			ProxyPdu_WS_SEND response;
			response.m_packet_data = packet_data;
			response.m_requestid = requestid;
			MyWrite(response.ToBinary());
		}

		void CProxyClient::MyWrite(const std::string &msg)
		{
			if (connection_status != status_connected) {
				return;
			}
			write(CWebsocketFrame::Create(opcode_binary, msg, true));
		}

		void CProxyClient::LoginToService()
		{
			std::string instanceid = sharedData.GetInstanceId();
			const long protocol_version = 3;

			ProxyPdu_AUTHENTICATE response;
			response.m_apikey = _apikey;
			response.m_instanceid = instanceid;
			response.m_password = _password;
			response.m_szAppVersion = szAppVersion;
			response.m_allowed_subsystems = _allowed_subsystems;
			response.m_protocol_version = protocol_version;
			MyWrite(response.ToBinary());
		}

		void CProxyClient::OnConnect()
		{
			Reset();
			WebsocketGetRequest();
		}

		void CProxyClient::WebsocketGetRequest()
		{
			// generate random websocket key
			unsigned char random[16];
			for (int i = 0; i < sizeof(random); i++) {
				random[i] = rand();
			}
			websocket_key = base64_encode(random, sizeof(random));
			std::string request = "GET /proxycereal HTTP/1.1\r\nHost: my.domoticz.com\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nOrigin: Domoticz\r\nSec-Websocket-Version: 13\r\nSec-Websocket-Protocol: MyDomoticz\r\nSec-Websocket-Key: " + websocket_key + "\r\n\r\n";
			write(request);
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

		PDUFUNCTION(REQUEST)
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

			if (pdu->m_subsystem == SUBSYSTEM_HTTP) {
				// "normal web request", get parameters

				if (pdu->m_requestbody.size() > 0) {
					request = "POST ";
				}
				else {
					request = "GET ";
				}
				request += pdu->m_requesturl;
				request += " HTTP/1.1\r\n";
				request += pdu->m_requestheaders;
				request += "\r\n";
				request += pdu->m_requestbody;

				_buf = boost::asio::buffer((void *)request.c_str(), request.length());

				sharedData.AddConnectedIp(pdu->m_originatingip);

				GetRequest(pdu->m_originatingip, _buf, reply_);
				// assemble response
				responseheaders = GetResponseHeaders(reply_);

				ProxyPdu_RESPONSE response;
				response.m_status = reply_.status;
				response.m_responseheaders = responseheaders;
				response.m_content = reply_.content;
				// we number the request, because we can send back asynchronously
				response.m_requestid = pdu->m_requestid;

				// send response to proxy
				MyWrite(response.ToBinary());
			}
			else {
				// unknown subsystem
				_log.Log(LOG_ERROR, "PROXY: Got Request pdu for unknown subsystem %d.", pdu->m_subsystem);
			}
		}

		PDUFUNCTION(ASSIGNKEY)
		{
			_log.Log(LOG_STATUS, "PROXY: We were assigned an instance id: %s.\n", pdu->m_newapi.c_str());
			sharedData.SetInstanceId(pdu->m_newapi);
			// re-login with the new instance id
			LoginToService();
		}

		PDUFUNCTION(ENQUIRE)
		{
			// assemble response
			ProxyPdu_ENQUIRE response;
			// send response to proxy
			MyWrite(response.ToBinary());
		}

		PDUFUNCTION(SERV_ROSTERIND)
		{
			_log.Log(LOG_STATUS, "PROXY: Notification received: slave %s online now.", pdu->m_c_slave.c_str());
			DomoticzTCP *slave = sharedData.findSlaveById(pdu->m_c_slave);
			if (slave) {
				slave->SetConnected(true);
			}
		}

		PDUFUNCTION(SERV_DISCONNECT)
		{
			bool success;

			// see if we are slave
			success = m_pDomServ->OnDisconnect(pdu->m_tokenparam);
			if (success) {
				_log.Log(LOG_STATUS, "PROXY: Master disconnected");
			}
			// see if we are master
			DomoticzTCP *slave = sharedData.findSlaveConnection(pdu->m_tokenparam);
			if (slave) {
				if (slave->isConnected()) {
					_log.Log(LOG_STATUS, "PROXY: Stopping slave connection.");
					slave->SetConnected(false);
				}
				if (pdu->m_reason == 50) {
					// proxy was restarted, try to reconnect to slave
					slave->SetConnected(true);
				}
			}
		}

		PDUFUNCTION(SERV_CONNECT)
		{
			/* incoming connect from master */
			if (!(_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ)) {
				_log.Log(LOG_ERROR, "PROXY: Shared Server access disallowed, denying connect request.");
				return;
			}
			long authenticated;
			std::string reason = "";

			sharedData.AddConnectedServer(pdu->m_ipparam);
			if (pdu->m_protocol_version > 4) {
				authenticated = 0;
				reason = "Protocol version not supported";
			}
			else {
				authenticated = m_pDomServ->OnNewConnection(pdu->m_tokenparam, pdu->m_usernameparam, pdu->m_passwordparam) ? 1 : 0;
				reason = authenticated ? "Success" : "Invalid user/password";
			}

			ProxyPdu_SERV_CONNECTRESP response;
			response.m_tokenparam = pdu->m_tokenparam;
			response.m_instanceid = sharedData.GetInstanceId();
			response.m_authenticated = authenticated;
			response.m_reason = reason;
			MyWrite(response.ToBinary());
		}

		PDUFUNCTION(SERV_CONNECTRESP)
		{
			if (!pdu->m_authenticated) {
				_log.Log(LOG_ERROR, "PROXY: Could not log in to slave: %s", pdu->m_reason.c_str());
				return;
			}
			DomoticzTCP *slave = sharedData.findSlaveConnection(pdu->m_instanceid);
			if (slave) {
				slave->Authenticated(pdu->m_tokenparam, pdu->m_authenticated == 1);
			}
		}

		PDUFUNCTION(SERV_SEND)
		{
			/* data from master to slave */
			bool success;
			success = m_pDomServ->OnIncomingData(pdu->m_token, (const unsigned char *)pdu->m_data.c_str(), pdu->m_data.length());
			if (!success) {
				SendServDisconnect(pdu->m_token, 1);
			}
		}

		PDUFUNCTION(SERV_RECEIVE)
		{
			if (!(_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ)) {
				_log.Log(LOG_ERROR, "PROXY: Shared Server access disallowed, denying receive data request.");
				return;
			}
			/* data from slave to master */

			DomoticzTCP *slave = sharedData.findSlaveConnection(pdu->m_tokenparam);
			if (slave && slave->isConnected()) {
				slave->FromProxy((const unsigned char *)pdu->m_data.c_str(), pdu->m_data.length());
			}
		}

		void CProxyClient::SendServDisconnect(const std::string &token, int reason)
		{
			ProxyPdu_SERV_DISCONNECT response;
			response.m_tokenparam = token;
			response.m_reason = reason;
			MyWrite(response.ToBinary());
		}

		PDUFUNCTION(AUTHRESP)
		{
			// get auth response (0 or 1)
			_log.Log(LOG_STATUS, "PROXY: Authenticate result: %s.", pdu->m_auth ? "success" : pdu->m_reason.c_str());
			if (pdu->m_auth) {
				sharedData.RestartTCPClients();
			}
			else {
				disconnect();
			}
		}

		PDUFUNCTION(WS_OPEN)
		{
			// todo: make a map of websocket connections. There can be more than one.
			// open new virtual websocket connection
			// todo: different request_url's can have different websocket handlers
			websocket_handlers[pdu->m_requestid] = new CWebsocketHandler(m_pWebEm, boost::bind(&CProxyClient::WS_Write, this, pdu->m_requestid, _1));
			websocket_handlers[pdu->m_requestid]->Start();
		}

		PDUFUNCTION(WS_CLOSE)
		{
			CWebsocketHandler *handler = websocket_handlers[pdu->m_requestid];
			if (handler) {
				handler->Stop();
				delete handler;
				websocket_handlers.erase(pdu->m_requestid);
			}
		}

		PDUFUNCTION(WS_RECEIVE)
		{
			CWebsocketHandler *handler = websocket_handlers[pdu->m_requestid];
			if (handler) {
				boost::tribool result = handler->Handle(pdu->m_packet_data);
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

		void CProxyClient::OnData(const unsigned char *pData, size_t length)
		{
			readbuf.append((const char *)pData, length);
			switch (connection_status) {
			case status_httpmode:
				if (parse_response(readbuf.c_str(), readbuf.size())) {
					readbuf.clear();
					connection_status = status_connected;
					LoginToService();
				}
				break;
			case status_connected:
				CWebsocketFrame frame;
				if (frame.Parse((const uint8_t *)readbuf.c_str(), readbuf.size())) {
					readbuf.clear();
					switch (frame.Opcode()) {
					case opcodes::opcode_ping:
						write(CWebsocketFrame::Create(opcodes::opcode_pong, PONG, true));
						break;
					case opcodes::opcode_binary:
					case opcodes::opcode_text:
						std::shared_ptr<CProxyPduBase> pdu = CProxyPduBase::FromString(frame.Payload());
						switch (pdu->pdu_type()) {
#define PDUSTRING(name)
#define PDULONG(name)
#define PDULONGLONG(name)
#define PROXYPDU(name, members) case ePDU_##name: OnPduReceived(std::dynamic_pointer_cast<ProxyPdu_##name>(pdu)); break;
#include "proxydef.def"
						default:
							// pdu enum not found
							break;
						}
#undef PDUSTRING
#undef PDULONG
#undef PDULONGLONG
#undef PROXYPDU
						break;
					}
				}
				break;
			}
		}

		bool CProxyClient::SharedServerAllowed()
		{
			return ((_allowed_subsystems & SUBSYSTEM_SHAREDDOMOTICZ) > 0);
		}

		void CProxyClient::OnDisconnect()
		{
			_log.Log(LOG_NORM, "Proxy: disconnected");
			// stop and destroy all open websocket handlers
			for (std::map<unsigned long long, CWebsocketHandler *>::iterator it = websocket_handlers.begin(); it != websocket_handlers.end(); ++it) {
				it->second->Stop();
				delete it->second;
			}
			websocket_handlers.clear();
		}

		void CProxyClient::OnError(const boost::system::error_code & error)
		{
			_log.Log(LOG_NORM, "Proxy: Error, reconnecting (%s)", error.message().c_str());
		}

		void CProxyClient::SetSharedServer(tcp::server::CTCPServerProxied *domserv)
		{
			m_pDomServ = domserv;
		}

		CProxyClient::~CProxyClient()
		{
		}

		void CProxyClient::Connect(http::server::cWebem *webEm)
		{
			m_pWebEm = webEm;
			SetReconnectDelay(15);
			connect("proxy.mydomoticz.com", 443);
		}

		void CProxyClient::Disconnect()
		{
			disconnect();
		}

		bool CProxyClient::Enabled()
		{
			Reset();
			return (_apikey != "" && _password != "" && _allowed_subsystems != 0);
		}


		CProxyManager::CProxyManager()
		{
		}

		CProxyManager::~CProxyManager()
		{
			Stop();
		}

		bool CProxyManager::Start(http::server::cWebem *webEm, tcp::server::CTCPServer *domServ)
		{
			if (!proxyclient.Enabled()) {
				// mydomoticz has not been set up
				return false;
			}
			m_pDomServ = domServ;
			proxyclient.Connect(webEm);
			if (proxyclient.SharedServerAllowed()) {
				m_pDomServ->StartServer(&proxyclient);
			}
			proxyclient.SetSharedServer(m_pDomServ->GetProxiedServer());
			return true;
		}

		void CProxyManager::Stop()
		{
			proxyclient.Disconnect();
		}

		void CProxyManager::SetWebRoot(const std::string & doc_root)
		{
			m_pDocRoot = doc_root;
		}

		CProxyClient *CProxyManager::GetProxyForMaster(DomoticzTCP *master) {
			sharedData.AddTCPClient(master);
			return &proxyclient;
		}

		void CProxyClient::ConnectToDomoticz(std::string instancekey, std::string username, std::string password, DomoticzTCP *master, int protocol_version)
		{
			ProxyPdu_SERV_CONNECT response;
			response.m_ipparam = instancekey;
			response.m_usernameparam = username;
			response.m_passwordparam = password;
			response.m_protocol_version = protocol_version;
			MyWrite(response.ToBinary());
		}

		void CProxyClient::DisconnectFromDomoticz(const std::string &token, DomoticzTCP *master)
		{
			int reason = 2;

			ProxyPdu_SERV_DISCONNECT response;
			response.m_tokenparam = token;
			response.m_reason = reason;
			MyWrite(response.ToBinary());
			DomoticzTCP *slave = sharedData.findSlaveConnection(token);
			if (slave) {
				_log.Log(LOG_STATUS, "PROXY: Stopping slave.");
				slave->SetConnected(false);
			}
		}

		void CProxySharedData::SetInstanceId(const std::string &instanceid)
		{
			_instanceid = instanceid;
			m_sql.UpdatePreferencesVar("MyDomoticzInstanceId", _instanceid);
		}

		std::string CProxySharedData::GetInstanceId()
		{
			m_sql.GetPreferencesVar("MyDomoticzInstanceId", _instanceid);
			return _instanceid;
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
			for (std::vector<DomoticzTCP *>::iterator it = TCPClients.begin(); it != TCPClients.end(); ++it) {
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

		/* pdu handlers not in use (server side) */
		PDUFUNCTION(AUTHENTICATE) {}
		PDUFUNCTION(NONE) {}
		PDUFUNCTION(RESPONSE) {}
		PDUFUNCTION(WS_SEND) {}
	}
}
#endif
