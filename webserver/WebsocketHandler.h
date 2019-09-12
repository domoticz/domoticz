#pragma once

#include "request.hpp"
#include "reply.hpp"
#include <boost/logic/tribool.hpp>
#include "../push/WebsocketPush.h"
#include "../main/StoppableTask.h"
#include <thread>
#include <mutex>
#include <memory>

namespace http {
	namespace server {

		class cWebem;

		class CWebsocketHandler : public StoppableTask 
		{
		public:
			CWebsocketHandler(cWebem *pWebem, boost::function<void(const std::string &packet_data)> _MyWrite);
			~CWebsocketHandler();
			virtual boost::tribool Handle(const std::string &packet_data, bool outbound);
			virtual void Start();
			virtual void Stop();
			virtual void OnDeviceChanged(const uint64_t DeviceRowIdx);
			virtual void OnSceneChanged(const uint64_t SceneRowIdx);
			virtual void SendNotification(const std::string& Subject, const std::string& Text, const std::string& ExtraData, const int Priority, const std::string& Sound, const bool bFromNotification);
			virtual void store_session_id(const request &req, const reply &rep);
		protected:
			boost::function<void(const std::string &packet_data)> MyWrite;
			std::string sessionid;
			cWebem* myWebem;
			CWebSocketPush m_Push;
		private:
			void SendDateTime();
			std::shared_ptr<std::thread> m_thread;
			std::mutex m_mutex;
			void Do_Work();
		};

	}
}
