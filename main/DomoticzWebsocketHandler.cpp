#include "stdafx.h"
#include "DomoticzWebsocketHandler.h"

#include <utility>
#include <algorithm>
#include "mainworker.h"
#include "Helper.h"
#include "json_helper.h"
#include <libwebem/cWebem.h>
#include <libwebem/session.h>
#include "Logger.h"
#include "../hardware/plugins/PluginWebSocketRegistry.h"
#include "../hardware/plugins/PluginWebSocketBridge.h"


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
			if (m_started.exchange(true))
				return;
			RequestStart();

			m_Push.Start();

			//Start worker thread
			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler started");
		}

		void CDomoticzWebsocketHandler::Stop()
		{
			if (!m_started.exchange(false))
				return;
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler stopping");
			m_Push.Stop();
			{
				std::unique_lock<std::mutex> lock(m_pending_mutex);
				m_pending_device_updates.clear();
				m_pending_scene_updates.clear();
			}
			RequestStop();
			m_stop_requested.store(true);
			m_pending_cv.notify_one();
			m_thread->join();
			m_thread.reset();
			_log.Debug(DEBUG_WEBSERVER, "WebSocket: handler stopped");
		}

		void CDomoticzWebsocketHandler::Do_Work()
		{
			time_t lastSentDateTime = 0;

			while (true)
			{
				std::vector<uint64_t> devicesToProcess;
				std::vector<uint64_t> scenesToProcess;
				{
					std::unique_lock<std::mutex> lock(m_pending_mutex);
					m_pending_cv.wait_for(lock, std::chrono::milliseconds(1000), [this] {
						return m_stop_requested.load() || !m_pending_device_updates.empty() || !m_pending_scene_updates.empty();
					});
					if (m_stop_requested.load())
						break;
					devicesToProcess.assign(m_pending_device_updates.begin(), m_pending_device_updates.end());
					m_pending_device_updates.clear();
					scenesToProcess.assign(m_pending_scene_updates.begin(), m_pending_scene_updates.end());
					m_pending_scene_updates.clear();
				}

				if (!devicesToProcess.empty())
					ProcessDeviceUpdates(devicesToProcess);
				for (uint64_t sceneIdx : scenesToProcess)
					ProcessSceneUpdate(sceneIdx);

				time_t atime = mytime(nullptr);
				if (atime % 10 == 0 && atime != lastSentDateTime)
				{
					lastSentDateTime = atime;
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
				else if (szEvent == "unsubscribe_devices")
				{
					if (HandleUnsubscribeDevices(value, outbound))
						return true;
				}
				else if (szEvent == "plugin_command")
				{
					if (HandlePluginCommand(value, outbound))
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
						std::vector<uint64_t> newDevices;
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
								newDevices.push_back(std::stoull(itt));
							}
						}
						std::unique_lock<std::mutex> devLock(m_subscribed_devices_mutex);
						m_subscribed_devices.clear();
						for (uint64_t devIDX : newDevices)
						{
							m_subscribed_devices[devIDX] = true;
						}
						m_device_updates_active.store(true);
					}

					Json::Value jsonValue;
					jsonValue["request"] = szEvent;
					jsonValue["event"] = "response";
					jsonValue["requestid"] = value["requestid"];
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

			subscribeTo(szTopic);
			Json::Value jsonValue;
			jsonValue["request"] = szEvent;
			jsonValue["event"] = "subscribed";
			jsonValue["requestid"] = value["requestid"];
			std::string response = JSonToFormatString(jsonValue);
			MyWrite(response);
			return true;
		}

		bool CDomoticzWebsocketHandler::HandleUnsubscribe(const std::string& szEvent, const Json::Value& value, const bool outbound)
		{
			std::string szTopic = value["topic"].asString();
			if (szTopic.empty())
				return false;

			unsubscribeFrom(szTopic);
			Json::Value jsonValue;
			jsonValue["request"] = szEvent;
			jsonValue["event"] = "unsubscribed";
			jsonValue["requestid"] = value["requestid"];
			std::string response = JSonToFormatString(jsonValue);
			MyWrite(response);
			return true;
		}


		bool CDomoticzWebsocketHandler::HandleUnsubscribeDevices(const Json::Value& value, const bool outbound)
		{
			std::unique_lock<std::mutex> devLock(m_subscribed_devices_mutex);
			m_device_updates_active.store(false);
			m_subscribed_devices.clear();
			return true;
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
			return m_subscribed_topics.erase(szTopic) > 0;
		}

		bool CDomoticzWebsocketHandler::isSubscribed(const std::string& szTopic)
		{
			std::unique_lock<std::mutex> lock(m_subscribe_mutex);
			return (m_subscribed_topics.find(szTopic) != m_subscribed_topics.end());
		}

		void CDomoticzWebsocketHandler::OnDeviceChanged(const uint64_t DeviceRowIdx)
		{
			if (!m_device_updates_active.load())
				return;
			{
				std::unique_lock<std::mutex> lock(m_subscribed_devices_mutex);
				if (!m_subscribed_devices.empty() && m_subscribed_devices.find(DeviceRowIdx) == m_subscribed_devices.end())
					return;
			}
			{
				std::unique_lock<std::mutex> lock(m_pending_mutex);
				m_pending_device_updates.insert(DeviceRowIdx);
			}
			m_pending_cv.notify_one();
		}

		void CDomoticzWebsocketHandler::OnSceneChanged(const uint64_t SceneRowIdx)
		{
			{
				std::unique_lock<std::mutex> lock(m_pending_mutex);
				m_pending_scene_updates.insert(SceneRowIdx);
			}
			m_pending_cv.notify_one();
		}

		void CDomoticzWebsocketHandler::ProcessDeviceUpdates(const std::vector<uint64_t>& deviceIndices)
		{
			try
			{
				std::string ridStr;
				for (size_t i = 0; i < deviceIndices.size(); ++i)
				{
					if (i > 0) ridStr += ',';
					ridStr += std::to_string(deviceIndices[i]);
				}
				std::string query = "type=command&param=getdevices&rid=" + ridStr;
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

		void CDomoticzWebsocketHandler::ProcessSceneUpdate(const uint64_t SceneRowIdx)
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

		// Maximum byte length accepted for a plugin_command payload (64 KiB).
		// DoS protection: caps the per-message allocation on the plugin's queue so
		// that a misbehaving or compromised frontend cannot exhaust process memory
		// by flooding large payloads before the plugin worker thread drains them.
		static constexpr size_t kPluginCommandMaxPayloadBytes = 65536;

		bool CDomoticzWebsocketHandler::HandlePluginCommand(const Json::Value& value, const bool outbound)
		{
			// Security: only URIGHTS_SWITCHER or above may send commands.
			// This mirrors the rights level enforced for device-command operations
			// (Cmd_UpdateDevice and sSwitcherCommands in WebServerCommands.cpp).
			// The upper-bound guard rejects URIGHTS_NONE (254) and URIGHTS_CLIENTID (255)
			// which are numerically above URIGHTS_ADMIN (2) and must not be allowed.
			if (m_session.rights < URIGHTS_SWITCHER || m_session.rights > URIGHTS_ADMIN)
			{
				_log.Log(LOG_ERROR, "WebSocket plugin_command: rejected, insufficient rights (user '%s')", m_session.username.c_str());
				Json::Value resp;
				resp["event"] = "plugin_command_ack";
				resp["error"] = "Forbidden";
				MyWrite(JSonToFormatString(resp));
				return true;
			}

			// Validate required fields.
			if (!value.isMember("plugin") || !value["plugin"].isString())
			{
				_log.Log(LOG_ERROR, "WebSocket plugin_command: missing or non-string 'plugin' field");
				return false;
			}
			const std::string pluginKey = value["plugin"].asString();
			if (pluginKey.empty())
			{
				_log.Log(LOG_ERROR, "WebSocket plugin_command: empty 'plugin' field");
				return false;
			}
			if (!value.isMember("data"))
			{
				_log.Log(LOG_ERROR, "WebSocket plugin_command: missing 'data' field");
				return false;
			}

			// Serialise the data payload for the plugin queue.
			std::string dataStr;
			const Json::Value& dataVal = value["data"];
			if (dataVal.isString())
				dataStr = dataVal.asString();
			else
				dataStr = JSonToFormatString(dataVal);

			// Reject oversized payloads before touching any plugin state.
			if (dataStr.size() > kPluginCommandMaxPayloadBytes)
			{
				_log.Log(LOG_ERROR, "WebSocket plugin_command: payload too large (%zu bytes, max %zu)", dataStr.size(), kPluginCommandMaxPayloadBytes);
				Json::Value resp;
				resp["event"] = "plugin_command_ack";
				resp["plugin"] = pluginKey;
				resp["error"] = "Payload too large";
				resp["delivered"] = 0;
				MyWrite(JSonToFormatString(resp));
				return true;
			}

			// Determine target instance(s).
			std::vector<int> hwIds = CPluginWebSocketRegistry::Get().GetInstances(pluginKey);
			if (hwIds.empty())
			{
				_log.Debug(DEBUG_WEBSERVER, "WebSocket plugin_command: no running instances for plugin '%s'", pluginKey.c_str());
				_log.Log(LOG_STATUS, "WebSocket plugin_command: plugin '%s' is not subscribed or not running (check plugin key)", pluginKey.c_str());
				Json::Value resp;
				resp["event"] = "plugin_command_ack";
				resp["plugin"] = pluginKey;
				resp["delivered"] = 0;
				MyWrite(JSonToFormatString(resp));
				return true;
			}

			// Optional single-instance targeting.
			if (value.isMember("hwid") && value["hwid"].isInt())
			{
				int targetHwId = value["hwid"].asInt();
				hwIds.erase(
					std::remove_if(hwIds.begin(), hwIds.end(), [targetHwId](int id) { return id != targetHwId; }),
					hwIds.end());
			}

			// Fan out — each call is non-blocking (enqueues onto the plugin's own queue).
			int delivered = 0;
			for (int hwId : hwIds)
			{
				if (Plugins::EnqueueWebSocketMessage(hwId, dataStr))
					++delivered;
			}

			_log.Debug(DEBUG_WEBSERVER, "WebSocket plugin_command: plugin '%s', delivered to %d instance(s)", pluginKey.c_str(), delivered);

			Json::Value resp;
			resp["event"] = "plugin_command_ack";
			resp["plugin"] = pluginKey;
			resp["delivered"] = delivered;
			MyWrite(JSonToFormatString(resp));
			return true;
		}

		void CDomoticzWebsocketHandler::SendPluginMessage(const std::string& pluginKey, int hwId, const std::string& jsonPayload)
		{
			if (!isSubscribed("plugin:" + pluginKey))
				return;
			try
			{
				Json::Value json;
				json["event"] = "plugin";
				json["plugin"] = pluginKey;
				json["hwid"] = hwId;
				Json::Value data;
				if (ParseJSon(jsonPayload, data))
					json["data"] = data;
				else
					json["data"] = jsonPayload;
				std::string response = json.toStyledString();
				MyWrite(response);
			}
			catch (std::exception& e)
			{
				_log.Log(LOG_ERROR, "WebsocketHandler::%s Exception: %s", __func__, e.what());
			}
		}
	} // namespace server
} // namespace http
