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
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "cWebem.h"

#define ZIPREADBUFFERSIZE (8192)

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
	int err=UNZ_OK;

	err = unzOpenCurrentFilePassword(uf,password);
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

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  std::string params="";

  size_t paramPos=request_path.find_first_of('?');
  if (paramPos!=std::string::npos)
  {
	  params=request_path.substr(paramPos+1);
	  request_path=request_path.substr(0,paramPos);
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
  const char *encoding_header;

  //check gzip support (only for .js and .htm(l) files
  if (
	  (request_path.find(".js")!=std::string::npos)||
	  (request_path.find(".htm")!=std::string::npos)
	  )
  {
	  if ((encoding_header = req.get_req_header(&req, "Accept-Encoding")) != NULL)
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
			  bHaveLoadedgzip=true;
			  // Fill out the reply to be sent to the client.
			  rep.status = reply::ok;
			  char buf[512];
			  while (is.read(buf, sizeof(buf)).gcount() > 0)
				  rep.content.append(buf, (unsigned int)is.gcount());
		  }
	  }
	  if (!bHaveLoadedgzip)
	  {
		  // Open the file to send back.
		  std::string full_path = doc_root_ + request_path;
		  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
		  if (!is)
		  {
			  rep = reply::stock_reply(reply::not_found);
			  return;
		  }

		  // Fill out the reply to be sent to the client.
		  rep.status = reply::ok;
		  char buf[512];
		  while (is.read(buf, sizeof(buf)).gcount() > 0)
			  rep.content.append(buf, (unsigned int)is.gcount());
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
			  if (do_extract_currentfile(m_uf,myWebem->m_zippassword.c_str(),rep.content)!=UNZ_OK)
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
		  if (do_extract_currentfile(m_uf,myWebem->m_zippassword.c_str(),rep.content)!=UNZ_OK)
		  {
			  rep = reply::stock_reply(reply::not_found);
			  return;
		  }
	  }
	  rep.status = reply::ok;

  }
#endif
  if (!bHaveLoadedgzip)
	rep.headers.resize(2);
  else
	rep.headers.resize(3);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
  if (bHaveLoadedgzip)
  {
	  rep.headers[2].name = "Content-Encoding";
	  rep.headers[2].value = "gzip";
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
