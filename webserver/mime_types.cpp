//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "mime_types.hpp"

namespace http
{
	namespace server
	{
		namespace mime_types
		{
			using mapping = std::pair<const char *, const char *>;
			constexpr auto mappings = std::array<mapping, 37>{
				mapping{ "gif", "image/gif" },
				mapping{ "htm", "text/html" },
				mapping{ "html", "text/html" },
				mapping{ "jpg", "image/jpeg" },
				mapping{ "png", "image/png" },
				mapping{ "css", "text/css" },
				mapping{ "xml", "application/xml" },
				mapping{ "js", "application/javascript" },
				mapping{ "json", "application/json" },
				mapping{ "swf", "application/x-shockwave-flash" },
				mapping{ "manifest", "text/cache-manifest" },
				mapping{ "appcache", "text/cache-manifest" },
				mapping{ "xls", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
				mapping{ "m3u", "audio/mpegurl" },
				mapping{ "mp3", "audio/mpeg" },
				mapping{ "ogg", "audio/ogg" },
				mapping{ "php", "text/html" },
				mapping{ "wav", "audio/x-wav" },
				mapping{ "svg", "image/svg+xml" },
				mapping{ "ico", "image/x-icon" },
				mapping{ "db", "application/octet-stream" },
				mapping{ "otf", "application/x-font-otf" },
				mapping{ "ttf", "application/x-font-ttf" },
				mapping{ "woff", "application/x-font-woff" },
				mapping{ "flv", "video/x-flv" },
				mapping{ "mp4", "video/mp4" },
				mapping{ "m3u8", "application/x-mpegURL" },
				mapping{ "ts", "video/MP2T" },
				mapping{ "3gp", "video/3gpp" },
				mapping{ "mov", "video/quicktime" },
				mapping{ "avi", "video/x-msvideo" },
				mapping{ "wmv", "video/x-ms-wmv" },
				mapping{ "h264", "video/h264" },
				mapping{ "mp4", "video/mp4" },
				mapping{ "wmv", "video/x-ms-wmv" },
				mapping{ "txt", "text/plain" },
				mapping{ "pdf", "application/pdf" },
			};

			std::string extension_to_type(const std::string &extension)
			{
				for (const auto &m : mappings)
				{
					if (m.first == extension)
					{
						return m.second;
					}
				}
				return ""; // RFC-7231 states that unknown files types should have not send any Content-Type header;
			}
		} // namespace mime_types
	}	  // namespace server
} // namespace http
