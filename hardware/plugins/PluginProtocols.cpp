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

	extern 	std::queue<CPluginMessage>	PluginMessageQueue;
	extern	boost::mutex PluginMutex;


	void CPluginProtocol::ProcessInbound(const int HwdID, std::string &ReadData)
	{
		// Raw protocol is to just always dispatch data to plugin without interpretation
		CPluginMessage	Message(PMT_Message, HwdID, ReadData);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	std::string CPluginProtocol::ProcessOutbound(const CPluginMessage & WriteMessage)
	{
		return WriteMessage.m_Message;
	}

	void CPluginProtocol::Flush(const int HwdID)
	{
		// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
		CPluginMessage	Message(PMT_Message, HwdID, m_sRetainedData);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
		m_sRetainedData.clear();
	}

	void CPluginProtocolLine::ProcessInbound(const int HwdID, std::string & ReadData)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

		int iPos = sData.find_first_of('\r');		//  Look for message terminator 
		while (iPos != std::string::npos)
		{
			CPluginMessage	Message(PMT_Message, HwdID, sData.substr(0, iPos));
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			if (sData[iPos + 1] == '\n') iPos++;				//  Handle \r\n 
			sData = sData.substr(iPos + 1);
			iPos = sData.find_first_of('\r');
		}

		m_sRetainedData = sData; // retain any residual for next time
	}

	void CPluginProtocolJSON::ProcessInbound(const int HwdID, std::string & ReadData)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

		int iPos = 1;
		while (iPos) {
			iPos = sData.find("}{", 0) + 1;		//  Look for message separater in case there is more than one
			if (!iPos) // no, just one or part of one
			{
				if ((sData.substr(sData.length() - 1, 1) == "}") &&
					(std::count(sData.begin(), sData.end(), '{') == std::count(sData.begin(), sData.end(), '}'))) // whole message so queue the whole buffer
				{
					CPluginMessage	Message(PMT_Message, HwdID, sData);
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(Message);
					}
					sData = "";
				}
			}
			else  // more than one message so queue the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				CPluginMessage	Message(PMT_Message, HwdID, sMessage);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		m_sRetainedData = sData; // retain any residual for next time
	}

	void CPluginProtocolXML::ProcessInbound(const int HwdID, std::string & ReadData)
	{
		//
		//	Only returns whole XML messages. Does not handle <tag /> as the top level tag
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in
		try
		{
			while (true)
			{
				//
				//	Find the top level tag name if it is not set
				//
				if (!m_Tag.length())
				{
					int iStart = sData.find_first_of('<');
					if (iStart == std::string::npos)
					{
						// start of a tag not found so discard
						m_sRetainedData.clear();
						break;
					}
					sData = sData.substr(iStart);					// remove any leading data
					int iEnd = sData.find_first_of('>');
					if (iEnd != std::string::npos)
					{
						m_Tag = sData.substr(1, (iEnd - 1));
					}
				}

				int	iPos = sData.find("</" + m_Tag + ">");
				if (iPos != std::string::npos)
				{
					int iEnd = iPos + m_Tag.length() + 3;
					CPluginMessage	Message(PMT_Message, HwdID, sData.substr(0, iEnd));
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(Message);
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

		m_sRetainedData = sData; // retain any residual for next time
	}

	void CPluginProtocolHTTP::ProcessInbound(const int HwdID, std::string& ReadData)
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

		// is this the start of a response?
		if (!m_Status)
		{
			std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

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
				std::string		sHeaderText = sHeaderLine.substr(sHeaderName.length() + 2);
				if (sHeaderName == "Content-Length")
				{
					m_ContentLength = atoi(sHeaderText.c_str());
				}
				if ((sHeaderName == "Transfer-Encoding") && (sHeaderText == "chunked"))
				{
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
				m_sRetainedData += ReadData; // retain any residual for next time
				m_Status = 0;
				m_ContentLength = 0;
				Py_DECREF(pHeaderDict);
				return;
			}

			m_Headers = pHeaderDict;
			sData = sData.substr(sData.find_first_of('\n') + 1);		// skip over 2nd new line char
			m_sRetainedData.clear();
			ReadData = sData;
		}

		// Process the message body
		if (m_Status)
		{
			if (!m_Chunked)
			{
				std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

																 // If full message then return it
				if (m_ContentLength == sData.length())
				{
					CPluginMessage	Message(PMT_Message, HwdID, sData);
					Message.m_iLevel = m_Status;
					Message.m_Object = m_Headers;
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);

					m_Status = 0;
					m_ContentLength = 0;
					m_Headers = NULL;
					m_sRetainedData.clear();
				}
				else
					m_sRetainedData += ReadData; // retain any residual for next time
			}
			else
			{
				// Process available chunks
				while (ReadData.length() && (ReadData != "\r\n"))
				{
					if (!m_RemainingChunk)	// Not processing a chunk so we should be at the start of one
					{
						if (ReadData[0] == '\r')
						{
							ReadData = ReadData.substr(ReadData.find_first_of('\n') + 1);
						}
						std::string		sChunkLine = ReadData.substr(0, ReadData.find_first_of('\r'));
						m_RemainingChunk = strtol(sChunkLine.c_str(), NULL, 16);
						ReadData = ReadData.substr(ReadData.find_first_of('\n') + 1);
						if (!m_RemainingChunk)	// last chunk is zero length
						{
							CPluginMessage	Message(PMT_Message, HwdID, m_sRetainedData);
							Message.m_iLevel = m_Status;
							Message.m_Object = m_Headers;
							boost::lock_guard<boost::mutex> l(PluginMutex);
							PluginMessageQueue.push(Message);

							m_Status = 0;
							m_ContentLength = 0;
							m_Headers = NULL;
							m_sRetainedData.clear();
							break;
						}
					}

					if (ReadData.length() <= m_RemainingChunk)		// Read data is just part of a chunk
					{
						m_sRetainedData += ReadData;
						m_RemainingChunk -= ReadData.length();
						break;
					}

					m_sRetainedData += ReadData.substr(0, m_RemainingChunk);
					ReadData = ReadData.substr(m_RemainingChunk);
					m_RemainingChunk = 0;
				}
			}
		}
	}

	std::string	CPluginProtocolHTTP::ProcessOutbound(const CPluginMessage & WriteMessage)
	{
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

		if (WriteMessage.m_Operation.length())
		{
			sHttpRequest = WriteMessage.m_Operation + " ";
		}

		if (WriteMessage.m_Address.length())
		{
			sHttpRequest += WriteMessage.m_Address + " ";
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
		if (WriteMessage.m_Object)
		{
			if ((((PyObject*)WriteMessage.m_Object)->ob_type->tp_flags & (Py_TPFLAGS_DICT_SUBCLASS)) != 0)
			{
				PyObject*	pHeaders = (PyObject*)WriteMessage.m_Object;
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

		sHttpRequest += "\r\n" + WriteMessage.m_Message;
		return sHttpRequest;
	}
}
#endif