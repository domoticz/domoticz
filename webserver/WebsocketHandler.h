#pragma once

#include "request.hpp"
#include "reply.hpp"
#include <boost/logic/tribool.hpp>

namespace http {
	namespace server {

		class cWebem;

		class CWebsocketHandler {
		public:
			CWebsocketHandler(cWebem *m_pWebem, boost::function<void(const std::string &packet_data)> _MyWrite);
			~CWebsocketHandler();
			virtual boost::tribool Handle(const std::string &packet_data);
		protected:
			boost::function<void(const std::string &packet_data)> MyWrite;
			void store_session_id(const request &req, const reply &rep);
			std::string sessionid;
			cWebem* myWebem;
		};

	}
}