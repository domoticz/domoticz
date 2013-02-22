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

namespace http {
namespace server {

void connection_manager::start(connection_ptr c)
{
  connections_.insert(c);
  std::string s = c->socket().remote_endpoint().address().to_string();

  if (connectedips_.find(s)==connectedips_.end())
  {
	  //ok, this could get a very long list when running for years
	  connectedips_.insert(s);
	  _log.Log(LOG_NORM,"Incoming connection from: %s", s.c_str());
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

void connection_manager::check_timeouts()
{
	time_t atime=time(NULL);
	std::set<connection_ptr>::const_iterator itt;
	for (itt=connections_.begin(); itt!=connections_.end(); ++itt)
	{
		if (atime-(*itt)->m_lastresponse>20*60)
		{
			stop(*itt);
			itt=connections_.begin();
		}
	}
}

} // namespace server
} // namespace http
