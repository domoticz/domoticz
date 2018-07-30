#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../webserver/Base64.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace Plugins {

	CPluginProtocol * CPluginProtocol::Create(std::string sProtocol, std::string sUsername, std::string sPassword)
	{
		if (sProtocol == "Line") return (CPluginProtocol*) new CPluginProtocolLine();
		else if (sProtocol == "XML") return (CPluginProtocol*) new CPluginProtocolXML();
		else if (sProtocol == "JSON") return (CPluginProtocol*) new CPluginProtocolJSON();
		else if ((sProtocol == "HTTP") || (sProtocol == "HTTPS"))
		{
			CPluginProtocolHTTP*	pProtocol = new CPluginProtocolHTTP(sProtocol == "HTTPS");
			pProtocol->AuthenticationDetails(sUsername, sPassword);
			return (CPluginProtocol*)pProtocol;
		}
		else if (sProtocol == "ICMP") return (CPluginProtocol*) new CPluginProtocolICMP();
		else if ((sProtocol == "MQTT") || (sProtocol == "MQTTS"))
		{
			CPluginProtocolMQTT*	pProtocol = new CPluginProtocolMQTT(sProtocol == "MQTTS");
			pProtocol->AuthenticationDetails(sUsername, sPassword);
			return (CPluginProtocol*)pProtocol;
		}
		else return new CPluginProtocol();
	}

	void CPluginProtocol::ProcessInbound(const ReadEvent* Message)
	{
		// Raw protocol is to just always dispatch data to plugin without interpretation
		Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, Message->m_Buffer));
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
			pPlugin->MessagePlugin(new onMessageCallback(pPlugin, pConnection, m_sRetainedData));
			m_sRetainedData.clear();
		}
	}

	void CPluginProtocolLine::ProcessInbound(const ReadEvent* Message)
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
			Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, std::vector<byte>(&sData[0], &sData[iPos])));

			if (sData[iPos + 1] == '\n') iPos++;				//  Handle \r\n
			sData = sData.substr(iPos + 1);
			iPos = sData.find_first_of('\r');
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolJSON::ProcessInbound(const ReadEvent* Message)
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
					Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, sData));
					sData.clear();
				}
			}
			else  // more than one message so queue the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, sMessage));
			}
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolXML::ProcessInbound(const ReadEvent* Message)
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
					Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, sData.substr(0, iEnd)));

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
			PyObject* pObj = Py_BuildValue("s", sHeaderText.c_str());
			PyObject* pPrevObj = PyDict_GetItemString((PyObject*)m_Headers, sHeaderName.c_str());
			// If the header is not unique, we concatenate with '\n'. RFC2616 recommends comma, but it doesn't work for cookies for instance
			if (pPrevObj != NULL) {
				std::string sCombin = PyUnicode_AsUTF8(pPrevObj);
				sCombin += '\n' + sHeaderText;
				PyObject*   pObjCombin = Py_BuildValue("s", sCombin.c_str());
				if (PyDict_SetItemString((PyObject*)m_Headers, sHeaderName.c_str(), pObjCombin) == -1) {
					_log.Log(LOG_ERROR, "(%s) failed to append key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
				}
				Py_DECREF(pObjCombin);
			}
			else if (PyDict_SetItemString((PyObject*)m_Headers, sHeaderName.c_str(), pObj) == -1) {
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
			}
			Py_DECREF(pObj);
			*pData = pData->substr(pData->find_first_of('\n') + 1);
		}
	}

static void AddBytesToDict(PyObject* pDict, const char* key, const std::string &value)
{
	PyObject*	pObj = Py_BuildValue("y#", value.c_str(), value.length());
	if (PyDict_SetItemString(pDict, key, pObj) == -1)
		_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	Py_DECREF(pObj);
}

static void AddStringToDict(PyObject* pDict, const char* key, const std::string &value)
{
	PyObject*	pObj = Py_BuildValue("s#", value.c_str(), value.length());
	if (PyDict_SetItemString(pDict, key, pObj) == -1)
		_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	Py_DECREF(pObj);
}

static void AddIntToDict(PyObject* pDict, const char* key, const int value)
{
	PyObject*	pObj = Py_BuildValue("i", value);
	if (PyDict_SetItemString(pDict, key, pObj) == -1)
		_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	Py_DECREF(pObj);
}

	void CPluginProtocolHTTP::ProcessInbound(const ReadEvent* Message)
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

						Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pDataDict));
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

								Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pDataDict));
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

					Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, DataDict));
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
				std::string encodedAuth = base64_encode(auth);
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
			sHttp += "Content-Length: " + std::to_string(iLength) + "\r\n";
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

	void CPluginProtocolICMP::ProcessInbound(const ReadEvent * Message)
	{
		PyObject*	pObj = NULL;
		PyObject*	pDataDict = PyDict_New();
		int			iTotalData = 0;
		int			iDataOffset = 0;

		// Handle response
		if (Message->m_Buffer.size())
		{
			PyObject*	pIPv4Dict = PyDict_New();
			if (pDataDict && pIPv4Dict)
			{
				if (PyDict_SetItemString(pDataDict, "IPv4", (PyObject*)pIPv4Dict) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "ICMP", "IPv4");
				else
				{
					Py_XDECREF((PyObject*)pIPv4Dict);

					ipv4_header*	pIPv4 = (ipv4_header*)(&Message->m_Buffer[0]);

					pObj = Py_BuildValue("s", pIPv4->source_address().to_string().c_str());
					PyDict_SetItemString(pIPv4Dict, "Source", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("s", pIPv4->destination_address().to_string().c_str());
					PyDict_SetItemString(pIPv4Dict, "Destination", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("b", pIPv4->version());
					PyDict_SetItemString(pIPv4Dict, "Version", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("b", pIPv4->protocol());
					PyDict_SetItemString(pIPv4Dict, "Protocol", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("b", pIPv4->type_of_service());
					PyDict_SetItemString(pIPv4Dict, "TypeOfService", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pIPv4->header_length());
					PyDict_SetItemString(pIPv4Dict, "HeaderLength", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pIPv4->total_length());
					PyDict_SetItemString(pIPv4Dict, "TotalLength", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pIPv4->identification());
					PyDict_SetItemString(pIPv4Dict, "Identification", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pIPv4->header_checksum());
					PyDict_SetItemString(pIPv4Dict, "HeaderChecksum", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("i", pIPv4->time_to_live());
					PyDict_SetItemString(pIPv4Dict, "TimeToLive", pObj);
					Py_DECREF(pObj);

					iTotalData = pIPv4->total_length();
					iDataOffset = pIPv4->header_length();
				}
				pIPv4Dict = NULL;
			}

			PyObject*	pIcmpDict = PyDict_New();
			if (pDataDict && pIcmpDict)
			{
				if (PyDict_SetItemString(pDataDict, "ICMP", (PyObject*)pIcmpDict) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "ICMP", "ICMP");
				else
				{
					Py_XDECREF((PyObject*)pIcmpDict);

					icmp_header*	pICMP = (icmp_header*)(&Message->m_Buffer[0] + 20);
					if ((pICMP->type() == icmp_header::echo_reply) && (Message->m_ElapsedMs >= 0))
					{
						pObj = Py_BuildValue("I", Message->m_ElapsedMs);
						PyDict_SetItemString(pDataDict, "ElapsedMs", pObj);
						Py_DECREF(pObj);
					}

					pObj = Py_BuildValue("b", pICMP->type());
					PyDict_SetItemString(pIcmpDict, "Type", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("b", pICMP->type());
					PyDict_SetItemString(pDataDict, "Status", pObj);
					Py_DECREF(pObj);

					switch (pICMP->type())
					{
					case icmp_header::echo_reply:
						pObj = Py_BuildValue("s", "echo_reply");
						break;
					case icmp_header::destination_unreachable:
						pObj = Py_BuildValue("s", "destination_unreachable");
						break;
					case icmp_header::time_exceeded:
						pObj = Py_BuildValue("s", "time_exceeded");
						break;
					default:
						pObj = Py_BuildValue("s", "unknown");
					}

					PyDict_SetItemString(pDataDict, "Description", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("b", pICMP->code());
					PyDict_SetItemString(pIcmpDict, "Code", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pICMP->checksum());
					PyDict_SetItemString(pIcmpDict, "Checksum", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pICMP->identifier());
					PyDict_SetItemString(pIcmpDict, "Identifier", pObj);
					Py_DECREF(pObj);

					pObj = Py_BuildValue("h", pICMP->sequence_number());
					PyDict_SetItemString(pIcmpDict, "SequenceNumber", pObj);
					Py_DECREF(pObj);

					iDataOffset += sizeof(icmp_header);
					if (pICMP->type() == icmp_header::destination_unreachable)
					{
						ipv4_header*	pIPv4 = (ipv4_header*)(pICMP + 1);
						iDataOffset += pIPv4->header_length() + sizeof(icmp_header);
					}
				}
				pIcmpDict = NULL;
			}
		}
		else
		{
			pObj = Py_BuildValue("b", icmp_header::time_exceeded);
			PyDict_SetItemString(pDataDict, "Status", pObj);
			Py_DECREF(pObj);

			pObj = Py_BuildValue("s", "time_exceeded");
			PyDict_SetItemString(pDataDict, "Description", pObj);
			Py_DECREF(pObj);
		}

		std::string		sData(Message->m_Buffer.begin(), Message->m_Buffer.end());
		sData = sData.substr(iDataOffset, iTotalData - iDataOffset);
		pObj = Py_BuildValue("y#", sData.c_str(), sData.length());
		if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "ICMP", "Data", sData.c_str());
		Py_DECREF(pObj);

		if (pDataDict)
		{
			Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pDataDict));
		}
	}

#define MQTT_CONNECT       1<<4
#define MQTT_CONNACK       2<<4
#define MQTT_PUBLISH       3<<4
#define MQTT_PUBACK        4<<4
#define MQTT_PUBREC        5<<4
#define MQTT_PUBREL        6<<4
#define MQTT_PUBCOMP       7<<4
#define MQTT_SUBSCRIBE     8<<4
#define MQTT_SUBACK        9<<4
#define MQTT_UNSUBSCRIBE  10<<4
#define MQTT_UNSUBACK     11<<4
#define MQTT_PINGREQ      12<<4
#define MQTT_PINGRESP     13<<4
#define MQTT_DISCONNECT   14<<4

#define MQTT_PROTOCOL	  4

	static void MQTTPushBackNumber(int iNumber, std::vector<byte> &vVector)
	{
		vVector.push_back(iNumber / 256);
		vVector.push_back(iNumber % 256);
	}

	static void MQTTPushBackString(const std::string &sString, std::vector<byte> &vVector)
	{
		vVector.insert(vVector.end(), sString.begin(), sString.end());
	}

	static void MQTTPushBackStringWLen(const std::string &sString, std::vector<byte> &vVector)
	{
		MQTTPushBackNumber(sString.length(), vVector);
		vVector.insert(vVector.end(), sString.begin(), sString.end());
	}

	void CPluginProtocolMQTT::ProcessInbound(const ReadEvent * Message)
	{
		byte loop = 0;
		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		do {
			std::vector<byte>::iterator it = m_sRetainedData.begin();

			byte		header = *it++;
			byte		bResponseType = header & 0xF0;
			PyObject*	pMqttDict = PyDict_New();
			PyObject*	pObj = NULL;
			uint16_t	iPacketIdentifier = 0;
			long		iRemainingLength = 0;
			long		multiplier = 1;
			byte 		encodedByte;

			do
			{
				encodedByte = *it++;
				iRemainingLength += (encodedByte & 127) * multiplier;
				multiplier *= 128;
				if (multiplier > 128*128*128)
				{
					_log.Log(LOG_ERROR, "(%s) Malformed Remaining Length.", __func__);
					return;
				}
			} while ((encodedByte & 128) != 0);

			if (iRemainingLength > std::distance(it, m_sRetainedData.end()))
			{
				// Full packet has not arrived, wait for more data
				_log.Debug(DEBUG_NORM, "(%s) Not enough data received (got %ld, expected %ld).", __func__, (long)std::distance(it, m_sRetainedData.end()), iRemainingLength);
				return;
			}

			std::vector<byte>::iterator pktend = it+iRemainingLength;

			switch (bResponseType)
			{
			case MQTT_CONNACK:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("CONNACK"));
				if (iRemainingLength == 2) // check length is correct
				{
					switch (*it++)
					{
					case 0:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Accepted"));
						break;
					case 1:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Refused, unacceptable protocol version"));
						break;
					case 2:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Refused, identifier rejected"));
						break;
					case 3:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Refused, Server unavailable"));
						break;
					case 4:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Refused, bad user name or password"));
						break;
					case 5:
						AddStringToDict(pMqttDict, "Description", std::string("Connection Refused, not authorized"));
						break;
					default:
						AddStringToDict(pMqttDict, "Description", std::string("Unknown status returned"));
						break;
					}
					AddIntToDict(pMqttDict, "Status", *it++);
				}
				else
				{
					AddIntToDict(pMqttDict, "Status", -1);
					AddStringToDict(pMqttDict, "Description", std::string("Invalid message length"));
				}
				break;
			}
			case MQTT_SUBACK:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("SUBACK"));
				iPacketIdentifier = (*it++ << 8) + *it++;
				AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);

				if (iRemainingLength >= 3) // check length is acceptable
				{
					PyObject* pResponsesList = PyList_New(0);
					if (PyDict_SetItemString(pMqttDict, "Topics", pResponsesList) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", __func__, "Topics");
						break;
					}
					Py_DECREF(pResponsesList);

					while (it != pktend)
					{
						PyObject*	pResponseDict = PyDict_New();
						byte Status = *it++;
						AddIntToDict(pResponseDict, "Status", Status);
						switch (Status)
						{
						case 0x00:
							AddStringToDict(pResponseDict, "Description", std::string("Success - Maximum QoS 0"));
							break;
						case 0x01:
							AddStringToDict(pResponseDict, "Description", std::string("Success - Maximum QoS 1"));
							break;
						case 0x02:
							AddStringToDict(pResponseDict, "Description", std::string("Success - Maximum QoS 2"));
							break;
						case 0x80:
							AddStringToDict(pResponseDict, "Description", std::string("Failure"));
							break;
						default:
							AddStringToDict(pResponseDict, "Description", std::string("Unknown status returned"));
							break;
						}
						PyList_Append(pResponsesList, pResponseDict);
						Py_DECREF(pResponseDict);
					}
				}
				else
				{
					AddIntToDict(pMqttDict, "Status", -1);
					AddStringToDict(pMqttDict, "Description", std::string("Invalid message length"));
				}
				break;
			}
			case MQTT_PUBACK:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBACK"));
				if (iRemainingLength == 2)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				break;
			case MQTT_PUBREC:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBREC"));
				if (iRemainingLength == 2)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				break;
			case MQTT_PUBCOMP:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBCOMP"));
				if (iRemainingLength == 2)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				break;
			case MQTT_PUBLISH:
			{
				// Fixed Header
				AddStringToDict(pMqttDict, "Verb", std::string("PUBLISH"));
				AddIntToDict(pMqttDict, "DUP", ((header & 0x08) >> 3));
				long	iQoS = (header & 0x06) >> 1;
				AddIntToDict(pMqttDict, "QoS", (int) iQoS);
				PyDict_SetItemString(pMqttDict, "Retain", PyBool_FromLong(header & 0x01));
				// Variable Header
				int		topicLen = (*it++ << 8) + *it++;
				std::string	sTopic((char const*)&*it, topicLen);
				AddStringToDict(pMqttDict, "Topic", sTopic);
				it += topicLen;
				if (iQoS)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				// Payload
				const char*	pPayload = (it==pktend)?0:(const char*)&*it;
				std::string	sPayload(pPayload, std::distance(it, pktend));
				AddBytesToDict(pMqttDict, "Payload", sPayload);
				break;
			}
			case MQTT_UNSUBACK:
				AddStringToDict(pMqttDict, "Verb", std::string("UNSUBACK"));
				if (iRemainingLength == 2)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				break;
			case MQTT_PINGRESP:
				AddStringToDict(pMqttDict, "Verb", std::string("PINGRESP"));
				break;
			default:
				_log.Log(LOG_ERROR, "(%s) MQTT response invalid '%d' is unknown.", __func__, bResponseType);
				m_sRetainedData.erase(m_sRetainedData.begin(),pktend);
				continue;
			}

			Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pMqttDict));

			m_sRetainedData.erase(m_sRetainedData.begin(),pktend);
		} while (m_sRetainedData.size() > 0);
	}

	std::vector<byte> CPluginProtocolMQTT::ProcessOutbound(const WriteDirective * WriteMessage)
	{
		std::vector<byte>	vVariableHeader;
		std::vector<byte>	vPayload;

		std::vector<byte>	retVal;

		// Sanity check input
		if (!WriteMessage->m_Object || !PyDict_Check(WriteMessage->m_Object))
		{
			_log.Log(LOG_ERROR, "(%s) MQTT Send parameter was not a dictionary, ignored. See Python Plugin wiki page for help.", __func__);
			return retVal;
		}

		// Extract potential values.  Failures return NULL, success returns borrowed reference
		PyObject *pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
		if (pVerb)
		{
			if (!PyUnicode_Check(pVerb))
			{
				_log.Log(LOG_ERROR, "(%s) MQTT 'Verb' dictionary entry not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}
			std::string sVerb = PyUnicode_AsUTF8(pVerb);

			if (sVerb == "CONNECT")
			{
				MQTTPushBackStringWLen("MQTT", vVariableHeader);
				vVariableHeader.push_back(MQTT_PROTOCOL);

				byte	bControlFlags = 0;

				// Client Identifier
				PyObject *pID = PyDict_GetItemString(WriteMessage->m_Object, "ID");
				if (pID && PyUnicode_Check(pID))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pID)), vPayload);
				}
				else
					MQTTPushBackStringWLen("Domoticz", vPayload); // TODO: default ID should be more unique, for example "Domoticz_<plugin_name>_<HwID>"

				byte	bCleanSession = 1;
				PyObject*	pCleanSession = PyDict_GetItemString(WriteMessage->m_Object, "CleanSession");
				if (pCleanSession && PyLong_Check(pCleanSession))
				{
					bCleanSession = (byte)PyLong_AsLong(pCleanSession);
				}
				bControlFlags |= (bCleanSession&1)<<1;

				// Will topic
				PyObject*	pTopic = PyDict_GetItemString(WriteMessage->m_Object, "WillTopic");
				if (pTopic && PyUnicode_Check(pTopic))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
					bControlFlags |= 4;
				}

				// Will QoS, Retain and Message
				if (bControlFlags & 4)
				{
					PyObject *pQoS = PyDict_GetItemString(WriteMessage->m_Object, "WillQoS");
					if (pQoS && PyLong_Check(pQoS))
					{
						byte bQoS = (byte) PyLong_AsLong(pQoS);
						bControlFlags |= (bQoS&3)<<3; // Set QoS flag
					}

					PyObject *pRetain = PyDict_GetItemString(WriteMessage->m_Object, "WillRetain");
					if (pRetain && PyLong_Check(pRetain))
					{
						byte bRetain = (byte)PyLong_AsLong(pRetain);
						bControlFlags |= (bRetain&1)<<5; // Set retain flag
					}

					std::string sPayload = "";
					PyObject*	pPayload = PyDict_GetItemString(WriteMessage->m_Object, "WillPayload");
					// Support both string and bytes
					//if (pPayload && PyByteArray_Check(pPayload)) // Gives linker error, why?
					if (pPayload && pPayload->ob_type->tp_name == std::string("bytearray"))
					{
						sPayload = std::string(PyByteArray_AsString(pPayload), PyByteArray_Size(pPayload));
					}
					else if (pPayload && PyUnicode_Check(pPayload))
					{
						sPayload = std::string(PyUnicode_AsUTF8(pPayload));
					}
					MQTTPushBackStringWLen(sPayload, vPayload);
				}

				// Username / Password
				if (m_Username.length())
				{
					MQTTPushBackStringWLen(m_Username, vPayload);
					bControlFlags |= 128;
				}

				if (m_Password.length())
				{
					MQTTPushBackStringWLen(m_Password, vPayload);
					bControlFlags |= 64;
				}

				// Control Flags
				vVariableHeader.push_back(bControlFlags);

				// Keep Alive
				vVariableHeader.push_back(0);
				vVariableHeader.push_back(60);

				retVal.push_back(MQTT_CONNECT);
			}
			else if (sVerb == "PING")
			{
				retVal.push_back(MQTT_PINGREQ);
			}
			else if (sVerb == "SUBSCRIBE")
			{
				// Variable header - Packet Identifier.
				// If supplied then use it otherwise create one
				PyObject *pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID && PyLong_Check(pID))
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is list of topics and QoS numbers
				PyObject *pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList || !PyList_Check(pTopicList))
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyObject*	pTopicDict = PyList_GetItem(pTopicList, i);
					if (!pTopicDict || !PyDict_Check(pTopicDict))
					{
						_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: Topics list entry is not a dictionary (Topic, QoS), nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
						return retVal;
					}
					PyObject*	pTopic = PyDict_GetItemString(pTopicDict, "Topic");
					if (pTopic && PyUnicode_Check(pTopic))
					{
						MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
						PyObject*	pQoS = PyDict_GetItemString(pTopicDict, "QoS");
						if (pQoS && PyLong_Check(pQoS))
						{
							vPayload.push_back((byte)PyLong_AsLong(pQoS));
						}
						else vPayload.push_back(0);
					}
					else
					{
						_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: 'Topic' not in dictionary (Topic, QoS), nothing to subscribe to, skipping. See Python Plugin wiki page for help.", __func__);
					}
				}
				retVal.push_back(MQTT_SUBSCRIBE | 0x02);	// Add mandatory reserved flags
			}
			else if (sVerb == "UNSUBSCRIBE")
			{
				// Variable Header
				PyObject *pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID && PyLong_Check(pID))
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is a Python list of topics
				PyObject *pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList || !PyList_Check(pTopicList))
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to unsubscribe from. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyObject*	pTopic = PyList_GetItem(pTopicList, i);
					if (pTopic && PyUnicode_Check(pTopic))
					{
						MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
					}
				}

				retVal.push_back(MQTT_UNSUBSCRIBE | 0x02);
			}
			else if (sVerb == "PUBLISH")
			{
				byte	bByte0 = MQTT_PUBLISH;

				// Fixed Header
				PyObject *pDUP = PyDict_GetItemString(WriteMessage->m_Object, "Duplicate");
				if (pDUP && PyLong_Check(pDUP))
				{
					long	bDUP = PyLong_AsLong(pDUP);
					if (bDUP) bByte0 |= 0x08; // Set duplicate flag
				}

				PyObject *pQoS = PyDict_GetItemString(WriteMessage->m_Object, "QoS");
				long	iQoS = 0;
				if (pQoS && PyLong_Check(pQoS))
				{
					iQoS = PyLong_AsLong(pQoS);
					bByte0 |= ((iQoS & 3) << 1); // Set QoS flag
				}

				PyObject *pRetain = PyDict_GetItemString(WriteMessage->m_Object, "Retain");
				if (pRetain && PyLong_Check(pRetain))
				{
					long	bRetain = PyLong_AsLong(pRetain);
					bByte0 |= (bRetain & 1); // Set retain flag
				}

				// Variable Header
				PyObject*	pTopic = PyDict_GetItemString(WriteMessage->m_Object, "Topic");
				if (pTopic && PyUnicode_Check(pTopic))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vVariableHeader);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: No 'Topic' specified, nothing to publish. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				PyObject *pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				if (iQoS)
				{
					long	iPacketIdentifier = 0;
					if (pID && PyLong_Check(pID))
					{
						iPacketIdentifier = PyLong_AsLong(pID);
					}
					else iPacketIdentifier = m_PacketID++;
					MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);
				}
				else if (pID)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: PacketIdentifier ignored, QoS not specified. See Python Plugin wiki page for help.", __func__);
				}

				// Payload
				std::string sPayload = "";
				PyObject*	pPayload = PyDict_GetItemString(WriteMessage->m_Object, "Payload");
				// Support both string and bytes
				//if (pPayload && PyByteArray_Check(pPayload)) // Gives linker error, why?
				if (pPayload) {
					_log.Debug(DEBUG_NORM, "(%s) MQTT Publish: payload %p (%s)", __func__, pPayload, pPayload->ob_type->tp_name);
				}
				if (pPayload && pPayload->ob_type->tp_name == std::string("bytearray"))
				{
					sPayload = std::string(PyByteArray_AsString(pPayload), PyByteArray_Size(pPayload));
				}
				else if (pPayload && PyUnicode_Check(pPayload))
				{
					sPayload = std::string(PyUnicode_AsUTF8(pPayload));
				}
				MQTTPushBackString(sPayload, vPayload);

				retVal.push_back(bByte0);
			}
			else if (sVerb == "PUBREL")
			{
				// Variable Header
				PyObject *pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID && PyLong_Check(pID))
				{
					iPacketIdentifier = PyLong_AsLong(pID);
					MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: No valid PacketIdentifier specified. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				retVal.push_back(MQTT_PUBREL & 0x02);
			}
			else if (sVerb == "DISCONNECT")
			{
				retVal.push_back(MQTT_DISCONNECT);
				retVal.push_back(0);
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) MQTT 'Verb' invalid '%s' is unknown.", __func__, sVerb.c_str());
				return retVal;
			}
		}

		// Build final message
		unsigned long	iRemainingLength = vVariableHeader.size() + vPayload.size();
		do
		{
			byte	encodedByte = iRemainingLength % 128;
			iRemainingLength = iRemainingLength / 128;

			// if there are more data to encode, set the top bit of this byte
			if (iRemainingLength > 0)
				encodedByte |= 128;
			retVal.push_back(encodedByte);

		} while (iRemainingLength > 0);

		retVal.insert(retVal.end(), vVariableHeader.begin(), vVariableHeader.end());
		retVal.insert(retVal.end(), vPayload.begin(), vPayload.end());

		return retVal;
	}
}
#endif
