#pragma once

#include <libwebem/IWebsocketHandler.h>
#include <libwebem/session.h>
#include "../push/WebsocketPush.h"
#include "StoppableTask.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <memory>
#include <map>
#include <string>

namespace Json
{
	class Value;
} // namespace Json

namespace http
{
	namespace server
	{

		class cWebem;

		class CDomoticzWebsocketHandler : public IWebsocketHandler, public StoppableTask
		{
		public:
			CDomoticzWebsocketHandler(cWebem* pWebem, std::function<void(const std::string& packet_data)> _MyWrite, const WebEmSession& session);
			~CDomoticzWebsocketHandler() override;
			bool Handle(const std::string& packet_data, bool outbound) override;
			void Start() override;
			void Stop() override;
			void OnDeviceChanged(uint64_t DeviceRowIdx);
			void OnSceneChanged(uint64_t SceneRowIdx);
			void SendNotification(const std::string& Subject, const std::string& Text, const std::string& ExtraData, int Priority, const std::string& Sound, bool bFromNotification);
			void SendLogMessage(int iLevel, const std::string& szMessage);

			bool subscribeTo(const std::string& szTopic);
			bool unsubscribeFrom(const std::string& szTopic);

		protected:
			std::function<void(const std::string& packet_data)> MyWrite;
			WebEmSession m_session;
			cWebem* myWebem;
			CWebSocketPush m_Push;

		private:
			bool HandleRequest(const std::string& szEvent, const Json::Value& value, bool outbound);
			bool HandleSubscribe(const std::string& szEvent, const Json::Value& value, bool outbound);
			bool HandleUnsubscribe(const std::string& szEvent, const Json::Value& value, bool outbound);
			bool isSubscribed(const std::string& szTopic);
			std::map<std::string, bool> m_subscribed_topics;
			std::map<uint64_t, bool> m_subscribed_devices;
			std::mutex m_subscribe_mutex;

			void SendDateTime();
			void ProcessDeviceUpdates(const std::vector<uint64_t>& deviceIndices);
			void ProcessSceneUpdate(uint64_t SceneRowIdx);
			void Do_Work();
			std::shared_ptr<std::thread> m_thread;
			std::mutex m_pending_mutex;
			std::condition_variable m_pending_cv;
			std::deque<uint64_t> m_pending_device_updates;
			std::deque<uint64_t> m_pending_scene_updates;
		};

	} // namespace server
} // namespace http
