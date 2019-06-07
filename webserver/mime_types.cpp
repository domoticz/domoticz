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

namespace http {
namespace server {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" },
  { "css", "text/css" },
  { "xml", "application/xml" },
  { "js", "application/javascript" },
  { "json", "application/json" },
  { "swf", "application/x-shockwave-flash" },
  { "manifest", "text/cache-manifest" },
  { "appcache", "text/cache-manifest" },
  { "xls", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
  { "m3u", "audio/mpegurl" },
  { "mp3", "audio/mpeg" },
  { "ogg", "audio/ogg" },
  { "php", "text/html" },
  { "wav", "audio/x-wav" },
  { "svg", "image/svg+xml" },
  { "ico", "image/x-icon" },
  { "db", "application/octet-stream" },
  { "otf", "application/x-font-otf" },
  { "ttf", "application/x-font-ttf" },
  { "woff", "application/x-font-woff" },
  { "flv", "video/x-flv" },
  { "mp4", "video/mp4" },
  { "m3u8", "application/x-mpegURL" },
  { "ts", "video/MP2T" },
  { "3gp", "video/3gpp" },
  { "mov", "video/quicktime" },
  { "avi", "video/x-msvideo" },
  { "wmv", "video/x-ms-wmv" },
  { "h264", "video/h264" },
  { "mp4", "video/mp4" },
  { "wmv", "video/x-ms-wmv" },
  { "txt", "text/plain" },
  { "pdf", "application/pdf" },
  { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping* m = mappings; m->extension; ++m)
  {
    if (m->extension == extension)
    {
      return m->mime_type;
    }
  }

  return "";// RFC-7231 states that unknown files types should have not send any Content-Type header;
}

} // namespace mime_types
} // namespace server
} // namespace http
