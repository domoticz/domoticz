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
#include <boost/algorithm/string.hpp>

namespace http {
namespace server {

namespace status_strings {

	constexpr auto switching_protocols = "HTTP/1.1 101 Switching Protocols\r\n";
	constexpr auto download_file = "HTTP/1.1 102 Download File\r\n";
	constexpr auto ok = "HTTP/1.1 200 OK\r\n";
	constexpr auto created = "HTTP/1.1 201 Created\r\n";
	constexpr auto accepted = "HTTP/1.1 202 Accepted\r\n";
	constexpr auto no_content = "HTTP/1.1 204 No Content\r\n";
	constexpr auto multiple_choices = "HTTP/1.1 300 Multiple Choices\r\n";
	constexpr auto moved_permanently = "HTTP/1.1 301 Moved Permanently\r\n";
	constexpr auto moved_temporarily = "HTTP/1.1 302 Moved Temporarily\r\n";
	constexpr auto not_modified = "HTTP/1.1 304 Not Modified\r\n";
	constexpr auto bad_request = "HTTP/1.1 400 Bad Request\r\n";
	constexpr auto unauthorized = "HTTP/1.1 401 Unauthorized\r\n";
	constexpr auto forbidden = "HTTP/1.1 403 Forbidden\r\n";
	constexpr auto not_found = "HTTP/1.1 404 Not Found\r\n";
	constexpr auto internal_server_error = "HTTP/1.1 500 Internal Server Error\r\n";
	constexpr auto not_implemented = "HTTP/1.1 501 Not Implemented\r\n";
	constexpr auto bad_gateway = "HTTP/1.1 502 Bad Gateway\r\n";
	constexpr auto service_unavailable = "HTTP/1.1 503 Service Unavailable\r\n";

	std::string to_string(const reply::status_type &status)
	{
		switch (status)
		{
			case reply::switching_protocols:
				return switching_protocols;
			case reply::download_file:
				return download_file;
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

} // namespace status_strings

namespace misc_strings {

	constexpr char name_value_separator[] = { ':', ' ', 0 };
	constexpr char crlf[] = { '\r', '\n', 0 };

} // namespace misc_strings

std::string reply::header_to_string()
{
	std::string buffers = status_strings::to_string(status);
	for (const auto &h : headers)
		buffers += h.name + misc_strings::name_value_separator + h.value + misc_strings::crlf;

	buffers += misc_strings::crlf;
	return buffers;
}

std::string reply::to_string(const std::string &method)
{
	std::string buffers = header_to_string();
	if (method != "HEAD") {
		buffers += content;
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

	constexpr auto switching_protocols = "";
	constexpr auto download_file = "";
	constexpr auto ok = "";
	constexpr auto created = "<html>"
				 "<head><title>Created</title></head>"
				 "<body><h1>201 Created</h1></body>"
				 "</html>";
	constexpr auto accepted = "<html>"
				  "<head><title>Accepted</title></head>"
				  "<body><h1>202 Accepted</h1></body>"
				  "</html>";
	constexpr auto no_content = ""; // The 204 response MUST NOT contain a message-body
	constexpr auto multiple_choices = "<html>"
					  "<head><title>Multiple Choices</title></head>"
					  "<body><h1>300 Multiple Choices</h1></body>"
					  "</html>";
	constexpr auto moved_permanently = "<html>"
					   "<head><title>Moved Permanently</title></head>"
					   "<body><h1>301 Moved Permanently</h1></body>"
					   "</html>";
	constexpr auto moved_temporarily = "<html>"
					   "<head><title>Moved Temporarily</title></head>"
					   "<body><h1>302 Moved Temporarily</h1></body>"
					   "</html>";
	constexpr auto not_modified = ""; // The 304 response MUST NOT contain a message-body
	constexpr auto bad_request = "<html>"
				     "<head><title>Bad Request</title></head>"
				     "<body><h1>400 Bad Request</h1></body>"
				     "</html>";
	constexpr auto unauthorized = "<html>"
				      "<head><title>Unauthorized</title></head>"
				      "<body><h1>401 Unauthorized</h1></body>"
				      "</html>";
	constexpr auto forbidden = "<html>"
				   "<head><title>Forbidden</title></head>"
				   "<body><h1>403 Forbidden</h1></body>"
				   "</html>";
	constexpr auto not_found = "<html>"
				   "<head><title>Not Found</title></head>"
				   "<body><h1>404 Not Found</h1></body>"
				   "</html>";
	constexpr auto internal_server_error = "<html>"
					       "<head><title>Internal Server Error</title></head>"
					       "<body><h1>500 Internal Server Error</h1></body>"
					       "</html>";
	constexpr auto not_implemented = "<html>"
					 "<head><title>Not Implemented</title></head>"
					 "<body><h1>501 Not Implemented</h1></body>"
					 "</html>";
	constexpr auto bad_gateway = "<html>"
				     "<head><title>Bad Gateway</title></head>"
				     "<body><h1>502 Bad Gateway</h1></body>"
				     "</html>";
	constexpr auto service_unavailable = "<html>"
					     "<head><title>Service Unavailable</title></head>"
					     "<body><h1>503 Service Unavailable</h1></body>"
					     "</html>";

	std::string to_string(const reply::status_type &status)
	{
		switch (status)
		{
			case reply::switching_protocols:
				return switching_protocols;
			case reply::download_file:
				return download_file;
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
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "text/html";
	}
	return rep;
}

void reply::add_header(reply *rep, const std::string &name, const std::string &value, bool replace)
{
	size_t num = rep->headers.size();
	if (replace) {
		for (auto &h : rep->headers)
		{
			if (boost::iequals(h.name, name))
			{
				h.value = value;
				return;
			}
		}
	}
	rep->headers.resize(num + 1);
	rep->headers[num].name = name;
	rep->headers[num].value = value;
}

void reply::add_header_if_absent(reply *rep, const std::string &name, const std::string &value)
{
	for (const auto &h : rep->headers)
	{
		if (boost::iequals(h.name, name))
		{
			// is present
			return;
		}
	}
	add_header(rep, name, value, false);
}

void reply::set_content(reply *rep, const std::string &content)
{
	rep->content.assign(content);
}

void reply::set_content(reply *rep, const std::wstring &content_w)
{
	cUTF utf( content_w.c_str() );
	rep->content.assign(utf.get8(), strlen(utf.get8()));
}

bool reply::set_content_from_file(reply *rep, const std::string &file_path)
{
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

bool reply::set_content_from_file(reply *rep, const std::string &file_path, const std::string &attachment, bool set_content_type)
{
	if (!reply::set_content_from_file(rep, file_path))
		return false;
	reply::add_header_attachment(rep, attachment);
	if (set_content_type == true) {
		std::size_t last_dot_pos = attachment.find_last_of('.');
		if (last_dot_pos != std::string::npos) {
			std::string file_extension = attachment.substr(last_dot_pos + 1);
			std::string mime_type = mime_types::extension_to_type(file_extension);
			if ((mime_type.find("text/") != std::string::npos) ||
					(mime_type.find("/xml") != std::string::npos) ||
					(mime_type.find("/javascript") != std::string::npos) ||
					(mime_type.find("/json") != std::string::npos)) {
				// Add charset on text content
				mime_type += ";charset=UTF-8";
			}
			reply::add_header_content_type(rep, mime_type);
		}
	}
	return true;
}

bool reply::set_download_file(reply* rep, const std::string& file_path, const std::string& attachment)
{
	if (file_path.empty() || attachment.empty())
		return false;
	rep->reset();
	rep->status = reply::status_type::download_file;
	rep->content = file_path + "\r\n" + attachment;
	return true;
}

void reply::add_header_attachment(reply *rep, const std::string &attachment)
{
	reply::add_header(rep, "Content-Disposition", "attachment; filename=" + attachment);
}

/*
RFC-7231
Hypertext Transfer Protocol (HTTP/1.1): Semantics and Content
3.1.1.5. Content-Type

A sender that generates a message containing a payload body SHOULD
generate a Content-Type header field in that message unless the
intended media type of the enclosed representation is unknown to the
sender.
*/
void reply::add_header_content_type(reply *rep, const std::string & content_type) {
	if (!content_type.empty())
		reply::add_header(rep, "Content-Type", content_type);
}

} // namespace server
} // namespace http
