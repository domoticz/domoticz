#include "stdafx.h"
#if defined(WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mynetwork.h"
#include "sock_port.h"

#include "mUri.h"

#if !defined(WIN32)
static int strnicmp( char const * a, char const * b, int n ) {
  return strncasecmp( a, b, n );
}
#else
#pragma warning(disable: 4996)
#endif


namespace {


class HTTPQuery : public I_HTTPRequest {
  public:
    HTTPQuery() {
      complete_ = false;
      socket_ = BAD_SOCKET_FD;
    }
    ~HTTPQuery() {
      if( socket_ != BAD_SOCKET_FD ) {
        ::closesocket( socket_ );
      }
	  std::vector<_tHTTPVecBuffer>::iterator itt;
	  for (itt=m_buffers.begin(); itt!=m_buffers.end(); ++itt)
	  {
		  delete (*itt).pData;
	  }
	  m_buffers.clear();
    }

    void setQuery( char const * host, unsigned short port, char const * url ) {
      if( strlen( url ) > 1536 || strlen( host ) > 256 ) {
        return;
      }
      struct hostent * hent = gethostbyname( host );
      if( hent == 0 ) {
        complete_ = true;
        return;
      }
      addr_.sin_family = AF_INET;
      addr_.sin_addr = *(in_addr *)hent->h_addr_list[0];
      addr_.sin_port = htons( port );
      socket_ = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
      if( socket_ == BAD_SOCKET_FD ) {
        complete_ = true;
        return;
      }
      int r;
      r = ::connect( socket_, (sockaddr *)&addr_, sizeof( addr_ ) );
      if( r < 0 ) {
        complete_ = true;
        return;
      }
      MAKE_SOCKET_NONBLOCKING( socket_, r );
      if( r < 0 ) {
        complete_ = true;
        return;
      }
      char buf[2048];
      sprintf( buf, "GET %s HTTP/1.0\r\nUser-Agent: Domoticz\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\n\r\n",
          url, host );
      r = ::send( socket_, buf, int(strlen( buf )), NONBLOCK_MSG_SEND );
      if( r != strlen( buf ) ) {
        complete_ = true;
        return;
      }
    }

    void dispose() {
      delete this;
    }
    bool complete() {
      return complete_;
    }
    int read( void * ptr, size_t size ) 
	{
		int r = ::recv( socket_, (char*)ptr, size,0);
		if( r <= 0 ) 
		{
			if( !SOCKET_WOULDBLOCK_ERROR( SOCKET_ERRNO ) ) {
				complete_ = true;
			}
		}
		return r;
    }
	bool readDataInVecBuffer()
	{
		char buf[4096];
		bool bFirstTime=true;
		time_t t=0;
		while( true ) 
		{
			int rd = read( buf, sizeof(buf) );
			if( rd > 0 ) {
				int offset=0;
				if (bFirstTime)
				{
					bFirstTime=false;
					//search for the end of the HTTP header
					while (offset<rd-4)
					{
						if (
							(buf[offset]==0x0d)&&
							(buf[offset+1]==0x0a)&&
							(buf[offset+2]==0x0d)&&
							(buf[offset+3]==0x0a)
							)
						{
							//found it
							offset+=4;
							break;
						}
						offset++;
					}

				}
				//do something with the data
				_tHTTPVecBuffer vbuffer;
				vbuffer.usLength = rd-offset;
				vbuffer.pData = new unsigned char[vbuffer.usLength];
				memcpy(vbuffer.pData,buf+offset,vbuffer.usLength);
				m_buffers.push_back(vbuffer);
			}
			else
			{
				if (complete())
					break;
				if( !t ) {
					time( &t );
				}
				else {
					time_t t2;
					time( &t2 );
					if( t2 > t+5 ) {
						//timeout
						return false;
					}
				}

			}
		}
		return complete();
	}
	bool getBuffer(unsigned char *&pDest, unsigned long &ulLength)
	{
		std::vector<_tHTTPVecBuffer>::iterator itt;

		//First get total length
		ulLength=0;
		for (itt=m_buffers.begin(); itt!=m_buffers.end(); ++itt)
		{
			ulLength+=(*itt).usLength;
		}
		pDest=new unsigned char[ulLength+1];
		if (pDest==NULL)
			return false;

		//now copy the buffers in our output buffer
		unsigned long offset=0;
		for (itt=m_buffers.begin(); itt!=m_buffers.end(); ++itt)
		{
			memcpy(pDest+offset,(*itt).pData,(*itt).usLength);
			offset+=(*itt).usLength;
		}
		pDest[ulLength]=0;
		return true;
	}
    bool complete_;
    SOCKET socket_;
    sockaddr_in addr_;
};

};

I_HTTPRequest * NewHTTPRequest( char const * url )
{
  static bool socketsInited=false;
  if( !socketsInited ) {
    socketsInited = true;
    INIT_SOCKET_LIBRARY();
  }

  Uri u0=Uri::Parse(url);
  if (u0.Host.size()<1)
	  return 0;
  if (u0.Path=="")
	  u0.Path="/";

  std::string path=u0.Path+u0.QueryString;
  int iPort=atoi(u0.Port.c_str());

  HTTPQuery * q = new HTTPQuery;
  q->setQuery( u0.Host.c_str(), iPort, path.c_str() );
  return q;
}


