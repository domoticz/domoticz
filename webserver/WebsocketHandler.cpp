#include "stdafx.h"
#include "WebsocketHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "../push/WebsocketPush.h"
#include "../json/json.h"
#include "cWebem.h"

namespace http {
	namespace server {

		// forward declaration
		std::string CreateFrame(opcodes opcode, const std::string &payload);

		CWebsocketHandler::CWebsocketHandler(cWebem *m_pWebem, boost::function<void(const std::string &packet_data)> _MyWrite) : m_Push(this)
		{
			MyWrite = _MyWrite;
			myWebem = m_pWebem;
		}

		CWebsocketHandler::~CWebsocketHandler()
		{
			Stop();
		}

		boost::tribool CWebsocketHandler::Handle(const std::string &packet_data)
		{
			request req;
			reply rep;
			Json::Value jsonValue;
			Json::StyledWriter writer;
			// todo: we now assume the session (still) exists
			WebEmSession session;
			std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(session.id);
			if (itt != myWebem->m_sessions.end()) {
				session = itt->second;
			}
			if (session.rights <= 0) {
				// of course we wont want this in the production code
				session.rights = 1;
			}
			req.method = "GET";
			// Expected format of packet_data: "requestid/query-string" (including the quotes)
			std::string data = packet_data.substr(1, packet_data.length() - 2); // json-decode, todo: include json.h
			size_t_t pos = data.find("/");
			std::string requestid = data.substr(0, pos);
			std::string querystring = data.substr(pos + 1);
			req.uri = "/json.htm?" + querystring;
			req.http_version_major = 1;
			req.http_version_minor = 1;
			req.headers.resize(0); // todo: do we need any headers?
			req.content.clear();
			if (myWebem->CheckForPageOverride(session, req, rep)) {
				if (rep.status == reply::ok) {
					// todo: json_encode
					jsonValue["event"] = "response";
					jsonValue["requestid"] = requestid;
					jsonValue["data"] = rep.content;
					std::string response = writer.write(jsonValue);
					MyWrite(response);
					return true;
				}
			}
			jsonValue["error"] = "Internal Server Error!!";
			std::string response = writer.write(jsonValue);
			MyWrite(response);
			return true;
		}

		void CWebsocketHandler::Start()
		{
			m_Push.Start();
		}

		void CWebsocketHandler::Stop()
		{
			m_Push.Stop();
		}


		// todo: move this function to websocket handler
		void CWebsocketHandler::store_session_id(const request &req, const reply &rep)
		{
			//Check cookie if still valid
			const char* cookie_header = request::get_req_header(&req, "Cookie");
			if (cookie_header != NULL)
			{
				std::string sSID;
				std::string sAuthToken;
				std::string szTime;

				// Parse session id and its expiration date
				std::string scookie = cookie_header;
				int fpos = scookie.find("SID=");
				if (fpos != std::string::npos)
				{
					scookie = scookie.substr(fpos);
					fpos = 0;
					size_t_t epos = scookie.find(';');
					if (epos != std::string::npos)
					{
						scookie = scookie.substr(0, epos);
					}
				}
				int upos = scookie.find("_", fpos);
				int ppos = scookie.find(".", upos);

				if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
				{
					sSID = scookie.substr(fpos + 4, upos - fpos - 4);
					sAuthToken = scookie.substr(upos + 1, ppos - upos - 1);
					szTime = scookie.substr(ppos + 1);

					sessionid = sSID;
				}

				if (sessionid.empty()) {
					for (unsigned int i = 0; i < rep.headers.size(); i++) {
						if (boost::iequals(rep.headers[i].name, "Set-Cookie")) {
							// Parse session id and its expiration date
							scookie = rep.headers[i].value;
							int fpos = scookie.find("SID=");
							if (fpos != std::string::npos)
							{
								scookie = scookie.substr(fpos);
								fpos = 0;
								size_t_t epos = scookie.find(';');
								if (epos != std::string::npos)
								{
									scookie = scookie.substr(0, epos);
								}
							}
							int upos = scookie.find("_", fpos);
							int ppos = scookie.find(".", upos);

							if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
							{
								sSID = scookie.substr(fpos + 4, upos - fpos - 4);
								sAuthToken = scookie.substr(upos + 1, ppos - upos - 1);
								szTime = scookie.substr(ppos + 1);

								sessionid = sSID;
							}
						}
					}
				}
			}
		}

		void CWebsocketHandler::OnDeviceChanged(const unsigned long long DeviceRowIdx)
		{
			std::string query = "type=devices&rid=" + boost::lexical_cast<std::string>(DeviceRowIdx);
			std::string packet = "\"-1/" + query + "\"";
			Handle(packet);
		}

		void CWebsocketHandler::OnMessage(const std::string & Subject, const std::string & Text, const std::string & ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)
		{
			Json::Value json;

			json["event"] = "notification";
			json["Text"] = Text;
			// todo: other parameters
			std::string response = json.toStyledString();
			MyWrite(response);
		}

	}
}