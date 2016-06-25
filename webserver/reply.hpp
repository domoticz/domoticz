//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef HTTP_REPLY_HPP
#define HTTP_REPLY_HPP

#include <string>
#include <vector>
#include <iterator>
#include <boost/asio.hpp>
#include "header.hpp"

namespace http {
namespace server {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
	switching_protocols = 101,
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;
  bool bIsGZIP;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> header_to_buffers();
  std::vector<boost::asio::const_buffer> to_buffers(const std::string &method);
  static void add_header(reply *rep, const std::string &name, const std::string &value, bool replace = true);
  static void add_header_if_absent(reply *rep, const std::string &name, const std::string &value);
  static void set_content(reply *rep, const std::string & content);
  static void set_content(reply *rep, const std::wstring & content_w);
  static bool set_content_from_file(reply *rep, const std::string & file_path);
  static bool set_content_from_file(reply *rep, const std::string & file_path, const std::string & attachment, bool set_content_type = false);
  static void add_header_attachment(reply *rep, const std::string & attachment);
  static void add_header_content_type(reply *rep, const std::string & content_type);

  template <class InputIterator>
  static void set_content(reply *rep, InputIterator first, InputIterator last) {
    rep->content.assign(first, last);
  }

  // we use this as an alternative for to_buffers()
  std::string header_to_string();
  std::string to_string(const std::string &method);

  // reset the reply, so we can re-use it during long-lived connections
  void reset();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

} // namespace server
} // namespace http

#endif // HTTP_REPLY_HPP
