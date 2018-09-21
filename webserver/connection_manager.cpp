//
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "connection_manager.hpp"
#include <algorithm>
#include <boost/bind.hpp>
#include <iostream>
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"

namespace http {
namespace server {

void connection_manager::start(connection_ptr c)
{
	connections_.insert(c);

	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint endpoint = c->socket().remote_endpoint(ec);
	if (ec) {
		// Prevent the exception to be thrown to run to avoid the server to be locked (still listening but no more connection or stop).
		// If the exception returns to WebServer to also create a exception loop.
		_log.Log(LOG_ERROR,"Getting error '%s' while getting remote_endpoint in connection_manager::start", ec.message().c_str());
		stop(c);
		return;
	}

	std::string s = endpoint.address().to_string();
	if (s.substr(0, 7) == "::ffff:") {
		s = s.substr(7);
	}
	if (connectedips_.find(s) == connectedips_.end())
	{
		//ok, this could get a very long list when running for years
		connectedips_.insert(s);
		//_log.Log(LOG_STATUS,"Incoming connection from: %s", s.c_str());
	}

	c->start();
}

void connection_manager::stop(connection_ptr c)
{
	connections_.erase(c);
	c->stop();
}

void connection_manager::stop_all()
{
	std::for_each(connections_.begin(), connections_.end(),
			boost::bind(&connection::stop, _1));
	connections_.clear();
}


} // namespace server
} // namespace http
