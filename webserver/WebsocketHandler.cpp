#include "stdafx.h"
#include "WebsocketHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "../main/localtime_r.h"
#include "../push/WebsocketPush.h"
#include "../json/json.h"
#include "cWebem.h"

namespace http {
	namespace server {

		CWebsocketHandler::CWebsocketHandler(cWebem *pWebem, boost::function<void(const std::string &packet_data)> _MyWrite) : 
			m_Push(this),
			sessionid(""),
			MyWrite(_MyWrite),
			myWebem(pWebem)
		{
			
		}

		CWebsocketHandler::~CWebsocketHandler()
		{
			Stop();
		}

		boost::tribool CWebsocketHandler::Handle(const std::string &packet_data)
		{
			reply rep;
			Json::Value jsonValue;
			Json::StyledWriter writer;
			// todo: we now assume the session (still) exists
			WebEmSession session;
			std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(sessionid);
			if (itt != myWebem->m_sessions.end()) {
				session = itt->second;
			}
			else if (myWebem->m_userpasswords.size() == 0) {
					session.rights = 2;
			}
			else {
				// todo: check: AreWeInLocalNetwork(). If yes, then session.rights = 2 without a session being setup.
				return false;
			}
			Json::Reader reader;
			Json::Value value;
			if (!reader.parse(packet_data, value)) {
				return true;
			}
			if (value["event"] != "request") {
				return true;
			}
			request req;
			req.method = "GET";
			std::string querystring = value["query"].asString();
			req.uri = "/json.htm?" + querystring;
			req.http_version_major = 1;
			req.http_version_minor = 1;
			req.headers.resize(0); // todo: do we need any headers?
			req.content.clear();
			if (myWebem->CheckForPageOverride(session, req, rep)) {
				if (rep.status == reply::ok) {
					jsonValue["event"] = "response";
					jsonValue["requestid"] = value["requestid"].asInt64();
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


		// todo: not sure 
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
				size_t fpos = scookie.find("SID=");
				if (fpos != std::string::npos)
				{
					scookie = scookie.substr(fpos);
					fpos = 0;
					size_t epos = scookie.find(';');
					if (epos != std::string::npos)
					{
						scookie = scookie.substr(0, epos);
					}
				}
				size_t upos = scookie.find("_", fpos);
				size_t ppos = scookie.find(".", upos);
				time_t now = mytime(NULL);
				if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
				{
					sSID = scookie.substr(fpos + 4, upos - fpos - 4);
					sAuthToken = scookie.substr(upos + 1, ppos - upos - 1);
					szTime = scookie.substr(ppos + 1);

					time_t stime;
					std::stringstream sstr;
					sstr << szTime;
					sstr >> stime;

					bool expired = stime < now;
					if (!expired) {
						sessionid = sSID;
					}
				}
			}
		}

		void CWebsocketHandler::OnDeviceChanged(const uint64_t DeviceRowIdx)
		{
			//Rob, needed a try/catch, but don't know why...
			//When a browser was still open and polling/connecting to the websocket, and the application was started this caused a crash
			try
			{
				std::string query = "type=devices&rid=" + boost::lexical_cast<std::string>(DeviceRowIdx);
				Json::Value request;
				Json::StyledWriter writer;
				request["event"] = "request";
				request["requestid"] = -1;
				request["query"] = query;
				std::string packet = writer.write(request);
				Handle(packet);
			}
			catch (...)
			{
				
			}
		}

		void CWebsocketHandler::OnMessage(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
		{
			Json::Value json;

			json["event"] = "notification";
			json["Subject"] = Subject;
			json["Text"] = Text;
			json["ExtraData"] = ExtraData;
			json["Priority"] = Priority;
			json["Sound"] = Sound;
			json["bFromNotification"] = bFromNotification;
			std::string response = json.toStyledString();
			MyWrite(response);
		}

	}
}