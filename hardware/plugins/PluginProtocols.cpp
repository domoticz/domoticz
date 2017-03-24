#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "PluginMessages.h"
#include "PluginProtocols.h"

#include "../main/Helper.h"
#include "DelayedLink.h"

#include "../main/Logger.h"
#include "../webserver/Base64.h"

#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace Plugins {

	extern 	std::queue<CPluginMessage*>	PluginMessageQueue;
	extern	boost::mutex PluginMutex;

	void CPluginProtocol::ProcessInbound(const ReadMessage* Message)
	{
		// Raw protocol is to just always dispatch data to plugin without interpretation
		ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, Message->m_Buffer);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(RecvMessage);
		}
	}

	std::vector<byte> CPluginProtocol::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		return WriteMessage->m_Buffer;
	}

	void CPluginProtocol::Flush(const int HwdID)
	{
		// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
		ReceivedMessage*	RecvMessage = new ReceivedMessage(HwdID, m_sRetainedData);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(RecvMessage);
		}
		m_sRetainedData.clear();
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
			ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, std::vector<byte>(&sData[0], &sData[iPos]));
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
					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, sData);
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
				ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, sMessage);
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
					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, sData.substr(0, iEnd));
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

	void CPluginProtocolHTTP::ProcessInbound(const ReadMessage* Message)
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

		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		// HTML is non binary so use strings
		std::string		sData(m_sRetainedData.begin(), m_sRetainedData.end());

		m_Status = 0;
		m_ContentLength = 0;
		m_Chunked = false;
		m_RemainingChunk = 0;

		// Process response header (HTTP/1.1 200 OK)
		std::string		sFirstLine = sData.substr(0, sData.find_first_of('\r'));
		sFirstLine = sFirstLine.substr(sFirstLine.find_first_of(' ') + 1);
		sFirstLine = sFirstLine.substr(0, sFirstLine.find_first_of(' '));
		m_Status = atoi(sFirstLine.c_str());
		sData = sData.substr(sData.find_first_of('\n') + 1);

		// Remove headers
		PyObject *pHeaderDict = PyDict_New();
		while (sData.length() && (sData[0] != '\r'))
		{
			std::string		sHeaderLine = sData.substr(0, sData.find_first_of('\r'));
			std::string		sHeaderName = sData.substr(0, sHeaderLine.find_first_of(':'));
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
			if (PyDict_SetItemString(pHeaderDict, sHeaderName.c_str(), pObj) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
			}
			Py_DECREF(pObj);
			sData = sData.substr(sData.find_first_of('\n') + 1);
		}

		// not enough data arrived to complete header processing
		if (!sData.length())
		{
			Py_DECREF(pHeaderDict);
			return;
		}

		m_Headers = pHeaderDict;
		sData = sData.substr(sData.find_first_of('\n') + 1);		// skip over 2nd new line char

		// Process the message body
		if (m_Status)
		{
			if (!m_Chunked)
			{
				// If full message then return it
				if (m_ContentLength == sData.length())
				{
					std::vector<byte>	vData(sData.c_str(), sData.c_str() + sData.length());
					ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, vData, m_Status, (PyObject*)m_Headers);
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(RecvMessage);
					m_sRetainedData.clear();
				}
			}
			else
			{
				// Process available chunks
				std::vector<byte>	vHTML;
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
							ReceivedMessage*	RecvMessage = new ReceivedMessage(Message->m_HwdID, vHTML, m_Status, (PyObject*)m_Headers);
							boost::lock_guard<boost::mutex> l(PluginMutex);
							PluginMessageQueue.push(RecvMessage);
							m_sRetainedData.clear();
							break;
						}
					}

					if (sData.length() <= m_RemainingChunk)		// Read data is just part of a chunk
					{
						Py_DECREF(pHeaderDict);
						break;
					}

					vHTML.insert(vHTML.end(), sData.c_str(), sData.c_str() + m_RemainingChunk);
					sData = sData.substr(m_RemainingChunk);
					m_RemainingChunk = 0;
				}
			}
		}
	}

	std::vector<byte>	CPluginProtocolHTTP::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;
		std::string	sHttpRequest = "GET ";
		// Create first line of the request.
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

		if (WriteMessage->m_Operation.length())
		{
			sHttpRequest = WriteMessage->m_Operation + " ";
		}

		if (WriteMessage->m_URL.length())
		{
			sHttpRequest += WriteMessage->m_URL + " ";
		}
		else
		{
			sHttpRequest = "/ ";
		}
		sHttpRequest += "HTTP/1.1\r\n";

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
			sHttpRequest += "Authorization:Basic " + encodedAuth + "\r\n";
		}

		// Did we get headers to send?
		if (WriteMessage->m_Object)
		{
			if ((((PyObject*)WriteMessage->m_Object)->ob_type->tp_flags & (Py_TPFLAGS_DICT_SUBCLASS)) != 0)
			{
				PyObject*	pHeaders = (PyObject*)WriteMessage->m_Object;
				PyObject *key, *value;
				Py_ssize_t pos = 0;
				while (PyDict_Next(pHeaders, &pos, &key, &value))
				{
					PyObject*	pKeyBytes = PyUnicode_AsASCIIString(key);
					std::string	sKey = PyBytes_AsString(pKeyBytes);
					Py_DECREF(pKeyBytes);
					PyObject*	pValueBytes = PyUnicode_AsASCIIString(value);
					std::string	sValue = PyBytes_AsString(pValueBytes);
					Py_DECREF(pValueBytes);
					sHttpRequest += sKey + ": " + sValue + "\r\n";
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) HTTP Request header parameter was not a dictionary, ignored.", __func__);
			}
		}

		std::string StringBuffer(WriteMessage->m_Buffer.begin(), WriteMessage->m_Buffer.end());
		sHttpRequest += "\r\n" + StringBuffer;

		retVal.assign(sHttpRequest.c_str(), sHttpRequest.c_str() + sHttpRequest.length());
		return retVal;
	}
}
#endif