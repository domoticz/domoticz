#include "stdafx.h"
#include "DomoticzWebsocketHandler.h"

#include <utility>
#include "mainworker.h"
#include "Helper.h"
#include "json_helper.h"
#include <libwebem/cWebem.h>
#include "Logger.h"


namespace http {
	namespace server {

		CDomoticzWebsocketHandler::CDomoticzWebsocketHandler(cWebem* pWebem, std::function<void(const std::string& packet_data)> _MyWrite, const WebEmSession& session)
			: MyWrite(std::move(_MyWrite))
			, m_session(session)
			, myWebem(pWebem)
			, m_Push(this)
		{
		}

		CDomoticzWebsocketHandler::~CDomoticzWebsocketHandler()
		{
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler destroyed");
			Stop();
		}

		void CDomoticzWebsocketHandler::Start()
		{
			RequestStart();

			m_Push.Start();

			//Start worker thread
			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler started");
		}

		void CDomoticzWebsocketHandler::Stop()
		{
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler stopping");
			m_Push.Stop();
			if (m_thread)
			{
				RequestStop();
				m_thread->join();
				m_thread.reset();
			}
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler stopped");
		}

		void CDomoticzWebsocketHandler::Do_Work()
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

		bool CDomoticzWebsocketHandler::Handle(const std::string& packet_data, bool outbound)
		{
			try
			{
				Json::Value value;
				if (!ParseJSon(packet_data, value)) {
					return true;
				}

				std::string szEvent = value["event"].asString();

				if (szEvent.find("request") != std::string::npos)
				{
					if (HandleRequest(szEvent, value, outbound))
						return true;
				}
				else if (szEvent == "subscribe")
				{
					if (HandleSubscribe(szEvent, value, outbound))
						return true;
				}
				else if (szEvent == "unsubscribe")
				{
					if (HandleUnsubscribe(szEvent, value, outbound))
						return true;
				}
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "WebsocketHandler::%s Exception: %s", __func__, e.what());
			}

			Json::Value jsonValue;
			jsonValue["error"] = "Internal Server Error!!";
			jsonValue["event"] = "response";
			std::string response = JSonToFormatString(jsonValue);
			MyWrite(response);
			return true;
		}

		bool CDomoticzWebsocketHandler::HandleRequest(const std::string& szEvent, const Json::Value& value, const bool outbound)
		{
			WebEmSession session = m_session; // local copy — thread-safe
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

					bool bInternal = false;
					if (value.isMember("internal"))
						bInternal = value["internal"].asBool();

					if ((!bInternal) && (querystring.find("param=getdevices") != std::string::npos))
					{
						m_subscribed_devices.clear();

						if (querystring.find("rid=") != std::string::npos)
						{
							//We are interested in certain devices only
							std::string tstring = querystring.substr(querystring.find("rid=") + 4);
							size_t pos = tstring.find("&");
							if (pos != std::string::npos)
							{
								tstring = tstring.substr(0, pos);
							}
							std::vector<std::string> strarray;
							StringSplit(tstring, ",", strarray);
							for (const auto& itt : strarray)
							{
								uint64_t devIDX = std::stoull(itt);
								m_subscribed_devices[devIDX] = true;
							}
						}
					}

					Json::Value jsonValue;
					jsonValue["request"] = szEvent;
					jsonValue["event"] = "response";
					jsonValue["requestid"] = value["requestid"].asInt64();
					jsonValue["data"] = rep.content;
					std::string response = JSonToFormatString(jsonValue);
					MyWrite(response);
					return true;
				}
			}
			return false;
		}

		bool CDomoticzWebsocketHandler::HandleSubscribe(const std::string& szEvent, const Json::Value& value, const bool outbound)
		{
			std::string szTopic = value["topic"].asString();
			if (szTopic.empty())
				return false;

			if (subscribeTo(szTopic) == true)
			{
				Json::Value jsonValue;
				jsonValue["request"] = szEvent;
				jsonValue["event"] = "subscribed";
				std::string response = JSonToFormatString(jsonValue);
				MyWrite(response);
				return true;
			}
			return false;
		}

		bool CDomoticzWebsocketHandler::HandleUnsubscribe(const std::string& szEvent, const Json::Value& value, const bool outbound)
		{
			std::string szTopic = value["topic"].asString();
			if (szTopic.empty())
				return false;

			if (unsubscribeFrom(szTopic) == true)
			{
				Json::Value jsonValue;
				jsonValue["request"] = szEvent;
				jsonValue["event"] = "unsubscribed";
				std::string response = JSonToFormatString(jsonValue);
				MyWrite(response);
				return true;
			}
			return false;
		}


		bool CDomoticzWebsocketHandler::subscribeTo(const std::string& szTopic)
		{
			std::unique_lock<std::mutex> lock(m_subscribe_mutex);
			m_subscribed_topics[szTopic] = true;
			return true;
		}

		bool CDomoticzWebsocketHandler::unsubscribeFrom(const std::string& szTopic)
		{
			std::unique_lock<std::mutex> lock(m_subscribe_mutex);
			if (m_subscribed_topics.find(szTopic) != m_subscribed_topics.end())
			{
				m_subscribed_topics.erase(m_subscribed_topics.find(szTopic));
				return true;
			}
			return false;
		}

		bool CDomoticzWebsocketHandler::isSubscribed(const std::string& szTopic)
		{
			std::unique_lock<std::mutex> lock(m_subscribe_mutex);
			return (m_subscribed_topics.find(szTopic) != m_subscribed_topics.end());
		}

		void CDomoticzWebsocketHandler::OnDeviceChanged(const uint64_t DeviceRowIdx)
		{
			try
			{
				if (!m_subscribed_devices.empty())
				{
					if (m_subscribed_devices.find(DeviceRowIdx) == m_subscribed_devices.end())
					{
						return;
					}
				}
				std::string query = "type=command&param=getdevices&rid=" + std::to_string(DeviceRowIdx);
				Json::Value request;
				request["event"] = "device_request";
				request["internal"] = true;
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

		void CDomoticzWebsocketHandler::OnSceneChanged(const uint64_t SceneRowIdx)
		{
			try
			{
				std::string query = "type=command&param=getscenes&rid=" + std::to_string(SceneRowIdx);
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

		void CDomoticzWebsocketHandler::SendNotification(const std::string& Subject, const std::string& Text, const std::string& ExtraData, const int Priority, const std::string& Sound, const bool bFromNotification)
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

		void CDomoticzWebsocketHandler::SendDateTime()
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

		void CDomoticzWebsocketHandler::SendLogMessage(const int iLevel, const std::string& szMessage)
		{
			if (!isSubscribed("log"))
				return;
			Json::Value json;
			json["event"] = "log";
			json["level"] = iLevel;
			json["message"] = szMessage;
			std::string response = json.toStyledString();
			MyWrite(response);
		}
	} // namespace server
} // namespace http
