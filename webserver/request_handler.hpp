//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#ifndef WEBSERVER_DONT_USE_ZIP
	#include "zip/unzip.h"
	#define USEWIN32IOAPI
	#include "zip/iowin32.h"
#endif

namespace http {
namespace server {

struct reply;
class request;
class cWebem;

struct modify_info {
	bool delay_status;
	bool mtime_support;
	bool is_modified;
	time_t last_written;
};

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:
  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root, cWebem* webem);
  ~request_handler();

  /// Handle a request and produce a reply.
  virtual void handle_request(const request& req, reply& rep);
  virtual void handle_request(const request & req, reply & rep, modify_info & mInfo);

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
  
  /// The directory containing the files to be served.
  std::string doc_root_;

  // expose myWebem so we can use it in websocket connections
  cWebem* Get_myWebem();

protected:
  // Webem link to application code
  cWebem* myWebem;

private:
	bool not_modified(std::string full_path, const request &req, reply &rep, modify_info &mInfo);
	//zip support
#ifndef WEBSERVER_DONT_USE_ZIP
	  zlib_filefunc_def m_ffunc;
	  unzFile m_uf;
	  bool m_bIsZIP;
	  void *m_pUnzipBuffer;
	  int do_extract_currentfile(unzFile uf, const char* password, std::string &outputstr);
#endif
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP
