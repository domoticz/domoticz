//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include "header.hpp"

namespace http {
namespace server {

/// A request received from a client.
class request
{
public:
	std::string host_address;
	std::string host_port;
	std::string method;
	std::string uri;
	int http_version_major;
	int http_version_minor;
	std::vector<header> headers;
	int content_length;				// the expected length of the contents
	std::string content;				// the contents
	bool keep_alive;					// send Keep-Alive header

	/// store map between pages and application functions (wide char)
	std::multimap<std::string, std::string> parameters;


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

	/** Find the value of a name set by a form submit action */
	static std::string print(const request *preq) {
		std::string buffer;
		buffer.append("[\n");
		std::multimap<std::string, std::string>::const_iterator iter;
		for (iter = preq->parameters.begin(); iter != preq->parameters.end(); ++iter) {
			buffer.append((*iter).first);
			buffer.append(":");
			buffer.append((*iter).second);
			buffer.append(",\n");
		}
		buffer.append("]");
		static std::string ret = buffer;
		return ret;
	}

	/** Check existing values */
	static bool hasParams(const request *preq) {
		return !preq->parameters.empty();
	};

	/** Find the value of a name set by a form submit action */
	static std::string findValue(const request *preq, const char* name) {
		return findValue(&preq->parameters, name);
	}

	/** Find the value of a name set by a form submit action */
	static std::string findValue(
			const std::multimap<std::string, std::string> *values,
			const char* name) {
		std::string ret;
		ret = "";
		std::multimap<std::string, std::string>::const_iterator iter;
		iter = values->find(name);
		if (iter != values->end()) {
			try {
				ret = iter->second;
			} catch (...) {

			}
		}
		return ret;
	}

	static bool hasValue(const request *preq, const char* name) {
		std::multimap<std::string, std::string>::const_iterator iter;
		iter = preq->parameters.find(name);
		return (iter != preq->parameters.end());
	}

	static void makeValuesFromPostContent(const request *preq, std::multimap<std::string, std::string> &values)
	{
		values.clear();
		std::string name;
		std::string value;

		std::string uri = preq->content;
		size_t q = 0;
		size_t p = q;
		int flag_done = 0;
		while (!flag_done) {
			q = uri.find("=", p);
			if (q == std::string::npos) {
				return;
			}
			name = uri.substr(p, q - p);
			p = q + 1;
			q = uri.find("&", p);
			if (q != std::string::npos) {
				value = uri.substr(p, q - p);
			} else {
				value = uri.substr(p);
				flag_done = 1;
			}
			// the browser sends blanks as +
			while (1) {
				size_t p2 = value.find("+");
				if (p2 == std::string::npos) {
					break;
				}
				value.replace(p2, 1, " ");
			}

			values.insert(std::pair< std::string, std::string >(name, value));
			p = q + 1;
		}
	}

};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
