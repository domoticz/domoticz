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
#include <iostream>
#include "../main/Logger.h"

namespace http {
namespace server {

	void connection_manager::start(const connection_ptr &c)
	{
		connections_.insert(c);
		c->start();
	}

	void connection_manager::stop(const connection_ptr &c)
	{
		connections_.erase(c);
		c->stop();
	}

void connection_manager::stop_all()
{
	for (const auto &con : connections_)
	{
		con->stop();
	}
	connections_.clear();
}


} // namespace server
} // namespace http
