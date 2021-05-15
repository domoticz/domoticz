#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "../../main/Helper.h"
#include "../../main/json_helper.h"
#include "../../main/Logger.h"
#include "../../webserver/Base64.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace Plugins {

	CPluginProtocol *CPluginProtocol::Create(const std::string &sProtocol)
	{
		if (sProtocol == "Line") return (CPluginProtocol*) new CPluginProtocolLine();
		if (sProtocol == "XML")
			return (CPluginProtocol *)new CPluginProtocolXML();
		if (sProtocol == "JSON")
			return (CPluginProtocol *)new CPluginProtocolJSON();
		if ((sProtocol == "HTTP") || (sProtocol == "HTTPS"))
		{
			CPluginProtocolHTTP* pProtocol = new CPluginProtocolHTTP(sProtocol == "HTTPS");
			return (CPluginProtocol*)pProtocol;
		}
		if (sProtocol == "ICMP")
			return (CPluginProtocol *)new CPluginProtocolICMP();
		if ((sProtocol == "MQTT") || (sProtocol == "MQTTS"))
		{
			CPluginProtocolMQTT* pProtocol = new CPluginProtocolMQTT(sProtocol == "MQTTS");
			return (CPluginProtocol*)pProtocol;
		}
		if ((sProtocol == "WS") || (sProtocol == "WSS"))
		{
			CPluginProtocolWS* pProtocol = new CPluginProtocolWS(sProtocol == "WSS");
			return (CPluginProtocol*)pProtocol;
		}
		return new CPluginProtocol();
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
			const char* pData = PyBytes_AsString(WriteMessage->m_Object);
			int			iSize = PyBytes_Size(WriteMessage->m_Object);
			retVal.reserve((size_t)iSize);
			retVal.assign(pData, pData + iSize);
		}
		// Handle ByteArray objects
		else if ((((PyObject*)WriteMessage->m_Object)->ob_type->tp_name == std::string("bytearray")))
		{
			size_t	len = PyByteArray_Size(WriteMessage->m_Object);
			char* data = PyByteArray_AsString(WriteMessage->m_Object);
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

	void CPluginProtocol::Flush(CPlugin *pPlugin, CConnection *pConnection)
	{
		if (!m_sRetainedData.empty())
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

	static void AddBytesToDict(PyObject* pDict, const char* key, const std::string& value)
	{
		PyNewRef pObj = Py_BuildValue("y#", value.c_str(), value.length());
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	}

	static void AddStringToDict(PyObject* pDict, const char* key, const std::string& value)
	{
		PyNewRef pObj = Py_BuildValue("s#", value.c_str(), value.length());
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	}

	static void AddIntToDict(PyObject* pDict, const char* key, const int value)
	{
		PyNewRef pObj = Py_BuildValue("i", value);
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	}

	static void AddUIntToDict(PyObject* pDict, const char* key, const unsigned int value)
	{
		PyNewRef pObj = Py_BuildValue("I", value);
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	}

	static void AddDoubleToDict(PyObject* pDict, const char* key, const double value)
	{
		PyNewRef pObj = Py_BuildValue("d", value);
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%f' to dictionary.", __func__, key, value);
	}

	static void AddBoolToDict(PyObject* pDict, const char* key, const bool value)
	{
		PyNewRef pObj = Py_BuildValue("N", PyBool_FromLong(value));
		if (PyDict_SetItemString(pDict, key, pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	}

	PyObject* CPluginProtocolJSON::JSONtoPython(Json::Value* pJSON)
	{
		PyObject *pRetVal = nullptr;

		if (pJSON->isArray())
		{
			pRetVal = PyList_New(pJSON->size());
			Py_ssize_t	Index = 0;
			for (auto &pRef : *pJSON)
			{
				if (pRef.isArray() || pRef.isObject())
				{
					PyObject* pObj = JSONtoPython(&pRef);
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1))
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for object.", __func__, Index - 1);
				}
				else if (pRef.isUInt())
				{
					PyObject *pObj = Py_BuildValue("I", pRef.asUInt());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1))  // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for unsigned integer.", __func__, Index - 1);
				}
				else if (pRef.isInt())
				{
					PyObject *pObj = Py_BuildValue("i", pRef.asInt());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1)) // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for integer.", __func__, Index - 1);
				}
				else if (pRef.isDouble())
				{
					PyObject *pObj = Py_BuildValue("d", pRef.asDouble());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1)) // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for double.", __func__, Index - 1);
				}
				else if (pRef.isConvertibleTo(Json::stringValue))
				{
					std::string sString = pRef.asString();
					PyObject* pObj = Py_BuildValue("s#", sString.c_str(), sString.length());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1)) // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for string.", __func__, Index - 1);
				}
				else
					_log.Log(LOG_ERROR, "(%s) failed to process entry.", __func__);
			}
		}
		else if (pJSON->isObject())
		{
			pRetVal = PyDict_New();
			for (Json::ValueIterator it = pJSON->begin(); it != pJSON->end(); ++it)
			{
				std::string KeyName = it.name();
				Json::ValueIterator::reference pRef = *it;
				if (pRef.isArray() || pRef.isObject())
				{
					PyObject* pObj = JSONtoPython(&pRef);
					if (!pObj || (PyDict_SetItemString(pRetVal, KeyName.c_str(), pObj) == -1))
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', to dictionary for object.", __func__, KeyName.c_str());
				}
				else if (pRef.isUInt())
					AddUIntToDict(pRetVal, KeyName.c_str(), pRef.asUInt());
				else if (pRef.isInt())
					AddIntToDict(pRetVal, KeyName.c_str(), pRef.asInt());
				else if (pRef.isBool())
					AddBoolToDict(pRetVal, KeyName.c_str(), pRef.asInt());
				else if (pRef.isDouble())
					AddDoubleToDict(pRetVal, KeyName.c_str(), pRef.asDouble());
				else if (pRef.isConvertibleTo(Json::stringValue))
					AddStringToDict(pRetVal, KeyName.c_str(), pRef.asString());
				else _log.Log(LOG_ERROR, "(%s) failed to process entry for '%s'.", __func__, KeyName.c_str());
			}
		}
		return pRetVal;
	}

	PyObject *CPluginProtocolJSON::JSONtoPython(const std::string &sData)
	{
		Json::Value		root;
		PyObject* pRetVal = Py_None;

		bool bRet = ParseJSon(sData, root);
		if ((!bRet) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "JSON Protocol: Parse Error on '%s'", sData.c_str());
			Py_INCREF(Py_None);
		}
		else
		{
			pRetVal = JSONtoPython(&root);
		}

		return pRetVal;
	}

	std::string CPluginProtocolJSON::PythontoJSON(PyObject* pObject)
	{
		std::string	sJson;

		if (PyUnicode_Check(pObject))
		{
			sJson += '"' + std::string(PyUnicode_AsUTF8(pObject)) + '"';
		}
		else if (pObject->ob_type->tp_name == std::string("bool"))
		{
			sJson += (PyObject_IsTrue(pObject) ? "true" : "false");
		}
		else if (PyLong_Check(pObject))
		{
			sJson += std::to_string(PyLong_AsLong(pObject));
		}
		else if (PyBytes_Check(pObject))
		{
			sJson += '"' + std::string(PyBytes_AsString(pObject)) + '"';
		}
		else if (pObject->ob_type->tp_name == std::string("bytearray"))
		{
			sJson += '"' + std::string(PyByteArray_AsString(pObject)) + '"';
		}
		else if (pObject->ob_type->tp_name == std::string("float"))
		{
			sJson += std::to_string(PyFloat_AsDouble(pObject));
		}
		else if (PyDict_Check(pObject))
		{
			sJson += "{ ";
			PyObject* key, * value;
			Py_ssize_t pos = 0;
			while (PyDict_Next(pObject, &pos, &key, &value))
			{
				sJson += PythontoJSON(key) + ':' + PythontoJSON(value) + ',';
			}
			sJson[sJson.length() - 1] = '}';
		}
		else if (PyList_Check(pObject))
		{
			sJson += "[ ";
			for (Py_ssize_t i = 0; i < PyList_Size(pObject); i++)
			{
				sJson += PythontoJSON(PyList_GetItem(pObject, i)) + ',';
			}
			sJson[sJson.length() - 1] = ']';
		}
		else if (PyTuple_Check(pObject))
		{
			sJson += "[ ";
			for (Py_ssize_t i = 0; i < PyTuple_Size(pObject); i++)
			{
				sJson += PythontoJSON(PyTuple_GetItem(pObject, i)) + ',';
			}
			sJson[sJson.length() - 1] = ']';
		}

		return sJson;
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
			Json::Value		root;
			iPos = sData.find("}{", 0) + 1;		//  Look for message separater in case there is more than one
			if (!iPos) // no, just one or part of one
			{
				if ((sData.substr(sData.length() - 1, 1) == "}") &&
					(std::count(sData.begin(), sData.end(), '{') == std::count(sData.begin(), sData.end(), '}'))) // whole message so queue the whole buffer
				{
					bool bRet = ParseJSon(sData, root);
					if ((!bRet) || (!root.isObject()))
					{
						_log.Log(LOG_ERROR, "JSON Protocol: Parse Error on '%s'", sData.c_str());
						Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, sData));
					}
					else
					{
						PyObject* pMessage = JSONtoPython(&root);
						Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pMessage));
					}
					sData.clear();
				}
			}
			else  // more than one message so queue the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				bool bRet = ParseJSon(sMessage, root);
				if ((!bRet) || (!root.isObject()))
				{
					_log.Log(LOG_ERROR, "JSON Protocol: Parse Error on '%s'", sData.c_str());
					Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, sMessage));
				}
				else
				{
					PyObject* pMessage = JSONtoPython(&root);
					Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pMessage));
				}
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
		catch (std::exception const& exc)
		{
			_log.Log(LOG_ERROR, "(CPluginProtocolXML::ProcessInbound) Unexpected exception thrown '%s', Data '%s'.", exc.what(), sData.c_str());
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	void CPluginProtocolHTTP::ExtractHeaders(std::string* pData)
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
			PyNewRef		pObj = Py_BuildValue("s", sHeaderText.c_str());
			PyBorrowedRef	pPrevObj = PyDict_GetItemString((PyObject *)m_Headers, sHeaderName.c_str());
			// Encode multi headers in a list
			if (pPrevObj)
			{
				PyObject* pListObj = pPrevObj;
				// First duplicate? Create a list and add previous value
				if (!PyList_Check(pListObj))
				{
					pListObj = PyList_New(1);
					if (!pListObj)
					{
						_log.Log(LOG_ERROR, "(%s) failed to create list to handle duplicate header. Name '%s'.", __func__, sHeaderName.c_str());
						return;
					}
					PyList_SetItem(pListObj, 0, pPrevObj);
					Py_INCREF(pPrevObj);
					PyDict_SetItemString((PyObject*)m_Headers, sHeaderName.c_str(), pListObj);
					Py_DECREF(pListObj);
				}
				// Append new value to the list
				if (PyList_Append(pListObj, pObj) == -1) {
					_log.Log(LOG_ERROR, "(%s) failed to append to list key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
				}
			}
			else if (PyDict_SetItemString((PyObject*)m_Headers, sHeaderName.c_str(), pObj) == -1) {
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to headers.", __func__, sHeaderName.c_str(), sHeaderText.c_str());
			}
			*pData = pData->substr(pData->find_first_of('\n') + 1);
		}
	}

	void CPluginProtocolHTTP::Flush(CPlugin *pPlugin, CConnection *pConnection)
	{
		if (!m_sRetainedData.empty())
		{
			// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
			ProcessInbound(new ReadEvent(pPlugin, pConnection, 0, nullptr));
			m_sRetainedData.clear();
		}
	}

	void CPluginProtocolHTTP::ProcessInbound(const ReadEvent* Message)
	{
		// There won't be a buffer if the connection closed
		if (!Message->m_Buffer.empty())
		{
			m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());
		}

		// HTML is non binary so use strings
		std::string		sData(m_sRetainedData.begin(), m_sRetainedData.end());

		m_ContentLength = -1;
		m_Chunked = false;
		m_RemainingChunk = 0;

		// Need at least the whole of the first line before going any further; otherwise attempting to parse it
		// will end badly.
		if (sData.find("\r\n") == std::string::npos)
		{
		        return;
		}

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
					if ((m_ContentLength == sData.length()) || (Message->m_Buffer.empty()))
					{
						PyObject* pDataDict = PyDict_New();
						PyNewRef pObj = Py_BuildValue("s", m_Status.c_str());
						if (PyDict_SetItemString(pDataDict, "Status", pObj) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Status", m_Status.c_str());

						if (m_Headers)
						{
							if (PyDict_SetItemString(pDataDict, "Headers", (PyObject*)m_Headers) == -1)
								_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
							Py_DECREF((PyObject*)m_Headers);
							m_Headers = nullptr;
						}

						if (sData.length())
						{
							PyNewRef pObj = Py_BuildValue("y#", sData.c_str(), sData.length());
							if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
								_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sData.c_str());
						}

						Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pDataDict));
						m_sRetainedData.clear();
					}
				}
				else
				{
					// Process available chunks
					std::string		sPayload;
					while (sData.length() >= 2 && (sData != "\r\n"))
					{
						if (!m_RemainingChunk)	// Not processing a chunk so we should be at the start of one
						{
						        // Skip terminating \r\n from previous chunk
							if (sData[0] == '\r')
							{
								sData = sData.substr(sData.find_first_of('\n') + 1);
							}
							// Stop if we have not received the complete chunk size terminator yet
							size_t uSizeEnd = sData.find_first_of('\r');
							if (uSizeEnd == std::string::npos || sData.find_first_of('\n', uSizeEnd + 1) == std::string::npos)
							{
							        break;
							}
							std::string		sChunkLine = sData.substr(0, uSizeEnd);
							m_RemainingChunk = strtol(sChunkLine.c_str(), nullptr, 16);
							sData = sData.substr(sData.find_first_of('\n') + 1);

							// last chunk is zero length, but still has a terminator.  We aren't done until we have received the terminator as well
							if (m_RemainingChunk == 0 && (sData.find_first_of('\n') != std::string::npos))
							{
								PyObject* pDataDict = PyDict_New();
								PyNewRef pObj = Py_BuildValue("s", m_Status.c_str());
								if (PyDict_SetItemString(pDataDict, "Status", pObj) == -1)
									_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Status", m_Status.c_str());

								if (m_Headers)
								{
									if (PyDict_SetItemString(pDataDict, "Headers", (PyObject*)m_Headers) == -1)
										_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
									Py_DECREF((PyObject*)m_Headers);
									m_Headers = nullptr;
								}

								if (sPayload.length())
								{
									PyNewRef pObj = Py_BuildValue("y#", sPayload.c_str(), sPayload.length());
									if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
										_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
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
			if (sData.substr(0, 2) == "\r\n")
			{
				std::string		sPayload = sData.substr(2);
				// No payload || we have the payload || the connection has closed
				if ((m_ContentLength == -1) || (m_ContentLength == sPayload.length()) || Message->m_Buffer.empty())
				{
					PyObject* DataDict = PyDict_New();
					std::string		sVerb = sFirstLine.substr(0, sFirstLine.find_first_of(' '));
					PyNewRef pObj = Py_BuildValue("s", sVerb.c_str());
					if (PyDict_SetItemString(DataDict, "Verb", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Verb", sVerb.c_str());

					std::string		sURL = sFirstLine.substr(sVerb.length() + 1, sFirstLine.find_first_of(' ', sVerb.length() + 1));
					PyNewRef pURL = Py_BuildValue("s", sURL.c_str());
					if (PyDict_SetItemString(DataDict, "URL", pURL) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "URL", sURL.c_str());

					if (m_Headers)
					{
						if (PyDict_SetItemString(DataDict, "Headers", (PyObject*)m_Headers) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "HTTP", "Headers");
						Py_DECREF((PyObject*)m_Headers);
						m_Headers = nullptr;
					}

					if (sPayload.length())
					{
						PyNewRef pObj = Py_BuildValue("y#", sPayload.c_str(), sPayload.length());
						if (PyDict_SetItemString(DataDict, "Data", pObj) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
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

		// Extract potential values.  Failures return nullptr, success returns borrowed reference
		PyBorrowedRef	pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
		PyBorrowedRef	pStatus = PyDict_GetItemString(WriteMessage->m_Object, "Status");
		PyBorrowedRef	pChunk = PyDict_GetItemString(WriteMessage->m_Object, "Chunk");
		PyBorrowedRef	pHeaders = PyDict_GetItemString(WriteMessage->m_Object, "Headers");
		PyBorrowedRef	pData = PyDict_GetItemString(WriteMessage->m_Object, "Data");

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
			stdupper(sHttp);
			sHttp += " ";

			PyBorrowedRef	pURL = PyDict_GetItemString(WriteMessage->m_Object, "URL");
			std::string	sHttpURL = "/";
			if (pURL && PyUnicode_Check(pURL))
			{
				sHttpURL = PyUnicode_AsUTF8(pURL);
			}
			sHttp += sHttpURL;
			sHttp += " HTTP/1.1\r\n";

			// If username &/or password specified then add a basic auth header (if one was not supplied)
			PyBorrowedRef pHead;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Authorization");
			if (!pHead)
			{
				std::string		User;
				std::string		Pass;
				PyObject*		pModule = (PyObject*)WriteMessage->m_pPlugin->PythonModule();
				PyNewRef		pDict = PyObject_GetAttrString(pModule, "Parameters");
				if (pDict)
				{
					PyBorrowedRef pUser = PyDict_GetItemString(pDict, "Username");
					if (pUser) User = PyUnicode_AsUTF8(pUser);
					PyBorrowedRef pPass = PyDict_GetItemString(pDict, "Password");
					if (pPass) Pass = PyUnicode_AsUTF8(pPass);
				}
				if (User.length() > 0 || Pass.length() > 0)
				{
					std::string auth;
					if (User.length() > 0)
					{
						auth += User;
					}
					auth += ":";
					if (Pass.length() > 0)
					{
						auth += Pass;
					}
					std::string encodedAuth = base64_encode(auth);
					sHttp += "Authorization: Basic " + encodedAuth + "\r\n";
				}
			}

			// Add Server header if it is not supplied
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
				_log.Log(LOG_ERROR, "(%s) HTTP 'Status' dictionary entry was not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}

			sHttp = "HTTP/1.1 ";
			sHttp += PyUnicode_AsUTF8(pStatus);
			sHttp += "\r\n";

			// Add Date header if it is not supplied
			PyObject *pHead = nullptr;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Date");
			if (!pHead)
			{
				char szDate[100];
				time_t rawtime;
				struct tm* info;
				time(&rawtime);
				info = gmtime(&rawtime);
				if (0 < strftime(szDate, sizeof(szDate), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", info))	sHttp += szDate;
			}

			// Add Server header if it is not supplied
			pHead = nullptr;
			if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Server");
			if (!pHead)
			{
				sHttp += "Server: Domoticz/1.0\r\n";
			}
		}
		//
		//	Only other valid options is a chunk response
		//
		else if (!pChunk)
		{
			_log.Log(LOG_ERROR, "(%s) HTTP unable to determine send type. 'Verb', 'Status' or 'Chunk' dictionary entries not found, ignored. See Python Plugin wiki page for help.", __func__);
			return retVal;
		}

		// Handle headers for normal Requests & Responses
		if (pVerb || pStatus)
		{
			// Did we get headers to send?
			if (pHeaders)
			{
				if (PyDict_Check(pHeaders))
				{
					PyObject* key, * value;
					Py_ssize_t pos = 0;
					while (PyDict_Next(pHeaders, &pos, &key, &value))
					{
						std::string	sKey = PyUnicode_AsUTF8(key);
						if (PyUnicode_Check(value))
						{
							std::string	sValue = PyUnicode_AsUTF8(value);
							sHttp += sKey + ": " + sValue + "\r\n";
						}
						else if (PyBytes_Check(value))
						{
							const char* pBytes = PyBytes_AsString(value);
							sHttp += sKey + ": " + pBytes + "\r\n";
						}
						else if (value->ob_type->tp_name == std::string("bytearray"))
						{
							const char* pByteArray = PyByteArray_AsString(value);
							sHttp += sKey + ": " + pByteArray + "\r\n";
						}
						else if (PyList_Check(value))
						{
							PyObject* iterator = PyObject_GetIter(value);
							PyObject* item;
							while ((item = PyIter_Next(iterator)))
							{
								if (PyUnicode_Check(item))
								{
									std::string	sValue = PyUnicode_AsUTF8(item);
									sHttp += sKey + ": " + sValue + "\r\n";
								}
								else if (PyBytes_Check(item))
								{
									const char* pBytes = PyBytes_AsString(item);
									sHttp += sKey + ": " + pBytes + "\r\n";
								}
								else if (item->ob_type->tp_name == std::string("bytearray"))
								{
									const char* pByteArray = PyByteArray_AsString(item);
									sHttp += sKey + ": " + pByteArray + "\r\n";
								}
								Py_DECREF(item);
							}

							Py_DECREF(iterator);
						}
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) HTTP 'Headers' parameter was not a dictionary, ignored.", __func__);
				}
			}

			// Add Content-Length header if it is required but not supplied
			PyBorrowedRef pLength = nullptr;
			if (pHeaders)
				pLength = PyDict_GetItemString(pHeaders, "Content-Length");
			if (!pLength && pData && !pChunk)
			{
				Py_ssize_t iLength = 0;
				if (PyUnicode_Check(pData))
					iLength = PyUnicode_GetLength(pData);
				else if (pData->ob_type->tp_name == std::string("bytearray"))
					iLength = PyByteArray_Size(pData);
				else if (PyBytes_Check(pData))
					iLength = PyBytes_Size(pData);
				sHttp += "Content-Length: " + std::to_string(iLength) + "\r\n";
			}

			// Add Transfer-Encoding header if required but not supplied
			if (pChunk)
			{
				PyBorrowedRef pHead = nullptr;
				if (pHeaders) pHead = PyDict_GetItemString(pHeaders, "Transfer-Encoding");
				if (!pHead)
				{
					sHttp += "Transfer-Encoding: chunked\r\n";
				}
			}

			// Terminate preamble
			sHttp += "\r\n";
		}

		// Chunks require hex encoded chunk length instead of normal response
		if (pChunk)
		{
			long	lChunkLength = 0;
			if (pData)
			{
				if (PyUnicode_Check(pData))
					lChunkLength = PyUnicode_GetLength(pData);
				else if (pData->ob_type->tp_name == std::string("bytearray"))
					lChunkLength = PyByteArray_Size(pData);
				else if (PyBytes_Check(pData))
					lChunkLength = PyBytes_Size(pData);
			}
			std::stringstream stream;
			stream << std::hex << lChunkLength;
			sHttp += std::string(stream.str());
			sHttp += "\r\n";
		}

		// Append data if supplied (for POST) or Response
		if (pData && PyUnicode_Check(pData))
		{
			sHttp += PyUnicode_AsUTF8(pData);
			retVal.reserve(sHttp.length() + 2);
			retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());
		}
		else if (pData && (pData->ob_type->tp_name == std::string("bytearray")))
		{
			retVal.reserve(sHttp.length() + PyByteArray_Size(pData) + 2);
			retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());
			const char* pByteArray = PyByteArray_AsString(pData);
			int iStop = PyByteArray_Size(pData);
			for (int i = 0; i < iStop; i++)
			{
				retVal.push_back(pByteArray[i]);
			}
		}
		else if (pData && PyBytes_Check(pData))
		{
			retVal.reserve(sHttp.length() + PyBytes_Size(pData) + 2);
			retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());
			const char* pBytes = PyBytes_AsString(pData);
			int iStop = PyBytes_Size(pData);
			for (int i = 0; i < iStop; i++)
			{
				retVal.push_back(pBytes[i]);
			}
		}
		else
		{
			retVal.reserve(sHttp.length() + 2);
			retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());
		}

		// Chunks require additional CRLF (hence '+2' on all vector reserves to make sure there is room)
		if (pChunk)
		{
			retVal.push_back('\r');
			retVal.push_back('\n');
		}

		return retVal;
	}

	void CPluginProtocolICMP::ProcessInbound(const ReadEvent* Message)
	{
		PyNewRef pObj = nullptr;
		PyObject* pDataDict = PyDict_New();
		int			iTotalData = 0;
		int			iDataOffset = 0;

		// Handle response
		if (!Message->m_Buffer.empty())
		{
			PyObject* pIPv4Dict = PyDict_New();
			if (pDataDict && pIPv4Dict)
			{
				if (PyDict_SetItemString(pDataDict, "IPv4", (PyObject*)pIPv4Dict) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "ICMP", "IPv4");
				else
				{
					Py_XDECREF((PyObject*)pIPv4Dict);

					ipv4_header* pIPv4 = (ipv4_header*)(&Message->m_Buffer[0]);

					pObj = Py_BuildValue("s", pIPv4->source_address().to_string().c_str());
					PyDict_SetItemString(pIPv4Dict, "Source", pObj);

					pObj = Py_BuildValue("s", pIPv4->destination_address().to_string().c_str());
					PyDict_SetItemString(pIPv4Dict, "Destination", pObj);

					pObj = Py_BuildValue("b", pIPv4->version());
					PyDict_SetItemString(pIPv4Dict, "Version", pObj);

					pObj = Py_BuildValue("b", pIPv4->protocol());
					PyDict_SetItemString(pIPv4Dict, "Protocol", pObj);

					pObj = Py_BuildValue("b", pIPv4->type_of_service());
					PyDict_SetItemString(pIPv4Dict, "TypeOfService", pObj);

					pObj = Py_BuildValue("h", pIPv4->header_length());
					PyDict_SetItemString(pIPv4Dict, "HeaderLength", pObj);

					pObj = Py_BuildValue("h", pIPv4->total_length());
					PyDict_SetItemString(pIPv4Dict, "TotalLength", pObj);

					pObj = Py_BuildValue("h", pIPv4->identification());
					PyDict_SetItemString(pIPv4Dict, "Identification", pObj);

					pObj = Py_BuildValue("h", pIPv4->header_checksum());
					PyDict_SetItemString(pIPv4Dict, "HeaderChecksum", pObj);

					pObj = Py_BuildValue("i", pIPv4->time_to_live());
					PyDict_SetItemString(pIPv4Dict, "TimeToLive", pObj);

					iTotalData = pIPv4->total_length();
					iDataOffset = pIPv4->header_length();
				}
				pIPv4Dict = nullptr;
			}

			PyObject* pIcmpDict = PyDict_New();
			if (pDataDict && pIcmpDict)
			{
				if (PyDict_SetItemString(pDataDict, "ICMP", (PyObject*)pIcmpDict) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "ICMP", "ICMP");
				else
				{
					Py_XDECREF((PyObject*)pIcmpDict);

					icmp_header* pICMP = (icmp_header*)(&Message->m_Buffer[0] + 20);
					if ((pICMP->type() == icmp_header::echo_reply) && (Message->m_ElapsedMs >= 0))
					{
						pObj = Py_BuildValue("I", Message->m_ElapsedMs);
						PyDict_SetItemString(pDataDict, "ElapsedMs", pObj);
					}

					pObj = Py_BuildValue("b", pICMP->type());
					PyDict_SetItemString(pIcmpDict, "Type", pObj);

					pObj = Py_BuildValue("b", pICMP->type());
					PyDict_SetItemString(pDataDict, "Status", pObj);

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

					pObj = Py_BuildValue("b", pICMP->code());
					PyDict_SetItemString(pIcmpDict, "Code", pObj);

					pObj = Py_BuildValue("h", pICMP->checksum());
					PyDict_SetItemString(pIcmpDict, "Checksum", pObj);

					pObj = Py_BuildValue("h", pICMP->identifier());
					PyDict_SetItemString(pIcmpDict, "Identifier", pObj);

					pObj = Py_BuildValue("h", pICMP->sequence_number());
					PyDict_SetItemString(pIcmpDict, "SequenceNumber", pObj);

					iDataOffset += sizeof(icmp_header);
					if (pICMP->type() == icmp_header::destination_unreachable)
					{
						ipv4_header* pIPv4 = (ipv4_header*)(pICMP + 1);
						iDataOffset += pIPv4->header_length() + sizeof(icmp_header);
					}
				}
				pIcmpDict = nullptr;
			}
		}
		else
		{
			pObj = Py_BuildValue("b", icmp_header::time_exceeded);
			PyDict_SetItemString(pDataDict, "Status", pObj);

			pObj = Py_BuildValue("s", "time_exceeded");
			PyDict_SetItemString(pDataDict, "Description", pObj);
		}

		std::string		sData(Message->m_Buffer.begin(), Message->m_Buffer.end());
		sData = sData.substr(iDataOffset, iTotalData - iDataOffset);
		pObj = Py_BuildValue("y#", sData.c_str(), sData.length());
		if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "ICMP", "Data", sData.c_str());

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

	static void MQTTPushBackNumber(int iNumber, std::vector<byte>& vVector)
	{
		vVector.push_back(iNumber / 256);
		vVector.push_back(iNumber % 256);
	}

	static void MQTTPushBackString(const std::string& sString, std::vector<byte>& vVector)
	{
		vVector.insert(vVector.end(), sString.begin(), sString.end());
	}

	static void MQTTPushBackStringWLen(const std::string& sString, std::vector<byte>& vVector)
	{
		MQTTPushBackNumber(sString.length(), vVector);
		vVector.insert(vVector.end(), sString.begin(), sString.end());
	}

	void CPluginProtocolMQTT::ProcessInbound(const ReadEvent* Message)
	{
		if (m_bErrored)
		{
			_log.Log(LOG_ERROR, "(%s) MQTT protocol errored, discarding additional data.", __func__);
			return;
		}

		byte loop = 0;
		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		do {
			std::vector<byte>::iterator it = m_sRetainedData.begin();

			byte		header = *it++;
			byte		bResponseType = header & 0xF0;
			byte		flags = header & 0x0F;
			PyObject* pMqttDict = PyDict_New();
			PyObject *pObj = nullptr;
			uint16_t	iPacketIdentifier = 0;
			long		iRemainingLength = 0;
			long		multiplier = 1;
			byte 		encodedByte;

			do
			{
				encodedByte = *it++;
				iRemainingLength += (encodedByte & 127) * multiplier;
				multiplier *= 128;
				if (multiplier > 128 * 128 * 128)
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

			std::vector<byte>::iterator pktend = it + iRemainingLength;

			switch (bResponseType)
			{
			case MQTT_CONNACK:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("CONNACK"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
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
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			}
			case MQTT_SUBACK:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("SUBACK"));
				iPacketIdentifier = (*it++ << 8) + *it++;
				AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);

				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
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
						PyObject* pResponseDict = PyDict_New();
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
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			}
			case MQTT_PUBACK:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBACK"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength == 2) // check length is correct
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			case MQTT_PUBREC:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBREC"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength == 2) // check length is correct
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			case MQTT_PUBCOMP:
				AddStringToDict(pMqttDict, "Verb", std::string("PUBCOMP"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength == 2) // check length is correct
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			case MQTT_PUBLISH:
			{
				// Fixed Header
				AddStringToDict(pMqttDict, "Verb", std::string("PUBLISH"));
				AddIntToDict(pMqttDict, "DUP", ((flags & 0x08) >> 3));
				long	iQoS = (flags & 0x06) >> 1;
				AddIntToDict(pMqttDict, "QoS", (int)iQoS);
				PyDict_SetItemString(pMqttDict, "Retain", PyBool_FromLong(flags & 0x01));
				// Variable Header
				int		topicLen = (*it++ << 8) + *it++;
				if (topicLen + 2 + (iQoS ? 2 : 0) > iRemainingLength)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u' (iQoS:%ld, topicLen:%d)", __func__, iRemainingLength, bResponseType >> 4, iQoS, topicLen);
					m_bErrored = true;
					break;
				}
				std::string	sTopic((char const*) & *it, topicLen);
				AddStringToDict(pMqttDict, "Topic", sTopic);
				it += topicLen;
				if (iQoS)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				// Payload
				const char *pPayload = (it == pktend) ? nullptr : (const char *)&*it;
				std::string	sPayload(pPayload, std::distance(it, pktend));
				AddBytesToDict(pMqttDict, "Payload", sPayload);
				break;
			}
			case MQTT_UNSUBACK:
				AddStringToDict(pMqttDict, "Verb", std::string("UNSUBACK"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength == 2) // check length is correct
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			case MQTT_PINGRESP:
				AddStringToDict(pMqttDict, "Verb", std::string("PINGRESP"));
				if (flags != 0)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength != 0) // check length is correct
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message length %ld for packet type '%u'", __func__, iRemainingLength, bResponseType >> 4);
					m_bErrored = true;
				}
				break;
			default:
				_log.Log(LOG_ERROR, "(%s) MQTT data invalid: packet type '%d' is unknown.", __func__, bResponseType);
				m_bErrored = true;
			}

			if (!m_bErrored) Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pMqttDict));

			m_sRetainedData.erase(m_sRetainedData.begin(), pktend);
		} while (!m_bErrored && !m_sRetainedData.empty());

		if (m_bErrored)
		{
			_log.Log(LOG_ERROR, "(%s) MQTT protocol violation, sending DisconnectedEvent to Connection.", __func__);
			Message->m_pPlugin->MessagePlugin(new DisconnectedEvent(Message->m_pPlugin, Message->m_pConnection));
		}
	}

	std::vector<byte> CPluginProtocolMQTT::ProcessOutbound(const WriteDirective* WriteMessage)
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

		// Extract potential values.  Failures return nullptr, success returns borrowed reference
		PyBorrowedRef pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
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
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "ID");
				if (pID && PyUnicode_Check(pID))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pID)), vPayload);
				}
				else
					MQTTPushBackStringWLen("Domoticz", vPayload); // TODO: default ID should be more unique, for example "Domoticz_<plugin_name>_<HwID>"

				byte	bCleanSession = 1;
				PyBorrowedRef pCleanSession = PyDict_GetItemString(WriteMessage->m_Object, "CleanSession");
				if (pCleanSession && PyLong_Check(pCleanSession))
				{
					bCleanSession = (byte)PyLong_AsLong(pCleanSession);
				}
				bControlFlags |= (bCleanSession & 1) << 1;

				// Will topic
				PyBorrowedRef pTopic = PyDict_GetItemString(WriteMessage->m_Object, "WillTopic");
				if (pTopic && PyUnicode_Check(pTopic))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
					bControlFlags |= 4;
				}

				// Will QoS, Retain and Message
				if (bControlFlags & 4)
				{
					PyBorrowedRef pQoS = PyDict_GetItemString(WriteMessage->m_Object, "WillQoS");
					if (pQoS && PyLong_Check(pQoS))
					{
						byte bQoS = (byte)PyLong_AsLong(pQoS);
						bControlFlags |= (bQoS & 3) << 3; // Set QoS flag
					}

					PyBorrowedRef pRetain = PyDict_GetItemString(WriteMessage->m_Object, "WillRetain");
					if (pRetain && PyLong_Check(pRetain))
					{
						byte bRetain = (byte)PyLong_AsLong(pRetain);
						bControlFlags |= (bRetain & 1) << 5; // Set retain flag
					}

					std::string sPayload;
					PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "WillPayload");
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
				std::string		User;
				std::string		Pass;
				PyObject* pModule = (PyObject*)WriteMessage->m_pPlugin->PythonModule();
				PyNewRef	pDict = PyObject_GetAttrString(pModule, "Parameters");
				if (pDict)
				{
					PyBorrowedRef	pUser = PyDict_GetItemString(pDict, "Username");
					if (pUser) User = PyUnicode_AsUTF8(pUser);
					PyBorrowedRef	pPass = PyDict_GetItemString(pDict, "Password");
					if (pPass) Pass = PyUnicode_AsUTF8(pPass);
				}
				if (User.length())
				{
					MQTTPushBackStringWLen(User, vPayload);
					bControlFlags |= 128;
				}

				if (Pass.length())
				{
					MQTTPushBackStringWLen(Pass, vPayload);
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
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID && PyLong_Check(pID))
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is list of topics and QoS numbers
				PyBorrowedRef pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList || !PyList_Check(pTopicList))
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyObject* pTopicDict = PyList_GetItem(pTopicList, i);
					if (!pTopicDict || !PyDict_Check(pTopicDict))
					{
						_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: Topics list entry is not a dictionary (Topic, QoS), nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
						return retVal;
					}
					PyBorrowedRef pTopic = PyDict_GetItemString(pTopicDict, "Topic");
					if (pTopic && PyUnicode_Check(pTopic))
					{
						MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
						PyBorrowedRef pQoS = PyDict_GetItemString(pTopicDict, "QoS");
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
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID && PyLong_Check(pID))
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is a Python list of topics
				PyBorrowedRef pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList || !PyList_Check(pTopicList))
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to unsubscribe from. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyObject* pTopic = PyList_GetItem(pTopicList, i);
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
				PyBorrowedRef pDUP = PyDict_GetItemString(WriteMessage->m_Object, "Duplicate");
				if (pDUP && PyLong_Check(pDUP))
				{
					long	bDUP = PyLong_AsLong(pDUP);
					if (bDUP) bByte0 |= 0x08; // Set duplicate flag
				}

				PyBorrowedRef pQoS = PyDict_GetItemString(WriteMessage->m_Object, "QoS");
				long	iQoS = 0;
				if (pQoS && PyLong_Check(pQoS))
				{
					iQoS = PyLong_AsLong(pQoS);
					bByte0 |= ((iQoS & 3) << 1); // Set QoS flag
				}

				PyBorrowedRef pRetain = PyDict_GetItemString(WriteMessage->m_Object, "Retain");
				if (pRetain && PyLong_Check(pRetain))
				{
					long	bRetain = PyLong_AsLong(pRetain);
					bByte0 |= (bRetain & 1); // Set retain flag
				}

				// Variable Header
				PyBorrowedRef pTopic = PyDict_GetItemString(WriteMessage->m_Object, "Topic");
				if (pTopic && PyUnicode_Check(pTopic))
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vVariableHeader);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: No 'Topic' specified, nothing to publish. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
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
				std::string sPayload;
				PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "Payload");
				// Support both string and bytes
				//if (pPayload && PyByteArray_Check(pPayload)) // Gives linker error, why?
				if (pPayload) {
					_log.Debug(DEBUG_NORM, "(%s) MQTT Publish: payload %p (%s)", __func__, (PyObject*)pPayload, pPayload->ob_type->tp_name);
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
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
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

	/*

	See: https://tools.ietf.org/html/rfc6455#section-5.2

	  0                   1                   2                   3
	  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 +-+-+-+-+-------+-+-------------+-------------------------------+
	 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
	 | |1|2|3|       |K|             |                               |
	 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	 |     Extended payload length continued, if payload len == 127  |
	 + - - - - - - - - - - - - - - - +-------------------------------+
	 |                               |Masking-key, if MASK set to 1  |
	 +-------------------------------+-------------------------------+
	 | Masking-key (continued)       |          Payload Data         |
	 +-------------------------------- - - - - - - - - - - - - - - - +
	 :                     Payload Data continued ...                :
	 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	 |                     Payload Data continued ...                |
	 +---------------------------------------------------------------+

	*/

	bool CPluginProtocolWS::ProcessWholeMessage(std::vector<byte>& vMessage, const ReadEvent* Message)
	{
		while (!vMessage.empty())
		{
			// Look for a complete message
			std::vector<byte>	vPayload;
			size_t		iOffset = 0;
			int			iOpCode = 0;
			long		lMaskingKey = 0;
			bool		bFinish = false;

			bFinish = (vMessage[iOffset] & 0x80);				// Indicates that this is the final fragment in a message if true
			if (vMessage[iOffset] & 0x0F)
			{
				iOpCode = (vMessage[iOffset] & 0x0F);			// %x0 denotes a continuation frame
			}
			// %x1 denotes a text frame
			// %x2 denotes a binary frame
			// %x8 denotes a connection close
			// %x9 denotes a ping
			// %xA denotes a pong
			iOffset++;
			bool	bMasked = (vMessage[iOffset] & 0x80);			// Is the payload masked?
			long	lPayloadLength = (vMessage[iOffset] & 0x7F);	// if < 126 then this is the length
			if (lPayloadLength == 126)
			{
				if (vMessage.size() < (iOffset + 2))
					return false;
				lPayloadLength = (vMessage[iOffset + 1] << 8) + vMessage[iOffset + 2];
				iOffset += 2;
			}
			else if (lPayloadLength == 127)							// 64 bit lengths not supported
			{
				_log.Log(LOG_ERROR, "(%s) 64 bit WebSocket messages lengths not supported.", __func__);
				vMessage.clear();
				iOffset += 5;
				return false;
			}
			iOffset++;

			byte *pbMask = nullptr;
			if (bMasked)
			{
				if (vMessage.size() < iOffset)
					return false;
				lMaskingKey = (long)vMessage[iOffset];
				pbMask = &vMessage[iOffset];
				iOffset += 4;
			}

			if (vMessage.size() < (iOffset + lPayloadLength))
				return false;

			// Append the payload to the existing (maybe) payload
			if (lPayloadLength)
			{
				vPayload.reserve(vPayload.size() + lPayloadLength);
				for (size_t i = iOffset; i < iOffset + lPayloadLength; i++)
				{
					vPayload.push_back(vMessage[i]);
				}
				iOffset += lPayloadLength;
			}

			PyObject* pDataDict = (PyObject*)PyDict_New();
			PyNewRef pPayload = nullptr;

			// Handle full message
			PyNewRef pObj = Py_BuildValue("N", PyBool_FromLong(bFinish));
			if (PyDict_SetItemString(pDataDict, "Finish", pObj) == -1)
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Finish", bFinish ? "True" : "False");

			// Masked data?
			if (lMaskingKey)
			{
				// Unmask data
				for (int i = 0; i < lPayloadLength; i++)
				{
					vPayload[i] ^= pbMask[i % 4];
				}
				PyNewRef pObj = Py_BuildValue("i", lMaskingKey);
				if (PyDict_SetItemString(pDataDict, "Mask", pObj) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%ld' to dictionary.", __func__, "Mask", lMaskingKey);
			}

			switch (iOpCode)
			{
			case 0x01:	// Text message
			{
				std::string		sPayload(vPayload.begin(), vPayload.end());
				pPayload = Py_BuildValue("s", sPayload.c_str());
				break;
			}
			case 0x02:	// Binary message
				break;
			case 0x08:	// Connection Close
			{
				PyNewRef pObj = Py_BuildValue("s", "Close");
				if (PyDict_SetItemString(pDataDict, "Operation", pObj) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Operation", "Close");
				if (vPayload.size() == 2)
				{
					int		iReasonCode = (vPayload[0] << 8) + vPayload[1];
					pPayload = Py_BuildValue("i", iReasonCode);
				}
				break;
			}
			case 0x09:	// Ping
			{
				pDataDict = (PyObject*)PyDict_New();
				PyNewRef pObj = Py_BuildValue("s", "Ping");
				if (PyDict_SetItemString(pDataDict, "Operation", pObj) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Operation", "Ping");
				break;
			}
			case 0x0A:	// Pong
			{
				pDataDict = (PyObject*)PyDict_New();
				PyNewRef pObj = Py_BuildValue("s", "Pong");
				if (PyDict_SetItemString(pDataDict, "Operation", pObj) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Operation", "Pong");
				break;
			}
			default:
				_log.Log(LOG_ERROR, "(%s) Unknown Operation Code (%d) encountered.", __func__, iOpCode);
			}

			// If there is a payload but not handled then map it as binary
			if (!vPayload.empty() && !pPayload)
			{
				pPayload = Py_BuildValue("y#", &vPayload[0], vPayload.size());
			}

			// If there is a payload then add it
			if (pPayload)
			{
				if (PyDict_SetItemString(pDataDict, "Payload", pPayload) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", __func__, "Payload");
			}

			Message->m_pPlugin->MessagePlugin(new onMessageCallback(Message->m_pPlugin, Message->m_pConnection, pDataDict));

			// Remove the processed message from retained data
			vMessage.erase(vMessage.begin(), vMessage.begin() + iOffset);

			return true;
		}

		return false;
	}

	void CPluginProtocolWS::ProcessInbound(const ReadEvent* Message)
	{
		//	Although messages can be fragmented, control messages can be inserted in between fragments
		//	so try to process just the message first, then retained data and the message
		std::vector<byte>	Buffer = Message->m_Buffer;
		if (ProcessWholeMessage(Buffer, Message))
		{
			return;		// Message processed
		}

		// Add new message to retained data, process all messages if this one is the finish of a message
		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		// Always process the whole buffer because we can't know if we have whole, multiple or even complete messages unless we work through from the start
		if (ProcessWholeMessage(m_sRetainedData, Message))
		{
			return;		// Message processed
		}

	}

	std::vector<byte> CPluginProtocolWS::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;

		//
		//	Parameters need to be in a dictionary.
		//	if a 'URL' key is found message is assumed to be HTTP otherwise WebSocket is assumed
		//
		if (!WriteMessage->m_Object || !PyDict_Check(WriteMessage->m_Object))
		{
			_log.Log(LOG_ERROR, "(%s) Dictionary parameter expected.", __func__);
		}
		else
		{
			PyBorrowedRef pURL = PyDict_GetItemString(WriteMessage->m_Object, "URL");
			if (pURL)
			{
				// Is a verb specified?
				PyBorrowedRef pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
				if (!pVerb)
				{
					PyNewRef pObj = Py_BuildValue("s", "GET");
					if (PyDict_SetItemString(WriteMessage->m_Object, "Verb", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Verb", "GET");
				}

				// Required headers specified?
				PyBorrowedRef pHeaders = PyDict_GetItemString(WriteMessage->m_Object, "Headers");
				if (!pHeaders)
				{
					pHeaders = (PyObject*)PyDict_New();
					if (PyDict_SetItemString(WriteMessage->m_Object, "Headers", (PyObject*)pHeaders) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", "WS", "Headers");
					Py_DECREF(pHeaders);
				}
				PyBorrowedRef pConnection = PyDict_GetItemString(pHeaders, "Connection");
				if (!pConnection)
				{
					PyNewRef pObj = Py_BuildValue("s", "keep-alive, Upgrade");
					if (PyDict_SetItemString(pHeaders, "Connection", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Connection", "Upgrade");
				}
				PyBorrowedRef pUpgrade = PyDict_GetItemString(pHeaders, "Upgrade");
				if (!pUpgrade)
				{
					PyNewRef pObj = Py_BuildValue("s", "websocket");
					if (PyDict_SetItemString(pHeaders, "Upgrade", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "Upgrade", "websocket");
				}
				PyBorrowedRef pUserAgent = PyDict_GetItemString(pHeaders, "User-Agent");
				if (!pUserAgent)
				{
					PyNewRef pObj = Py_BuildValue("s", "Domoticz/1.0");
					if (PyDict_SetItemString(pHeaders, "User-Agent", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, "User-Agent", "Domoticz/1.0");
				}

				// Use parent HTTP protocol object to do the actual formatting
				return CPluginProtocolHTTP::ProcessOutbound(WriteMessage);
			}
			int iOpCode = 0;
			long lMaskingKey = 0;
			long lPayloadLength = 0;
			byte bMaskBit = 0x00;

			PyBorrowedRef pOperation = PyDict_GetItemString(WriteMessage->m_Object, "Operation");
			PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "Payload");
			PyBorrowedRef pMask = PyDict_GetItemString(WriteMessage->m_Object, "Mask");

			if (pOperation)
			{
				if (!PyUnicode_Check(pOperation))
				{
					_log.Log(LOG_ERROR, "(%s) Expected dictionary 'Operation' key to have a string value.", __func__);
					return retVal;
				}

				std::string sOperation = PyUnicode_AsUTF8(pOperation);
				if (sOperation == "Ping")
				{
					iOpCode = 0x09;
				}
				else if (sOperation == "Pong")
				{
					iOpCode = 0x0A;
				}
				else if (sOperation == "Close")
				{
					iOpCode = 0x08;
				}
			}

			// If there is no specific OpCode then set it from the payload datatype
			if (pPayload)
			{
				if (PyUnicode_Check(pPayload))
				{
					lPayloadLength = PyUnicode_GetLength(pPayload);
					if (!iOpCode)
						iOpCode = 0x01; // Text message
				}
				else if (PyBytes_Check(pPayload))
				{
					lPayloadLength = PyBytes_Size(pPayload);
					if (!iOpCode)
						iOpCode = 0x02; // Binary message
				}
				else if (pPayload->ob_type->tp_name == std::string("bytearray"))
				{
					lPayloadLength = PyByteArray_Size(pPayload);
					if (!iOpCode)
						iOpCode = 0x02; // Binary message
				}
			}

			if (pMask)
			{
				if (PyLong_Check(pMask))
				{
					lMaskingKey = PyLong_AsLong(pMask);
					bMaskBit = 0x80; // Set mask bit in header
				}
				else if (PyUnicode_Check(pMask))
				{
					std::string sMask = PyUnicode_AsUTF8(pMask);
					lMaskingKey = atoi(sMask.c_str());
					bMaskBit = 0x80; // Set mask bit in header
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Invalid mask, expected number (integer or string).", __func__);
					return retVal;
				}
			}

			// Assemble the actual message
			retVal.reserve(lPayloadLength + 16); // Masking relies on vector not reallocating during message assembly
			retVal.push_back(0x80 | iOpCode);
			if (lPayloadLength < 126)
			{
				retVal.push_back((bMaskBit | lPayloadLength) & 0xFF); // Short length
			}
			else
			{
				retVal.push_back(bMaskBit | 126);
				retVal.push_back(lPayloadLength >> 24);
				retVal.push_back((lPayloadLength >> 16) & 0xFF);
				retVal.push_back((lPayloadLength >> 8) & 0xFF);
				retVal.push_back(lPayloadLength & 0xFF); // Longer length
			}

			byte *pbMask = nullptr;
			if (bMaskBit)
			{
				retVal.push_back(lMaskingKey >> 24);
				pbMask = &retVal[retVal.size() - 1];
				retVal.push_back((lMaskingKey >> 16) & 0xFF);
				retVal.push_back((lMaskingKey >> 8) & 0xFF);
				retVal.push_back(lMaskingKey & 0xFF); // Encode mask
			}

			if (pPayload)
			{
				if (PyUnicode_Check(pPayload))
				{
					std::string sPayload = PyUnicode_AsUTF8(pPayload);
					for (int i = 0; i < lPayloadLength; i++)
					{
						retVal.push_back(sPayload[i] ^ pbMask[i % 4]);
					}
				}
				else if (PyBytes_Check(pPayload))
				{
					byte *pByte = (byte *)PyBytes_AsString(pPayload);
					for (int i = 0; i < lPayloadLength; i++)
					{
						retVal.push_back(pByte[i] ^ pbMask[i % 4]);
					}
				}
				else if (pPayload->ob_type->tp_name == std::string("bytearray"))
				{
					byte *pByte = (byte *)PyByteArray_AsString(pPayload);
					for (int i = 0; i < lPayloadLength; i++)
					{
						retVal.push_back(pByte[i] ^ pbMask[i % 4]);
					}
				}
			}
		}

		return retVal;
	}
} // namespace Plugins
#endif
