#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginProtocols.h"

#include "../main/Helper.h"
#include "DelayedLink.h"

#include "../main/Logger.h"
#include "../webserver/Base64.h"

#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

namespace Plugins {

	extern 	std::queue<CPluginMessageBase*>	PluginMessageQueue;
	extern	boost::mutex PluginMutex;

	void CPluginProtocol::ProcessInbound(const ReadMessage* Message)
	{
		// Raw protocol is to just always dispatch data to plugin without interpretation
		ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, Message->m_Buffer);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(RecvMessage);
		}
	}

	std::vector<byte> CPluginProtocol::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;

		// Handle Bytes objects
		if ((((PyObject*)WriteMessage->m_Object)->ob_type->tp_flags & (Py_TPFLAGS_BYTES_SUBCLASS)) != 0)
		{
			const char*	pData = PyBytes_AsString(WriteMessage->m_Object);
			int			iSize = PyBytes_Size(WriteMessage->m_Object);
			retVal.reserve((size_t)iSize);
			retVal.assign(pData, pData + iSize);
		}
		// Handle ByteArray objects
		else if ((((PyObject*)WriteMessage->m_Object)->ob_type->tp_name == std::string("bytearray")))
		{
			size_t	len = PyByteArray_Size(WriteMessage->m_Object);
			char*	data = PyByteArray_AsString(WriteMessage->m_Object);
			retVal.reserve(len);
			retVal.assign((const byte*)data, (const byte*)data + len);
		}
		// Handle String objects
		else if ((((PyObject*)WriteMessage->m_Object)->ob_type->tp_flags & (Py_TPFLAGS_UNICODE_SUBCLASS)) != 0)
		{
			std::string	sData = PyUnicode_AsUTF8(WriteMessage->m_Object);
			retVal.reserve((size_t)sData.length());
			retVal.assign((const byte*)sData.c_str(), (const byte*)sData.c_str() + sData.length());
		}
		else
			_log.Log(LOG_ERROR, "(%s) Send request Python object parameter was not of type Unicode or Byte, ignored.", __func__);

		return retVal;
	}

	void CPluginProtocol::Flush(CPlugin* pPlugin, PyObject* pConnection)
	{
		if (m_sRetainedData.size())
		{
			// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
			ReceivedMessage*	RecvMessage = new ReceivedMessage(pPlugin, pConnection, m_sRetainedData);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(RecvMessage);
			}
			m_sRetainedData.clear();
		}
	}

	void CPluginProtocolLine::ProcessInbound(const ReadMessage* Message)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::vector<byte>	vData = m_sRetainedData;										// if there was some data left over from last time add it back in
		vData.insert(vData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());		// add the new data

		std::string		sData(vData.begin(), vData.end());
		int iPos = sData.find_first_of('\r');		//  Look for message terminator 
		while (iPos != std::string::npos)
		{
			ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, std::vector<byte>(&sData[0], &sData[iPos]));
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(RecvMessage);
			}

			if (sData[iPos + 1] == '\n') iPos++;				//  Handle \r\n 
			sData = sData.substr(iPos + 1);
			iPos = sData.find_first_of('\r');
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolJSON::ProcessInbound(const ReadMessage* Message)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::vector<byte>	vData = m_sRetainedData;										// if there was some data left over from last time add it back in
		vData.insert(vData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());		// add the new data

		std::string		sData(vData.begin(), vData.end());
		int iPos = 1;
		while (iPos) {
			iPos = sData.find("}{", 0) + 1;		//  Look for message separater in case there is more than one
			if (!iPos) // no, just one or part of one
			{
				if ((sData.substr(sData.length() - 1, 1) == "}") &&
					(std::count(sData.begin(), sData.end(), '{') == std::count(sData.begin(), sData.end(), '}'))) // whole message so queue the whole buffer
				{
					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, sData);
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(RecvMessage);
					}
					sData.clear();
				}
			}
			else  // more than one message so queue the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, sMessage);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(RecvMessage);
				}
			}
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolXML::ProcessInbound(const ReadMessage* Message)
	{
		//
		//	Only returns whole XML messages. Does not handle <tag /> as the top level tag
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::vector<byte>	vData = m_sRetainedData;										// if there was some data left over from last time add it back in
		vData.insert(vData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());		// add the new data

		std::string		sData(vData.begin(), vData.end());
		try
		{
			while (true)
			{
				//
				//	Find the top level tag name if it is not set
				//
				if (!m_Tag.length())
				{
					if (sData.find("<?xml") != std::string::npos)	// step over '<?xml version="1.0" encoding="utf-8"?>' if present
					{
						int iEnd = sData.find("?>");
						sData = sData.substr(iEnd + 2);
					}

					int iStart = sData.find_first_of('<');
					if (iStart == std::string::npos)
					{
						// start of a tag not found so discard
						m_sRetainedData.clear();
						break;
					}
					if (iStart) sData = sData.substr(iStart);		// remove any leading data
					int iEnd = sData.find_first_of(" >");
					if (iEnd != std::string::npos)
					{
						m_Tag = sData.substr(1, (iEnd - 1));
					}
				}

				int	iPos = sData.find("</" + m_Tag + ">");
				if (iPos != std::string::npos)
				{
					int iEnd = iPos + m_Tag.length() + 3;
					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, sData.substr(0, iEnd));
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(RecvMessage);
					}

					if (iEnd == sData.length())
					{
						sData.clear();
					}
					else
					{
						sData = sData.substr(++iEnd);
					}
					m_Tag = "";
				}
				else
					break;
			}
		}
		catch (std::exception const &exc)
		{
			_log.Log(LOG_ERROR, "(CPluginProtocolXML::ProcessInbound) Unexpected exception thrown '%s', Data '%s'.", exc.what(), sData.c_str());
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolHTTP::ExtractHeaders(std::string * pData)
	{
		// Remove headers
		if (m_Headers)
		{
			Py_DECREF(m_Headers);
		}
		m_Headers = (PyObject*)PyDict_New();

		*pData = pData->substr(pData->find_first_of('\n') + 1);
		while (pData->length() && ((*pData)[0] != '\r'))
		{
			std::string		sHeaderLine = pData->substr(0, pData->find_first_of('\r'));
			std::string		sHeaderName = pData->substr(0, sHeaderLine.find_first_of(':'));
			std::string		uHeaderName = sHeaderName;
			stdupper(uHeaderName);
			std::string		sHeaderText = sHeaderLine.substr(sHeaderName.length() + 2);
			if (uHeaderName == "CONTENT-LENGTH")
			{
				m_ContentLength = atoi(sHeaderText.c_str());
			}
			if (uHeaderName == "TRANSFER-ENCODING")
			{
				std::string		uHeaderText = sHeaderText;
				stdupper(uHeaderText);
				if (uHeaderText == "CHUNKED")
					m_Chunked = true;
			}
			PyObject*	pObj = Py_BuildValue("s", sHeaderText.c_str());
			if (PyDict_SetItemString((PyObject*)m_Headers, sHeaderName.c_str(), pObj) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
			}
			Py_DECREF(pObj);
			*pData = pData->substr(pData->find_first_of('\n') + 1);
		}
	}

#define ADD_UTF8_TO_DICT(pDict, key, value) \
		{	\
			PyObject*	pObj = Py_BuildValue("y#", value.c_str(), value.length());	\
			if (PyDict_SetItem(pDict, key, pObj) == -1)	\
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", m_PluginKey.c_str(), key, value.c_str());	\
			Py_DECREF(pObj); \
		}

	void CPluginProtocolHTTP::ProcessInbound(const ReadMessage* Message)
	{
		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		// HTML is non binary so use strings
		std::string		sData(m_sRetainedData.begin(), m_sRetainedData.end());

		m_ContentLength = 0;
		m_Chunked = false;
		m_RemainingChunk = 0;

		//
		//	Process server responses
		//
		if (sData.substr(0, 4) == "HTTP")
		{
			// HTTP/1.0 404 Not Found
			// Content-Type: text/html; charset=UTF-8
			// Content-Length: 1570
			// Date: Thu, 05 Jan 2017 05:50:33 GMT
			//
			// <!DOCTYPE html>
			// <html lang=en>
			//   <meta charset=utf-8>
			//   <meta name=viewport...

			// HTTP/1.1 200 OK
			// Content-Type: text/html; charset=UTF-8
			// Transfer-Encoding: chunked
			// Date: Thu, 05 Jan 2017 05:50:33 GMT
			//
			// 40d
			// <!DOCTYPE html>
			// <html lang=en>
			//   <meta charset=utf-8>
			// ...
			// </html>
			// 0

			// Process response header (HTTP/1.1 200 OK)
			std::string		sFirstLine = sData.substr(0, sData.find_first_of('\r'));
			sFirstLine = sFirstLine.substr(sFirstLine.find_first_of(' ') + 1);
			m_Status = sFirstLine.substr(0, sFirstLine.find_first_of(' '));

			ExtractHeaders(&sData);

			// not enough data arrived to complete header processing
			if (!sData.length())
			{
				return;
			}

			sData = sData.substr(sData.find_first_of('\n') + 1);		// skip over 2nd new line char

			// Process the message body
			if (m_Status.length())
			{
				if (!m_Chunked)
				{
					// If full message then return it
					if (m_ContentLength == sData.length())
					{
						PyObject*	pDataDict = PyDict_New();
						PyObject*	pObj = Py_BuildValue("s", m_Status.c_str());
						if (PyDict_SetItemString(pDataDict, "Status", pObj) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Status", m_Status.c_str());
						Py_DECREF(pObj);

						if (m_Headers)
						{
							if (PyDict_SetItemString(pDataDict, "Headers", (PyObject*)m_Headers) == -1)
								_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
							Py_DECREF((PyObject*)m_Headers);
							m_Headers = NULL;
						}

						if (sData.length())
						{
							pObj = Py_BuildValue("y#", sData.c_str(), sData.length());
							if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
								_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sData.c_str());
							Py_DECREF(pObj);
						}

						ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, pDataDict);
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(RecvMessage);
						m_sRetainedData.clear();
					}
				}
				else
				{
					// Process available chunks
					std::string		sPayload;
					while (sData.length() && (sData != "\r\n"))
					{
						if (!m_RemainingChunk)	// Not processing a chunk so we should be at the start of one
						{
							if (sData[0] == '\r')
							{
								sData = sData.substr(sData.find_first_of('\n') + 1);
							}
							std::string		sChunkLine = sData.substr(0, sData.find_first_of('\r'));
							m_RemainingChunk = strtol(sChunkLine.c_str(), NULL, 16);
							sData = sData.substr(sData.find_first_of('\n') + 1);
							if (!m_RemainingChunk)	// last chunk is zero length
							{
								PyObject*	pDataDict = PyDict_New();
								PyObject*	pObj = Py_BuildValue("s", m_Status.c_str());
								if (PyDict_SetItemString(pDataDict, "Status", pObj) == -1)
									_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Status", m_Status.c_str());
								Py_DECREF(pObj);

								if (m_Headers)
								{
									if (PyDict_SetItemString(pDataDict, "Headers", (PyObject*)m_Headers) == -1)
										_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
									Py_DECREF((PyObject*)m_Headers);
									m_Headers = NULL;
								}

								if (sPayload.length())
								{
									pObj = Py_BuildValue("y#", sPayload.c_str(), sPayload.length());
									if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
										_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
									Py_DECREF(pObj);
								}

								ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, pDataDict);
								boost::lock_guard<boost::mutex> l(PluginMutex);
								PluginMessageQueue.push(RecvMessage);
								m_sRetainedData.clear();
								break;
							}
						}

						if (sData.length() <= m_RemainingChunk)		// Read data is just part of a chunk
						{
							break;
						}

						sPayload += sData.substr(0, m_RemainingChunk);
						sData = sData.substr(m_RemainingChunk);
						m_RemainingChunk = 0;
					}
				}
			}
		}

		//
		//	Process client requests
		//
		else
		{
			// GET / HTTP / 1.1\r\n
			// Host: 127.0.0.1 : 9090\r\n
			// User - Agent: Mozilla / 5.0 (Windows NT 10.0; WOW64; rv:53.0) Gecko / 20100101 Firefox / 53.0\r\n
			// Accept: text / html, application / xhtml + xml, application / xml; q = 0.9, */*;q=0.8\r\n
			std::string		sFirstLine = sData.substr(0, sData.find_first_of('\r'));
			sFirstLine = sFirstLine.substr(0, sFirstLine.find_last_of(' '));

			ExtractHeaders(&sData);
			if (sData.substr(0,2) == "\r\n")
			{
				std::string		sPayload = sData.substr(2);
				if (!m_ContentLength || (m_ContentLength == sPayload.length()))
				{
					PyObject* DataDict = PyDict_New();
					std::string		sVerb = sFirstLine.substr(0, sFirstLine.find_first_of(' '));
					PyObject*	pObj = Py_BuildValue("s", sVerb.c_str());
					if (PyDict_SetItemString(DataDict, "Verb", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Verb", sVerb.c_str());
					Py_DECREF(pObj);

					std::string		sURL = sFirstLine.substr(sVerb.length() + 1, sFirstLine.find_first_of(' ', sVerb.length() + 1));
					pObj = Py_BuildValue("s", sURL.c_str());
					if (PyDict_SetItemString(DataDict, "URL", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "URL", sURL.c_str());
					Py_DECREF(pObj);

					if (m_Headers)
					{
						if (PyDict_SetItemString(DataDict, "Headers", (PyObject*)m_Headers) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
						Py_DECREF((PyObject*)m_Headers);
						m_Headers = NULL;
					}

					if (sPayload.length())
					{
						pObj = Py_BuildValue("y#", sPayload.c_str(), sPayload.length());
						if (PyDict_SetItemString(DataDict, "Data", pObj) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
						Py_DECREF(pObj);
					}

					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_pPlugin, Message->m_pConnection, DataDict);
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(RecvMessage);
					m_sRetainedData.clear();
				}
			}
		}
	}

	std::vector<byte>	CPluginProtocolHTTP::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;
		std::string	sHttp;

		// Sanity check input
		if (!WriteMessage->m_Object || !PyDict_Check(WriteMessage->m_Object))
		{
			_log.Log(LOG_ERROR, "(%s) HTTP Send parameter was not a dictionary, ignored. See Python Plugin wiki page for help.", __func__);
			return retVal;
		}

		// Extract potential values.  Failures return NULL, success returns borrowed reference
		PyObject *pStatus = PyDict_GetItemString(WriteMessage->m_Object, "Status");
		PyObject *pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
		PyObject *pURL = PyDict_GetItemString(WriteMessage->m_Object, "URL");
		PyObject *pData = PyDict_GetItemString(WriteMessage->m_Object, "Data");
		PyObject *pHeaders = PyDict_GetItemString(WriteMessage->m_Object, "Headers");

		//
		//	Assume Request if 'Verb' specified
		//
		if (pVerb)
		{
			// GET /path/file.html HTTP/1.1
			// Connection: "keep-alive"
			// Accept: "text/html"
			//

			// POST /path/test.cgi HTTP/1.1
			// From: info@domoticz.com
			// User-Agent: Domoticz/1.0
			// Content-Type : application/x-www-form-urlencoded
			// Content-Length : 32
			//
			// param1=value&param2=other+value

			if (!PyUnicode_Check(pVerb))
			{
				_log.Log(LOG_ERROR, "(%s) HTTP 'Verb' dictionary entry not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}
			sHttp = PyUnicode_AsUTF8(pVerb);
			sHttp += " ";

			std::string	sHttpURL = "/";
			if (pURL && PyUnicode_Check(pURL))
			{
				sHttpURL = PyUnicode_AsUTF8(pURL);
			}
			sHttp += sHttpURL;
			sHttp += " HTTP/1.1\r\n";

			// If username &/or password specified then add a basic auth header
			std::string auth;
			if (m_Username.length() > 0 || m_Password.length() > 0)
			{
				if (m_Username.length() > 0)
				{
					auth += m_Username;
				}
				auth += ":";
				if (m_Password.length() > 0)
				{
					auth += m_Password;
				}
				std::string encodedAuth = base64_encode((const unsigned char *)auth.c_str(), auth.length());
				sHttp += "Authorization:Basic " + encodedAuth + "\r\n";
			}

			// Add Server header if it is not supplied
			PyObject *pHead = NULL;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "User-Agent");
			if (!pHead)
			{
				sHttp += "User-Agent: Domoticz/1.0\r\n";
			}
		}
		//
		//	Assume Response if 'Status' specified
		//
		else if (pStatus)
		{
			//	HTTP/1.1 200 OK
			//	Date: Mon, 27 Jul 2009 12:28:53 GMT
			//	Server: Apache/2.2.14 (Win32)
			//	Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
			//	Content-Length: 88
			//	Content-Type: text/html
			//	Connection: Closed
			//
			//	<html>
			//	<body>
			//	<h1>Hello, World!</h1>
			//	</body>
			//	</html>

			if (!PyUnicode_Check(pStatus))
			{
				_log.Log(LOG_ERROR, "(%s) HTTP 'Status' dictionary entry was not found or not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}

			sHttp = "HTTP/1.1 ";
			sHttp += PyUnicode_AsUTF8(pStatus);
			sHttp += "\r\n";

			// Add Date header if it is not supplied
			PyObject *pHead = NULL;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Date");
			if (!pHead)
			{
				char szDate[100];
				time_t rawtime;
				struct tm *info;
				time(&rawtime);
				info = gmtime(&rawtime);
				if (0 < strftime(szDate, sizeof(szDate), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", info))	sHttp += szDate;
			}

			// Add Server header if it is not supplied
			pHead = NULL;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Server");
			if (!pHead)
			{
				sHttp += "Server: Domoticz/1.0\r\n";
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) HTTP unable to determine send type. 'Status' or 'Verb' dictionary entries not found, ignored. See Python Plugin wiki page for help.", __func__);
			return retVal;
		}

		// Did we get headers to send?
		if (pHeaders)
		{
			if (PyDict_Check(pHeaders))
			{
				PyObject *key, *value;
				Py_ssize_t pos = 0;
				while (PyDict_Next(pHeaders, &pos, &key, &value))
				{
					std::string	sKey = PyUnicode_AsUTF8(key);
					std::string	sValue = PyUnicode_AsUTF8(value);
					sHttp += sKey + ": " + sValue + "\r\n";
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) HTTP Response 'Headers' parameter was not a dictionary, ignored.", __func__);
			}
		}

		// Add Content-Length header if it is required but not supplied
		PyObject *pLength = NULL;
		if (pHeaders)
			pLength = PyDict_GetItemString(pHeaders, "Content-Length");
		if (!pLength && pData && PyUnicode_Check(pData))
		{
			Py_ssize_t iLength = PyUnicode_GetLength(pData);
			sHttp += "Content-Length: " + SSTR(iLength) + "\r\n";
		}

		sHttp += "\r\n";

		// Append data if supplied (for POST)
		if (pData && PyUnicode_Check(pData))
		{
			sHttp += PyUnicode_AsUTF8(pData);
		}

		retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());

		return retVal;
	}
}
#endif