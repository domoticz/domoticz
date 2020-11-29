#include "stdafx.h"
#include "WebsocketHandler.h"

#include <utility>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/Helper.h"
#include "../main/json_helper.h"
#include "cWebem.h"
#include "../main/Logger.h"

#define WEBSOCKET_SESSION_TIMEOUT 86400 // 1 day

namespace http {
	namespace server {

		CWebsocketHandler::CWebsocketHandler(cWebem *pWebem, boost::function<void(const std::string &packet_data)> _MyWrite)
			: MyWrite(std::move(_MyWrite))
			, myWebem(pWebem)
			, m_Push(this)
		{
		}

		CWebsocketHandler::~CWebsocketHandler()
		{
			Stop();
		}

		boost::tribool CWebsocketHandler::Handle(const std::string &packet_data, bool outbound)
		{
			Json::Value jsonValue;
			try
			{
				// WebSockets only do security during set up so keep pushing the expiry out to stop it being cleaned up
				WebEmSession session;
				std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(sessionid);
				if (itt != myWebem->m_sessions.end())
				{
					session = itt->second;
				}
				else
					// for outbound messages create a temporary session if required
					// todo: Add the username and rights from the original connection
					if (outbound)
					{
						time_t nowAnd1Day = ((time_t)mytime(nullptr)) + WEBSOCKET_SESSION_TIMEOUT;
						session.timeout = nowAnd1Day;
						session.expires = nowAnd1Day;
						session.isnew = false;
						session.forcelogin = false;
						session.rememberme = false;
						session.reply_status = 200;
					}


				Json::Value value;
				if (!ParseJSon(packet_data, value)) {
					return true;
				}
				std::string szEvent = value["event"].asString();
				if (szEvent.find("request") == std::string::npos)
					return true;

				request req;
				req.method = "GET";
				std::string querystring = value["query"].asString();
				req.uri = myWebem->GetWebRoot() + "/json.htm?" + querystring;
				req.http_version_major = 1;
				req.http_version_minor = 1;
				req.headers.resize(0); // todo: do we need any headers?
				req.content.clear();
				reply rep;
				if (myWebem->CheckForPageOverride(session, req, rep)) {
					if (rep.status == reply::ok) {
						jsonValue["request"] = szEvent;
						jsonValue["event"] = "response";
						Json::Value::Int64 reqID = value["requestid"].asInt64();
						jsonValue["requestid"] = reqID;
						jsonValue["data"] = rep.content;
						std::string response = JSonToFormatString(jsonValue);
						MyWrite(response);
						return true;
					}
				}
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "WebsocketHandler::%s Exception: %s", __func__, e.what());
			}

			jsonValue["error"] = "Internal Server Error!!";
			std::string response = JSonToFormatString(jsonValue);
			MyWrite(response);
			return true;
		}

		void CWebsocketHandler::Start()
		{
			RequestStart();

			m_Push.Start();

			//Start worker thread
			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
		}

		void CWebsocketHandler::Stop()
		{
			m_Push.Stop();
			if (m_thread)
			{
				RequestStop();
				m_thread->join();
				m_thread.reset();
			}
		}

		void CWebsocketHandler::Do_Work()
		{
			while (!IsStopRequested(1000))
			{
				time_t atime = mytime(nullptr);
				if (atime % 10 == 0)
				{
					//Send Date/Time every 10 seconds
					SendDateTime();
				}
			}
		}

		// todo: not sure 
		void CWebsocketHandler::store_session_id(const request &req, const reply &rep)
		{
			//Check cookie if still valid
			const char* cookie_header = request::get_req_header(&req, "Cookie");
			if (cookie_header != nullptr)
			{
				std::string sSID;
				std::string szTime;

				// Parse session id and its expiration date
				std::string scookie = cookie_header;
				size_t fpos = scookie.find("DMZSID=");
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
				size_t upos = scookie.find('_', fpos);
				size_t ppos = scookie.find('.', upos);
				time_t now = mytime(nullptr);
				if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
				{
					sSID = scookie.substr(fpos + 7, upos - fpos - 7);
					//std::string sAuthToken = scookie.substr(upos + 1, ppos - upos - 1);
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
			try
			{
				std::string query = "type=devices&rid=" + std::to_string(DeviceRowIdx);
				Json::Value request;
				request["event"] = "device_request";
				request["requestid"] = -1;
				request["query"] = query;
				std::string packet = JSonToFormatString(request);
				Handle(packet, true);
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "WebsocketHandler::%s Exception: %s", __func__, e.what());
			}
		}

		void CWebsocketHandler::OnSceneChanged(const uint64_t SceneRowIdx)
		{
			try
			{
				std::string query = "type=scenes&rid=" + std::to_string(SceneRowIdx);
				Json::Value request;
				request["event"] = "scene_request";
				request["requestid"] = -1;
				request["query"] = query;

				std::string packet = JSonToFormatString(request);
				Handle(packet, true);
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "WebsocketHandler::%s Exception: %s", __func__, e.what());
			}
		}

		void CWebsocketHandler::SendNotification(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
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

		void CWebsocketHandler::SendDateTime()
		{
			if (!m_mainworker.m_LastSunriseSet.empty())
			{
				std::vector<std::string> strarray;
				StringSplit(m_mainworker.m_LastSunriseSet, ";", strarray);
				if (strarray.size() == 10)
				{
					char szTmp[100];
					struct tm loctime;
					time_t now = mytime(nullptr);

					localtime_r(&now, &loctime);
					strftime(szTmp, 80, "%Y-%m-%d %X", &loctime);

					Json::Value json;

					json["event"] = "date_time";
					json["ServerTime"] = szTmp;
					json["Sunrise"] = strarray[0];
					json["Sunset"] = strarray[1];
					std::string response = json.toStyledString();
					MyWrite(response);
				}
			}
		}

	} // namespace server
} // namespace http
