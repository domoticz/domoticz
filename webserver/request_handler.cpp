//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#ifdef HAVE_BOOST_FILESYSTEM
#include <boost/filesystem/operations.hpp>
#include <time.h>
#endif
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "cWebem.h"
#include "GZipHelper.h"

#define ZIPREADBUFFERSIZE (8192)

#ifdef HAVE_BOOST_FILESYSTEM
#define HTTP_DATE_RFC_1123 "%a, %d %b %Y %H:%M:%S %Z" // Sun, 06 Nov 1994 08:49:37 GMT
#define HTTP_DATE_RFC_850  "%A, %d-%b-%y %H:%M:%S %Z" // Sunday, 06-Nov-94 08:49:37 GMT
#define HTTP_DATE_ASCTIME  "%a %b %e %H:%M:%S %Y"     // Sun Nov  6 08:49:37 1994
#endif

#ifdef WIN32
// some ported functions
#define timegm _mkgmtime

extern "C" char* strptime(const char* s,
	const char* f,
struct tm* tm) {
	// Isn't the C++ standard lib nice? std::get_time is defined such that its
	// format parameters are the exact same as strptime. Of course, we have to
	// create a string stream first, and imbue it with the current C locale, and
	// we also have to make sure we return the right things if it fails, or
	// if it succeeds, but this is still far simpler an implementation than any
	// of the versions in any of the C standard libraries.
	std::istringstream input(s);
	input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
	input >> std::get_time(tm, f);
	if (input.fail()) {
		return nullptr;
	}
	return (char*)(s + input.tellg());
}
#endif

namespace http {
namespace server {


request_handler::request_handler(const std::string& doc_root, cWebem* webem)
  : doc_root_(doc_root), myWebem(webem)
{
#ifndef WEBSERVER_DONT_USE_ZIP
	m_uf=NULL;
	m_bIsZIP=(doc_root.find(".zip")!=std::string::npos)||(doc_root.find(".dat")!=std::string::npos);
	if (m_bIsZIP)
	{
		//open zip file, get file list
		fill_win32_filefunc(&m_ffunc);

		m_uf = unzOpen2(doc_root.c_str(),&m_ffunc);
	}
	m_pUnzipBuffer = (void*)malloc(ZIPREADBUFFERSIZE);
#endif
}

request_handler::~request_handler()
{
#ifndef WEBSERVER_DONT_USE_ZIP
	if (m_uf!=NULL)
	{
		unzClose(m_uf);
		m_uf=NULL;
	}
	if (m_pUnzipBuffer)
		free(m_pUnzipBuffer);
	m_pUnzipBuffer=NULL;
#endif
}

#ifndef WEBSERVER_DONT_USE_ZIP
int request_handler::do_extract_currentfile(unzFile uf, const char* password, std::string &outputstr)
{
	int err = unzOpenCurrentFilePassword(uf, password);
	if (err!=UNZ_OK)
		return err;

	do
	{
		err = unzReadCurrentFile(uf,m_pUnzipBuffer,ZIPREADBUFFERSIZE);
		if (err<0)
		{
			//printf("error %d with zipfile in unzReadCurrentFile\n",err);
			break;
		}
		if (err>0)
			outputstr.append((char*)m_pUnzipBuffer, (unsigned int)err);
	}
	while (err>0);
	unzCloseCurrentFile (uf);
	return err;
}
#endif

#ifdef HAVE_BOOST_FILESYSTEM
static time_t convert_from_http_date(const std::string &str)
{
	if (str.empty())
		return 0;

	struct tm time_data;
	memset(&time_data, 0, sizeof(struct tm));

	bool success = strptime(str.c_str(), HTTP_DATE_RFC_1123, &time_data)
		|| strptime(str.c_str(), HTTP_DATE_RFC_850, &time_data)
		|| strptime(str.c_str(), HTTP_DATE_ASCTIME, &time_data);

	if (!success)
		return 0;

	return timegm(&time_data);
}

static std::string convert_to_http_date(time_t time)
{
	struct tm *tm_result = gmtime(&time);

	if (!tm_result)
		return std::string();

	char buffer[1024];
	strftime(buffer, sizeof(buffer), HTTP_DATE_RFC_1123, tm_result);
	buffer[sizeof(buffer) - 1] = '\0';

	return buffer;
}

bool request_handler::not_modified(std::string full_path, const request &req, reply &rep)
{
	const char *if_modified = request::get_req_header(&req, "If-Modified-Since");
	boost::filesystem::path path(full_path);
	time_t last_written_time = boost::filesystem::last_write_time(path);
	// propagate timestamp to browser
	reply::AddHeader(&rep, "Last-Modified", convert_to_http_date(last_written_time));
	if (NULL == if_modified) {
		// we have no if-modified header, continue to serve content
		return false;
	}
	time_t if_modified_since_time = convert_from_http_date(if_modified);
	if (if_modified_since_time >= last_written_time) {
		// content has not been modified since last serve
		// indicate to use a cached copy to browser
		reply::AddHeader(&rep, "Connection", "Keep-Alive");
		reply::AddHeader(&rep, "Keep-Alive", "max=20, timeout=60");
		rep.content = "";
		rep.status = reply::not_modified;
		return true;
	}
	// force new content
	return false;
}
#endif

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }
  if (request_path.find(".htpasswd")!=std::string::npos)
  {
	  rep = reply::stock_reply(reply::bad_request);
	  return;
  }

  int paramPos = request_path.find_first_of('?');
  if (paramPos != std::string::npos)
  {
	  request_path = request_path.substr(0, paramPos);
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  if (request_path.find("/@login")==0)
	  request_path="/";

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }
  else if (myWebem && request_path.find("/acttheme/") == 0)
  {
	  request_path = myWebem->m_actTheme + request_path.substr(9);
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  bool bHaveGZipSupport=false;

  //check gzip support (only for .js and .htm(l) files
  if (
	  (request_path.find(".js")!=std::string::npos)||
	  (request_path.find(".htm")!=std::string::npos)
	  )
  {
	  const char *encoding_header;
	  if ((encoding_header = request::get_req_header(&req, "Accept-Encoding")) != NULL)
	  {
		  //see if we support gzip
		  bHaveGZipSupport=(strstr(encoding_header,"gzip")!=NULL);
	  }
  }

  bool bHaveLoadedgzip=false;
#ifndef WEBSERVER_DONT_USE_ZIP
  if (!m_bIsZIP)
#endif
  {
	  //first try gzip version
	  if (bHaveGZipSupport)
	  {
		  // Open the file to send back.
		  std::string full_path = doc_root_ + request_path + ".gz";
		  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
		  if (is)
		  {
#ifdef HAVE_BOOST_FILESYSTEM
			  // check if file date is still the same since last request
			  if (not_modified(full_path, req, rep)) {
				  return;
			  }
#endif
			  bHaveLoadedgzip=true;
			  rep.bIsGZIP=true;
			  // Fill out the reply to be sent to the client.
			  rep.status = reply::ok;
			  rep.content.append((std::istreambuf_iterator<char>(is)),
				  (std::istreambuf_iterator<char>()));
		  }
	  }
	  if (!bHaveLoadedgzip)
	  {
		  // Open the file to send back.
		  std::string full_path = doc_root_ + request_path;
		  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
		  if (!is)
		  {
			  //maybe its a gz file (and clients browser does not support compression)
			  full_path += ".gz";
			  is.open(full_path.c_str(), std::ios::in | std::ios::binary);
			  if (is.is_open())
			  {
#ifdef HAVE_BOOST_FILESYSTEM
				  // check if file date is still the same since last request
				  if (not_modified(full_path, req, rep)) {
					  return;
				  }
#endif
				  bHaveLoadedgzip = true;
				  std::string gzcontent((std::istreambuf_iterator<char>(is)),
					  (std::istreambuf_iterator<char>()));

				  CGZIP2AT<> decompress((LPGZIP)gzcontent.c_str(), gzcontent.size());

				  rep.status = reply::ok;
				  // Fill out the reply to be sent to the client.
				  rep.content.append(decompress.psz, decompress.Length);
			  }
			  else
			  {
				  //Maybe it is a folder, lets add the index file
				  if (full_path.find('.') != std::string::npos)
				  {
					  rep = reply::stock_reply(reply::not_found);
					  return;
				  }
				  request_path += "/index.html";
				  full_path = doc_root_ + request_path;
				  is.open(full_path.c_str(), std::ios::in | std::ios::binary);
				  if (!is.is_open())
				  {
					  rep = reply::stock_reply(reply::not_found);
					  return;
				  }
				  extension = "html";
			  }
#if 0
			  // unfortunately, we cannot cache these files, because there might be
			  // include codes in it, but otherwise here is the place to add it
#ifdef HAVE_BOOST_FILESYSTEM
			  if (not_modified(full_path, req, rep)) {
				return;
			  }
#endif
#endif
		  }
		  if (!bHaveLoadedgzip)
		  {
			  // Fill out the reply to be sent to the client.
			  rep.status = reply::ok;
			  rep.content.append((std::istreambuf_iterator<char>(is)),
				  (std::istreambuf_iterator<char>()));
		  }
	  }
  }
#ifndef WEBSERVER_DONT_USE_ZIP
  else
  {
	  if (m_uf==NULL)
	  {
		  rep = reply::stock_reply(reply::not_found);
		  return;
	  }

	  //remove first /
	  request_path=request_path.substr(1);
	  if (bHaveGZipSupport)
	  {
		  std::string gzpath = request_path + ".gz";
		  if (unzLocateFile(m_uf,gzpath.c_str(),0)==UNZ_OK)
		  {
			  if (myWebem && do_extract_currentfile(m_uf,myWebem->m_zippassword.c_str(),rep.content)!=UNZ_OK)
			  {
				  rep = reply::stock_reply(reply::not_found);
				  return;
			  }
			  bHaveLoadedgzip=true;
		  }
	  }
	  if (!bHaveLoadedgzip)
	  {
		  if (unzLocateFile(m_uf,request_path.c_str(),0)!=UNZ_OK)
		  {
			  rep = reply::stock_reply(reply::not_found);
			  return;
		  }
		  if (myWebem && do_extract_currentfile(m_uf,myWebem->m_zippassword.c_str(),rep.content)!=UNZ_OK)
		  {
			  rep = reply::stock_reply(reply::not_found);
			  return;
		  }
	  }
	  rep.status = reply::ok;

  }
#endif
  int ExtraHeaders = 0;
  if (req.keep_alive)
  {
	  ExtraHeaders += 2;
  }
  if (!bHaveLoadedgzip)
	  rep.headers.resize(3 + ExtraHeaders);
  else
	  rep.headers.resize(4 + ExtraHeaders);
  
  int iHeader = 0;

  rep.headers[iHeader].name = "Content-Length";
  rep.headers[iHeader++].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[iHeader].name = "Content-Type";
  rep.headers[iHeader++].value = mime_types::extension_to_type(extension);
  rep.headers[iHeader].name = "Access-Control-Allow-Origin";
  rep.headers[iHeader++].value = "*";
  if (req.keep_alive)
  {
	  rep.headers[iHeader].name = "Connection";
	  rep.headers[iHeader++].value = "Keep-Alive";
	  rep.headers[iHeader].name = "Keep-Alive";
	  rep.headers[iHeader++].value = "max=20, timeout=10";
  }
  if (bHaveLoadedgzip)
  {
	  rep.headers[iHeader].name = "Content-Encoding";
	  rep.headers[iHeader++].value = "gzip";
  }
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server
} // namespace http
