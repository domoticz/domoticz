#pragma once

#include "request.hpp"
#include "reply.hpp"
#include "../push/WebsocketPush.h"
#include "../main/StoppableTask.h"
#include <thread>
#include <mutex>
#include <memory>
#include <map>
#include <string>

namespace http
{
	namespace server
	{

		class cWebem;

		class CWebsocketHandler : public StoppableTask
		{
		public:
			CWebsocketHandler(cWebem* pWebem, std::function<void(const std::string& packet_data)> _MyWrite);
			~CWebsocketHandler();
			bool Handle(const std::string& packet_data, bool outbound);
			void Start();
			void Stop();
			void OnDeviceChanged(uint64_t DeviceRowIdx);
			void OnSceneChanged(uint64_t SceneRowIdx);
			void SendNotification(const std::string& Subject, const std::string& Text, const std::string& ExtraData, int Priority, const std::string& Sound, const bool bFromNotification);
			void SendLogMessage(const int iLevel, const std::string& szMessage);
			void store_session_id(const request& req, const reply& rep);

			bool subscribeTo(const std::string& szTopic, const std::string& szSessionID);
			bool unsubscribeTo(const std::string& szTopic, const std::string& szSessionID);

		protected:
			std::function<void(const std::string& packet_data)> MyWrite;
			std::string sessionid;
			cWebem* myWebem;
			CWebSocketPush m_Push;

		private:
			bool isSubscribed(const std::string& szTopic);
			std::map<std::string, bool> m_subscribed_topics;
			std::mutex m_subscribe_mutex;

			void SendDateTime();
			std::shared_ptr<std::thread> m_thread;
			std::mutex m_mutex;
			void Do_Work();
		};

	} // namespace server
} // namespace http
