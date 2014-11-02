//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>
#include "header.hpp"

namespace http {
namespace server {

/// A request received from a client.
class request
{
public:
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
  int content_length;				// the expected length of the contents
  std::string content;				// the contents

  static int mg_strcasecmp(const char *s1, const char *s2)
  {
	  int diff;

	  do {
		  diff = lowercase(s1++) - lowercase(s2++);
	  } while (diff == 0 && s1[-1] != '\0');

	  return diff;
  }
  static int lowercase(const char *s) {
	  return tolower(* (const unsigned char *) s);
  }

  // Return HTTP header value, or NULL if not found.
  static const char *get_req_header(const request *preq, const char *name) 
  {
	  std::vector<header>::const_iterator itt;

	  for (itt=preq->headers.begin(); itt!=preq->headers.end(); ++itt)
	  {
		  if (!mg_strcasecmp(name, itt->name.c_str()))
			  return itt->value.c_str();
	  }
	  return NULL;
  }

};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
