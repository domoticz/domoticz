//-----------------------------------------------------------------------------
//
//	HttpClient.h
//
//	Cross-platform HttpClient
//
//  Originally based upon minihttp client
//	Copyright (c) 2016 Justin Hammond <Justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

// Original License Text:
/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar.
 * See http://sam.zoy.org/wtfpl/COPYING for more details. */

#ifndef MINIHTTPSOCKET_H
#define MINIHTTPSOCKET_H

// ---- Compile config -----
#define MINIHTTP_SUPPORT_HTTP
#define MINIHTTP_SUPPORT_SOCKET_SET
// -------------------------

// Intentionally avoid pulling in any other headers

#include <string>
#include <map>
#include <queue>

namespace OpenZWave
{
	/** \defgroup SimpleHttpClient Basic HTTP(s) Client
	 *
	 * a Basic HTTP Client for talking to webservers
	 */

	namespace Internal
	{
		namespace Platform
		{
			/** \brief Initialize the Network for HTTP requests
			 * \ingroup SimpleHttpClient
			 *
			 * Initializes the Network for HTTP requests
			 *
			 * \returns success/failure
			 */
			bool InitNetwork();

			/** \brief Stop the Network for HTTP requests
			 * \ingroup SimpleHttpClient
			 *
			 * Stops the Network for HTTP requests and releases resources associated with it
			 */
			void StopNetwork();

			/** \brief Indicates if we support HTTPS requests
			 * \ingroup SimpleHttpClient
			 *
			 * Indicates if we support HTTPS requests
			 *
			 * \returns success/failure
			 * \todo SSL is not actually implemented yet.
			 */
			bool HasSSL();

			/** \brief Split a URL into its different parts/ports etc
			 * \ingroup SimpleHttpClient
			 *
			 * Split a URL Into the different parts/ports
			 *
			 * \param uri the URL to parse
			 * \param host the Hostname of the URL
			 * \param file the directory/file name of the URL
			 * \param port the port number of the URL, or 80 if not specified
			 *
			 * \returns success/failure
			 */
			bool SplitURI(const std::string& uri, std::string& host, std::string& file, int& port);

			/** \brief Encode a String suitable for sending as a URL request (eg Get)
			 * \ingroup SimpleHttpClient
			 *
			 * Encode a String so it can be sent as part of a URL request
			 *
			 * \param s the string to encode
			 * \param enc the encoded version of the string that is returned
			 *
			 */
			void URLEncode(const std::string& s, std::string& enc);

			/** \brief Result Codes for SSL operations
			 * \ingroup SimpleHttpClient
			 *
			 */
			enum SSLResult
			{
				SSLR_OK = 0x0, /**< No Error */
				SSLR_NO_SSL = 0x1, /**< SSL Is not required */
				SSLR_FAIL = 0x2, /**< Internal SSL Engine Failure */
				SSLR_CERT_EXPIRED = 0x4, /**< SSL Certificate has expired */
				SSLR_CERT_REVOKED = 0x8, /**< SSL Certificate is revoked */
				SSLR_CERT_CN_MISMATCH = 0x10, /**< SSL CN Name does not match Hostname */
				SSLR_CERT_NOT_TRUSTED = 0x20, /**< SSL Certificate is not trusted */
				SSLR_CERT_MISSING = 0x40, /**< SSL Certificate is missing */
				SSLR_CERT_SKIP_VERIFY = 0x80, /**< SSL Certificate Verification is disabled */
				SSLR_CERT_FUTURE = 0x100, /**< SSL Certificate is valid in the future */

				_SSLR_FORCE32BIT = 0x7fffffff
			};

			/** \brief a TCP Socket that can optionally be protected via SSL
			 * \ingroup SimpleHttpClient
			 *
			 * This represents a TCP Socket that can be encrypted via SSL and
			 * is used to connect to a TCP Server (in this case, a HTTP(s) Server
			 */
			class TcpSocket
			{
				public:
					TcpSocket();
					virtual ~TcpSocket();

					virtual bool HasPendingTask() const
					{
						return false;
					}

					bool open(const char *addr = NULL, unsigned int port = 0);
					void close();
					bool update(); // returns true if something interesting happened (incoming data, closed connection, etc)

					bool isOpen(void);

					void SetBufsizeIn(unsigned int s);
					bool SetNonBlocking(bool nonblock);
					unsigned int GetBufSize()
					{
						return _inbufSize;
					}
					const char *GetHost(void)
					{
						return _host.c_str();
					}
					bool SendBytes(const void *buf, unsigned int len);

					// SSL related
					bool initSSL(const char *certs);
					bool hasSSL() const
					{
						return !!_sslctx;
					}
					void shutdownSSL();
					SSLResult verifySSL();

				protected:
					virtual void _OnCloseInternal();
					virtual void _OnData(); // data received callback. Internal, should only be overloaded to call _OnRecv()

					virtual void _OnRecv(void *buf, unsigned int size) = 0;
					virtual void _OnClose()
					{
					}
					; // close callback
					virtual void _OnOpen()
					{
					} // called when opened
					virtual bool _OnUpdate()
					{
						return true;
					} // called before reading from the socket

					void _ShiftBuffer();

					char *_inbuf;
					char *_readptr; // part of inbuf, optionally skipped header
					char *_writeptr; // passed to recv(). usually equal to _inbuf, but may point inside the buffer in case of a partial transfer.

					unsigned int _inbufSize; // size of internal buffer
					unsigned int _writeSize; // how many bytes can be written to _writeptr;
					unsigned int _recvSize; // incoming size, max _inbufSize - 1

					unsigned int _lastport; // port used in last open() call

					bool _nonblocking; // Default true. If false, the current thread is blocked while waiting for input.

#ifdef _WIN32
					intptr_t _s; // socket handle. really an int, but to be sure its 64 bit compatible as it seems required on windows, we use this.
#else
					long _s;
#endif

					std::string _host;

				private:
					int _writeBytes(const unsigned char *buf, size_t len);
					int _readBytes(unsigned char *buf, size_t maxlen);
					void *_sslctx;
			};

#ifdef MINIHTTP_SUPPORT_HTTP

			enum HttpCode
			{
				HTTP_OK = 200,
				HTTP_NOTFOUND = 404,
			};

			/** \brief This class is used for Posting data to a HTTP(s) server
			 * \ingroup SimpleHttpClient
			 *
			 * Post some data to a HTTP(s) server
			 *
			 */
			class POST
			{
				public:
					void reserve(size_t res)
					{
						data.reserve(res);
					}
					POST& add(const char *key, const char *value);
					const char *c_str() const
					{
						return data.c_str();
					}
					const std::string& str() const
					{
						return data;
					}
					bool empty() const
					{
						return data.empty();
					}
					size_t length() const
					{
						return data.length();
					}
				private:
					std::string data;
			};

			/** \brief Main class for making a HTTP request to a HTTP(s) server
			 * \ingroup SimpleHttpClient
			 *
			 * Make a request to a HTTP Server
			 *
			 */
			struct Request
			{
					Request() :
							port(80), user(NULL)
					{
					}
					Request(const std::string& h, const std::string& res, int p = 80, void *u = NULL) :
							host(h), resource(res), port(80), user(u), useSSL(false)
					{
					}

					std::string protocol;
					std::string host;
					std::string header; // set by socket
					std::string resource;
					std::string extraGetHeaders;
					int port;
					void *user;
					bool useSSL;
					POST post; // if this is empty, it's a GET request, otherwise a POST request
			};

			/** \brief a Socket that speaks HTTP protocol.
			 * \ingroup SimpleHttpClient
			 *
			 * Talk to a HTTP(s) server
			 *
			 */
			class HttpSocket: public OpenZWave::Internal::Platform::TcpSocket
			{
				public:

					HttpSocket();
					virtual ~HttpSocket();

					virtual bool HasPendingTask() const
					{
						return ExpectMoreData() || _requestQ.size();
					}

					void SetKeepAlive(unsigned int secs)
					{
						_keep_alive = secs;
					}
					void SetUserAgent(const std::string &s)
					{
						_user_agent = s;
					}
					void SetAcceptEncoding(const std::string& s)
					{
						_accept_encoding = s;
					}
					void SetFollowRedirect(bool follow)
					{
						_followRedir = follow;
					}
					void SetAlwaysHandle(bool h)
					{
						_alwaysHandle = h;
					}
					void SetDownloadFile(std::string filename)
					{
						_filename = filename;
					}

					bool Download(const std::string& url, const char *extraRequest = NULL, void *user = NULL, const POST *post = NULL);
					bool SendRequest(Request& what, bool enqueue);
					bool SendRequest(const std::string what, const char *extraRequest = NULL, void *user = NULL);
					bool QueueRequest(const std::string what, const char *extraRequest = NULL, void *user = NULL);

					unsigned int GetRemaining() const
					{
						return _remaining;
					}

					unsigned int GetStatusCode() const
					{
						return _status;
					}
					unsigned int GetContentLen() const
					{
						return _contentLen;
					}
					bool ChunkedTransfer() const
					{
						return _chunkedTransfer;
					}
					bool ExpectMoreData() const
					{
						return _remaining || _chunkedTransfer;
					}

					const Request &GetCurrentRequest() const
					{
						return _curRequest;
					}
					const char *Hdr(const char *h) const;

					bool IsRedirecting() const;
					bool IsSuccess() const;

				protected:
					virtual void _OnCloseInternal();
					virtual void _OnClose();
					virtual void _OnData(); // data received callback. Internal, should only be overloaded to call _OnRecv()
					virtual void _OnRecv(void *buf, unsigned int size);
					virtual void _OnOpen(); // called when opene
					virtual bool _OnUpdate(); // called before reading from the socket

					// new ones:
					virtual void _OnRequestDone()
					{
					}

					bool _Redirect(std::string loc, bool forceGET);

					void _ProcessChunk();
					bool _EnqueueOrSend(const Request& req, bool forceQueue = false);
					void _DequeueMore();
					bool _OpenRequest(const Request& req);
					void _ParseHeader();
					void _ParseHeaderFields(const char *s, size_t size);
					bool _HandleStatus(); // Returns whether the processed request was successful, or not
					void _FinishRequest();
					void _OnRecvInternal(void *buf, unsigned int size);

					std::string _user_agent;
					std::string _accept_encoding; // Default empty.
					std::string _tmpHdr; // used to save the http header if the incoming buffer was not large enough

					unsigned int _keep_alive; // http related
					unsigned int _remaining; // http "Content-Length: X" - already recvd. 0 if ready for next packet.
											 // For chunked transfer encoding, this holds the remaining size of the current chunk
					unsigned int _contentLen; // as reported by server
					unsigned int _status; // http status code, HTTP_OK if things are good

					std::queue<Request> _requestQ;
					std::map<std::string, std::string> _hdrs; // Maps HTTP header fields to their values

					Request _curRequest;

					bool _inProgress;
					bool _chunkedTransfer;
					bool _mustClose; // keep-alive specified, or not
					bool _followRedir; // Default true. Follow 3xx redirects if this is set.
					bool _alwaysHandle; // Also deliver to _OnRecv() if a non-success code was received.
					std::string _filename;
					FILE* _pFile;
			};

#endif

// ------------------------------------------------------------------------

#ifdef MINIHTTP_SUPPORT_SOCKET_SET

			/** \brief Support Multiple TCP Socket connections
			 * \ingroup SimpleHttpClient
			 *
			 * to Support multiple TCP Socket Connections
			 *
			 */
			class SocketSet
			{
				public:
					virtual ~SocketSet();
					void deleteAll();
					bool update();
					void add(TcpSocket *s, bool deleteWhenDone = true);
					bool has(TcpSocket *s);
					void remove(TcpSocket *s);
					inline size_t size()
					{
						return _store.size();
					}

//protected:

					struct SocketSetData
					{
							bool deleteWhenDone;
							// To be extended
					};

					typedef std::map<TcpSocket*, SocketSetData> Store;

					Store _store;
			};

#endif
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave

#endif
