#include "stdafx.h"
#include "Websockets.hpp"

namespace http {
	namespace server {


CWebsocket::CWebsocket()
{

}

CWebsocket::~CWebsocket()
{
}

boost::tribool CWebsocket::parse(const char *begin, size_t size, std::string &websocket_data)
{
	return true;
}

void CWebsocket::handle_packet(const std::string &websocket_data, bool &keepalive_)
{

}

}
}