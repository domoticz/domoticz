#pragma once
#ifndef HTTP_FASTCGI_HPP
#define HTTP_FASTCGI_HPP

#include "request_handler.hpp"
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

#include "reply.hpp"
#include "request.hpp"
#include "server_settings.hpp"


namespace http {
namespace server {

class fastcgi_parser
{
public:
	static bool handlePHP(const server_settings &settings, const std::string &script_path, const request &req, reply &rep, modify_info &mInfo);
	static uint16_t request_id_;
};

} //namespace server
} //namespace http

#endif //HTTP_FASTCGI_HPP

