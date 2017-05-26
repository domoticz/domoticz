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
#include "fastcgi.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#ifdef WIN32
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/date.hpp>
#endif
#include <time.h>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "cWebem.h"
#include "GZipHelper.h"

// remove
#include "../main/Logger.h"

#define ZIPREADBUFFERSIZE (8192)

#define HTTP_DATE_RFC_1123 "%a, %d %b %Y %H:%M:%S %Z" // Sun, 06 Nov 1994 08:49:37 GMT
#define HTTP_DATE_RFC_850  "%A, %d-%b-%y %H:%M:%S %Z" // Sunday, 06-Nov-94 08:49:37 GMT
#define HTTP_DATE_ASCTIME  "%a %b %e %H:%M:%S %Y"     // Sun Nov  6 08:49:37 1994

#ifdef WIN32
// some ported functions
#define timegm _mkgmtime
#define stat _stat

extern "C" const char* strptime(const char* s, const char* f, struct tm* tm)
{
	try {
		boost::posix_time::ptime d;
		std::stringstream ss;
		ss.exceptions(std::ios_base::failbit);
		boost::local_time::local_time_input_facet *input_facet = new boost::local_time::local_time_input_facet(f);
		ss.imbue(std::locale(ss.getloc(), input_facet));
		ss.str(s);
		ss >> d; // throws bad date exception AND sets failbit on stream
		*tm = boost::posix_time::to_tm(d);
		return s + strlen(s);
	}
	catch (...) {
	}
	return NULL;
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

static time_t last_write_time(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		return st.st_mtime;
	}
	_log.Log(LOG_ERROR, "stat returned errno = %d", errno);
	return 0;
}

bool request_handler::not_modified(std::string full_path, const request &req, reply &rep, modify_info &mInfo)
{
	mInfo.last_written = last_write_time(full_path);
	if (mInfo.last_written == 0) {
		// file system doesn't support this, don't enable header
		mInfo.mtime_support = false;
		mInfo.is_modified = true;
		return false;
	}
	mInfo.mtime_support = true;
	// propagate timestamp to browser
	reply::add_header(&rep, "Last-Modified", convert_to_http_date(mInfo.last_written));
	const char *if_modified = request::get_req_header(&req, "If-Modified-Since");
	if (NULL == if_modified) {
		// we have no if-modified header, continue to serve content
		mInfo.is_modified = true;
		//_log.Log(LOG_STATUS, "%s: No If-Modified-Since header", full_path.c_str());
		return false;
	}
	time_t if_modified_since_time = convert_from_http_date(if_modified);
	if (if_modified_since_time >= mInfo.last_written) {
		mInfo.is_modified = false;
		if (mInfo.delay_status) {
			//_log.Log(LOG_STATUS, "%s: Delaying status code", full_path.c_str());
			return false;
		}
		else {
			rep = reply::stock_reply(reply::not_modified);
			//_log.Log(LOG_STATUS, "%s: Setting status code reply::not_modified", full_path.c_str());
			return true;
		}
	}
	mInfo.is_modified = true;
	// file is newer, force new content
	//_log.Log(LOG_STATUS, "%s: Force content", full_path.c_str());
	return false;
}

void request_handler::handle_request(const request& req, reply& rep)
{
	modify_info mInfo;
	handle_request(req, rep, mInfo);
}

void request_handler::handle_request(const request &req, reply &rep, modify_info &mInfo)
{
  mInfo.mtime_support = false;
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  if (myWebem->IsBadRequestPath(request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  request_path = myWebem->ExtractRequestPath(request_path);

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
	  if (extension == "php")
	  {
		  if (!myWebem->m_settings.is_php_enabled())
		  {
			  rep = reply::stock_reply(reply::not_implemented);
			  return;
		  }

		  //
		  //For now this is very simplistic!
		  //Later we should add FastCGI support, or at least provide some environment variables
		  fastcgi_parser::handlePHP(myWebem->m_settings, request_path, req, rep, mInfo);
		  return;
	  }
	  else
	  {
		  //first try gzip version
		  if (bHaveGZipSupport)
		  {
			  std::string full_path = doc_root_ + request_path + ".gz";
			  // Open the file to send back.
			  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
			  if (is)
			  {
				  mInfo.delay_status = false;
				  if (not_modified(full_path, req, rep, mInfo)) {
					  return;
				  }
				  // check if file date is still the same since last request
				  bHaveLoadedgzip = true;
				  rep.bIsGZIP = true;
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
					  // check if file date is still the same since last request
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
#ifdef _DEBUG
						  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d) (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());
#endif
						  return;
					  }
					  request_path += "/index.html";
					  full_path = doc_root_ + request_path;
					  is.open(full_path.c_str(), std::ios::in | std::ios::binary);
					  if (!is.is_open())
					  {
						  rep = reply::stock_reply(reply::not_found);
#ifdef _DEBUG
						  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d)  (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());
#endif
						  return;
					  }
					  extension = "html";
				  }
			  }
			  if (!bHaveLoadedgzip)
			  {
				  // Fill out the reply to be sent to the client.
				  rep.status = reply::ok;
				  rep.content.append((std::istreambuf_iterator<char>(is)),
					  (std::istreambuf_iterator<char>()));
				  // set delay_status to true, because it can possibly have
				  // include codes in it.
				  mInfo.delay_status = true;
				  if (not_modified(full_path, req, rep, mInfo)) {
					  return;
				  }
			  }
		  }
	  }
  }
#ifndef WEBSERVER_DONT_USE_ZIP
  else
  {
	  if (m_uf==NULL)
	  {
		  rep = reply::stock_reply(reply::not_found);
#ifdef _DEBUG
		  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d) (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());

#endif
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
#ifdef _DEBUG
				  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d) (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());
#endif
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
#ifdef _DEBUG
			  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d) (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());
#endif
			  return;
		  }
		  if (myWebem && do_extract_currentfile(m_uf,myWebem->m_zippassword.c_str(),rep.content)!=UNZ_OK)
		  {
			  rep = reply::stock_reply(reply::not_found);
#ifdef _DEBUG
			  _log.Log(LOG_ERROR, "Webserver: File '%s': %s (%d) (remote address: %s)", request_path.c_str(), strerror(errno), errno, req.host_address.c_str());
#endif
			  return;
		  }
	  }
	  rep.status = reply::ok;

  }
#endif

  reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
  reply::add_header(&rep, "Content-Type", mime_types::extension_to_type(extension));
  reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
  if (bHaveGZipSupport && bHaveLoadedgzip)
  {
	reply::add_header(&rep, "Content-Encoding", "gzip");
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

cWebem* request_handler::Get_myWebem()
{
	return myWebem;
}

} // namespace server
} // namespace http
