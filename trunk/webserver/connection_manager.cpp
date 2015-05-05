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
#include "../main/Logger.h"
#include "../main/localtime_r.h"

namespace http {
namespace server {

#define CONNECTION_SESSION_TIMEOUT 10*60

connection_manager::connection_manager()
{
	last_timeout_check_ = time(NULL);
}

void connection_manager::start(connection_ptr c)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	connections_.insert(c);
	std::string s = c->socket().remote_endpoint().address().to_string();

	time_t atime = time(NULL);
	std::map<std::string, time_t>::iterator itt = connectedips_.find(s);
	if (itt==connectedips_.end())
	{
		connectedips_[s] = atime;
		_log.Log(LOG_STATUS,"Incoming connection from: %s", s.c_str());
	}
	else
	{
		if (atime - connectedips_[s] > 2 * 60)
		{
			_log.Log(LOG_STATUS, "Incoming connection from: %s", s.c_str());
		}
		connectedips_[s]=atime;
	}

	//Cleanup old connections
	if (atime - last_timeout_check_ > 60)
	{
		last_timeout_check_ = atime;
		itt = connectedips_.begin();
		while (itt != connectedips_.end())
		{
			if (atime - itt->second > CONNECTION_SESSION_TIMEOUT)
			{
				connectedips_.erase(itt++); //C++03 fix
			}
			else
				++itt;
		}
	}

	c->start();
}

void connection_manager::stop(connection_ptr c)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	connections_.erase(c);
	c->stop();
}

void connection_manager::stop_all()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::for_each(connections_.begin(), connections_.end(),
		boost::bind(&connection::stop, _1));
}

} // namespace server
} // namespace http
