#pragma once

#include <libwebem/IWebsocketHandler.h>
#include <libwebem/request.h>
#include <libwebem/reply.h>
#include "../push/WebsocketPush.h"
#include "StoppableTask.h"
#include <thread>
#include <mutex>
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
			CDomoticzWebsocketHandler(cWebem* pWebem, std::function<void(const std::string& packet_data)> _MyWrite);
			~CDomoticzWebsocketHandler() override;
			bool Handle(const std::string& packet_data, bool outbound) override;
			void Start() override;
			void Stop() override;
			void OnDeviceChanged(uint64_t DeviceRowIdx);
			void OnSceneChanged(uint64_t SceneRowIdx);
			void SendNotification(const std::string& Subject, const std::string& Text, const std::string& ExtraData, int Priority, const std::string& Sound, bool bFromNotification);
			void SendLogMessage(int iLevel, const std::string& szMessage);
			void store_session_id(const request& req, const reply& rep) override;

			bool subscribeTo(const std::string& szTopic);
			bool unsubscribeFrom(const std::string& szTopic);

		protected:
			std::function<void(const std::string& packet_data)> MyWrite;
			std::string sessionid;
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
			std::shared_ptr<std::thread> m_thread;
			std::mutex m_mutex;
			void Do_Work();
		};

	} // namespace server
} // namespace http
