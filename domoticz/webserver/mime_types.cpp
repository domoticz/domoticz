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
  { "xml", "application/css" },
  { "js", "text/javascript" },
  { "swf", "application/x-shockwave-flash" },
  { "manifest", "text/cache-manifest" },
  { "appcache", "text/cache-manifest" },
  { "xls", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
  { "m3u", "audio/mpegurl" },
  { "wav", "audio/x-wav" },
  { "db", "application/octet-stream" },
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

  return "text/plain";
}

} // namespace mime_types
} // namespace server
} // namespace http
