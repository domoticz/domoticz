//
// reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "reply.hpp"
#include "mime_types.hpp"
#include "utf.hpp"
#include <string>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace http {
namespace server {

namespace status_strings {

const std::string ok =
  "HTTP/1.1 200 OK\r\n";
const std::string created =
  "HTTP/1.1 201 Created\r\n";
const std::string accepted =
  "HTTP/1.1 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.1 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.1 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.1 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.1 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.1 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.1 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.1 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.1 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.1 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.1 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.1 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.1 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.1 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return boost::asio::buffer(ok);
  case reply::created:
    return boost::asio::buffer(created);
  case reply::accepted:
    return boost::asio::buffer(accepted);
  case reply::no_content:
    return boost::asio::buffer(no_content);
  case reply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case reply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case reply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case reply::not_modified:
    return boost::asio::buffer(not_modified);
  case reply::bad_request:
    return boost::asio::buffer(bad_request);
  case reply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case reply::forbidden:
    return boost::asio::buffer(forbidden);
  case reply::not_found:
    return boost::asio::buffer(not_found);
  case reply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case reply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case reply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case reply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::header_to_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(status_strings::to_buffer(status));
	for (std::size_t i = 0; i < headers.size(); ++i)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	return buffers;
}

std::vector<boost::asio::const_buffer> reply::to_buffers(const std::string &method)
{
	std::vector<boost::asio::const_buffer> buffers = header_to_buffers();
	if (method != "HEAD") {
		buffers.push_back(boost::asio::buffer(content));
	}
	return buffers;
}

void reply::reset()
{
	headers.clear();
	content = "";
	bIsGZIP = false;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  ""; // The 204 response MUST NOT contain a message-body
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  ""; // The 304 response MUST NOT contain a message-body
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
	reply rep;
	rep.status = status;
	rep.content = stock_replies::to_string(status);
	if (!rep.content.empty()) { // response can be empty (eg. HTTP 304)
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "text/html";
	}
	return rep;
}

void reply::add_header(reply *rep, const std::string &name, const std::string &value, bool replace)
{
	int num = rep->headers.size();
	if (replace) {
		for (int h = 0; h < num; h++) {
			if (boost::iequals(rep->headers[h].name, name)) {
				rep->headers[h].value = value;
				return;
			}
		}
	}
	rep->headers.resize(num + 1);
	rep->headers[num].name = name;
	rep->headers[num].value = value;
}

void reply::add_header_if_absent(reply *rep, const std::string &name, const std::string &value) {
	int num = rep->headers.size();
	for (int h = 0; h < num; h++) {
		if (boost::iequals(rep->headers[h].name, name)) {
			// is present
			return;
		}
	}
	add_header(rep, name, value, false);
}

void reply::set_content(reply *rep, const std::string & content) {
	rep->content.assign(content);
}

void reply::set_content(reply *rep, const std::wstring & content_w) {
	cUTF utf( content_w.c_str() );
	rep->content.assign(utf.get8(), strlen(utf.get8()));
}

bool reply::set_content_from_file(reply *rep, const std::string & file_path) {
	std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return false;
	file.seekg(0, std::ios::end);
	size_t fileSize = (size_t)file.tellg();
	if (fileSize > 0) {
		rep->content.resize(fileSize);
		file.seekg(0, std::ios::beg);
		file.read(&rep->content[0], rep->content.size());
	}
	file.close();
	return true;
}

bool reply::set_content_from_file(reply *rep, const std::string & file_path, const std::string & attachment, bool set_content_type) {
	if (!reply::set_content_from_file(rep, file_path))
		return false;
	reply::add_header_attachment(rep, attachment);
	if (set_content_type == true) {
		std::size_t last_dot_pos = attachment.find_last_of(".");
		if (last_dot_pos != std::string::npos) {
			std::string file_extension = attachment.substr(last_dot_pos + 1);
			std::string mime_type = mime_types::extension_to_type(file_extension);
			if ((mime_type.find("text/") >= 0) ||
					(mime_type.find("/xml") >= 0) ||
					(mime_type.find("/javascript") >= 0) ||
					(mime_type.find("/json") >= 0)) {
				// Add charset on text content
				mime_type += ";charset=UTF-8";
			}
			reply::add_header_content_type(rep, mime_type);
		}
	}
	return true;
}

void reply::add_header_attachment(reply *rep, const std::string & attachment) {
	reply::add_header(rep, "Content-Disposition", "attachment; filename=" + attachment);
}

void reply::add_header_content_type(reply *rep, const std::string & content_type) {
	reply::add_header(rep, "Content-Type", content_type);
}

} // namespace server
} // namespace http
