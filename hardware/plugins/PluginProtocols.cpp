#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "../../main/Helper.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "../../main/Helper.h"
#include "../../main/json_helper.h"
#include "../../main/Logger.h"
#include "../../webserver/Base64.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace Plugins {

	CPluginProtocol* CPluginProtocol::Create(const std::string& sProtocol)
	{
		if (sProtocol == "Line") return (CPluginProtocol*) new CPluginProtocolLine();
		if (sProtocol == "XML")
			return (CPluginProtocol*)new CPluginProtocolXML();
		if (sProtocol == "JSON")
			return (CPluginProtocol*)new CPluginProtocolJSON();
		if ((sProtocol == "HTTP") || (sProtocol == "HTTPS"))
		{
			CPluginProtocolHTTP* pProtocol = new CPluginProtocolHTTP(sProtocol == "HTTPS");
			return (CPluginProtocol*)pProtocol;
		}
		if (sProtocol == "ICMP")
			return (CPluginProtocol*)new CPluginProtocolICMP();
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
		Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, Message->m_Buffer));
	}

	std::vector<byte> CPluginProtocol::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;
		PyBorrowedRef	pObject(WriteMessage->m_Object);

		// Handle Bytes objects
		if (pObject.IsBytes())
		{
			const char* pData = PyBytes_AsString(pObject);
			size_t		iSize = PyBytes_Size(pObject);
			retVal.reserve(iSize);
			retVal.assign(pData, pData + iSize);
		}
		// Handle ByteArray objects
		else if (pObject.IsByteArray())
		{
			size_t	len = PyByteArray_Size(pObject);
			char* data = PyByteArray_AsString(pObject);
			retVal.reserve(len);
			retVal.assign((const byte*)data, (const byte*)data + len);
		}
		// Try forcing a String conversion
		else
		{
			PyNewRef	pStr = PyObject_Str(pObject);
			if (pStr)
			{
				std::string	sData = PyUnicode_AsUTF8(pStr);
				retVal.reserve((size_t)sData.length());
				retVal.assign((const byte*)sData.c_str(), (const byte*)sData.c_str() + sData.length());
			}
			else
				_log.Log(LOG_ERROR, "(%s) Unable to convert data (%s) to string representation, ignored.", __func__, pObject.Type().c_str());
		}

		return retVal;
	}

	void CPluginProtocol::Flush(CPlugin* pPlugin, CConnection* pConnection)
	{
		if (!m_sRetainedData.empty())
		{
			// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
			pPlugin->MessagePlugin(new onMessageCallback(pConnection, m_sRetainedData));
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

		std::string	sData(vData.begin(), vData.end());
		size_t iPos = sData.find_first_of('\r');		//  Look for message terminator
		while (iPos != std::string::npos)
		{
			Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, std::vector<byte>(&sData[0], &sData[iPos])));

			if (sData[iPos + 1] == '\n') iPos++;				//  Handle \r\n
			sData = sData.substr(iPos + 1);
			iPos = sData.find_first_of('\r');
		}

		m_sRetainedData.assign(sData.c_str(), sData.c_str() + sData.length()); // retain any residual for next time
	}

	static void AddBytesToDict(PyObject* pDict, const char* key, const std::string& value)
	{
		//PyNewRef pObj = Py_BuildValue("y#", value.c_str(), value.length());
		PyNewRef pObj((byte*)value.c_str(), value.length());
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	}

	static void AddStringToDict(PyObject* pDict, const char* key, const std::string& value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value.c_str());
	}

	static void AddIntToDict(PyObject* pDict, const char* key, const int value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	}

	static void AddLongToDict(PyObject* pDict, const char* key, const long value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%ld' to dictionary.", __func__, key, value);
	}

	static void AddUIntToDict(PyObject* pDict, const char* key, const unsigned int value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%d' to dictionary.", __func__, key, value);
	}

	static void AddDoubleToDict(PyObject* pDict, const char* key, const double value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%f' to dictionary.", __func__, key, value);
	}

	static void AddBoolToDict(PyObject* pDict, const char* key, const bool value)
	{
		PyNewRef pObj(value);
		if (!pObj || PyDict_SetItemString(pDict, key, (PyObject*)pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", __func__, key, value ? "True" : "False");
	}

	PyObject* CPluginProtocolJSON::JSONtoPython(Json::Value* pJSON)
	{
		PyObject* pRetVal = nullptr;

		if (pJSON->isArray())
		{
			pRetVal = PyList_New(pJSON->size());
			Py_ssize_t	Index = 0;
			for (auto& pRef : *pJSON)
			{
				// PyList_SetItem 'steals' a reference so use PyBorrowedRef instead of PyNewRef
				if (pRef.isArray() || pRef.isObject())
				{
					PyBorrowedRef pObj = JSONtoPython(&pRef);
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1))
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for object.", __func__, Index - 1);
				}
				else if (pRef.isUInt())
				{
					PyBorrowedRef pObj = Py_BuildValue("I", pRef.asUInt());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1))  // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for unsigned integer.", __func__, Index - 1);
				}
				else if (pRef.isInt())
				{
					PyBorrowedRef pObj = Py_BuildValue("i", pRef.asInt());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1)) // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for integer.", __func__, Index - 1);
				}
				else if (pRef.isDouble())
				{
					PyBorrowedRef pObj = Py_BuildValue("d", pRef.asDouble());
					if (!pObj || (PyList_SetItem(pRetVal, Index++, pObj) == -1)) // steals the ref to pObj
						_log.Log(LOG_ERROR, "(%s) failed to add item '%zd', to list for double.", __func__, Index - 1);
				}
				else if (pRef.isConvertibleTo(Json::stringValue))
				{
					std::string sString = pRef.asString();
					PyBorrowedRef pObj = Py_BuildValue("s#", sString.c_str(), sString.length());
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
					PyNewRef	pObj = JSONtoPython(&pRef);  // PyDict_SetItemString will add its own reference
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

	PyObject* CPluginProtocolJSON::JSONtoPython(const std::string& sData)
	{
		Json::Value		root;
		PyObject* pRetVal = nullptr;

		bool bRet = ParseJSon(sData, root);
		if ((!bRet) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "(%s) Parse Error on '%s'", __func__, sData.c_str());
			Py_RETURN_NONE;
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
		PyBorrowedRef	pObj(pObject);

		if (pObj.IsDict())
		{
			sJson += "{ ";
			PyObject* key, * value;
			Py_ssize_t pos = 0;
			while (PyDict_Next(pObj, &pos, &key, &value))
			{
				sJson += PythontoJSON(key) + ':' + PythontoJSON(value) + ',';
			}
			sJson[sJson.length() - 1] = '}';
		}
		else if (pObj.IsList())
		{
			sJson += "[ ";
			for (Py_ssize_t i = 0; i < PyList_Size(pObj); i++)
			{
				sJson += PythontoJSON(PyList_GetItem(pObj, i)) + ',';
			}
			sJson[sJson.length() - 1] = ']';
		}
		else if (pObj.IsTuple())
		{
			sJson += "[ ";
			for (Py_ssize_t i = 0; i < PyTuple_Size(pObj); i++)
			{
				sJson += PythontoJSON(PyTuple_GetItem(pObj, i)) + ',';
			}
			sJson[sJson.length() - 1] = ']';
		}
		else if (pObj.IsBool())
		{
			sJson += (PyObject_IsTrue(pObj) ? "true" : "false");
		}
		else if (pObj.IsLong())
		{
			sJson += std::to_string(PyLong_AsLong(pObj));
		}
		else if (pObj.IsFloat())
		{
			sJson += std::to_string(PyFloat_AsDouble(pObj));
		}
		else if (pObj.IsBytes())
		{
			sJson += '"' + std::string(PyBytes_AsString(pObj)) + '"';
		}
		else if (pObj.IsByteArray())
		{
			sJson += '"' + std::string(PyByteArray_AsString(pObj)) + '"';
		}
		else
		{
			// Try forcing a String conversion
			PyNewRef	pStr = PyObject_Str(pObject);
			if (pStr)
			{
				sJson += '"' + std::string(PyUnicode_AsUTF8(pStr)) + '"';
			}
			else
				_log.Log(LOG_ERROR, "(%s) Unable to convert data type (%s) to string representation, ignored.", __func__, pObj.Type().c_str());
		}

		return sJson;
	}

	void CPluginProtocolJSON::ProcessInbound(const ReadEvent* Message)
	{
		CConnection* pConnection = Message->m_pConnection;
		CPlugin* pPlugin = pConnection->pPlugin;

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
						pPlugin->Log(LOG_ERROR, "(%s) Parse Error on '%s'", __func__, sData.c_str());
						pPlugin->MessagePlugin(new onMessageCallback(pConnection, sData));
					}
					else
					{
						PyObject* pMessage = JSONtoPython(&root);
						pPlugin->MessagePlugin(new onMessageCallback(pConnection, pMessage));
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
					pPlugin->Log(LOG_ERROR, "(%s) Parse Error on '%s'", __func__, sData.c_str());
					pPlugin->MessagePlugin(new onMessageCallback(pConnection, sMessage));
				}
				else
				{
					PyObject* pMessage = JSONtoPython(&root);
					pPlugin->MessagePlugin(new onMessageCallback(pConnection, pMessage));
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
					Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, sData.substr(0, iEnd)));

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
			PyNewRef		pObj(sHeaderText);
			PyBorrowedRef	pPrevObj = PyDict_GetItemString((PyObject*)m_Headers, sHeaderName.c_str());
			// Encode multi headers in a list
			if (pPrevObj)
			{
				PyObject* pListObj = pPrevObj;
				// First duplicate? Create a list and add previous value
				if (!pPrevObj.IsList())
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

	void CPluginProtocolHTTP::Flush(CPlugin* pPlugin, CConnection* pConnection)
	{
		if (!m_sRetainedData.empty())
		{
			// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
			ProcessInbound(new ReadEvent(pConnection, 0, nullptr));
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

		m_ContentLength = 0;
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
						PyNewRef pObj(m_Status);
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
							PyNewRef pObj((const byte*)sData.c_str(), sData.length());
							if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
								_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sData.c_str());
						}

						Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, pDataDict));
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
								PyNewRef pObj(m_Status);
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
									PyNewRef pObj = PyBytes_FromStringAndSize(sPayload.c_str(), sPayload.length());
									if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
										_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
								}

								Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, pDataDict));
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
					PyNewRef pObj(sVerb);
					if (PyDict_SetItemString(DataDict, "Verb", pObj) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Verb", sVerb.c_str());

					// Beware - the request may be malformed; so make sure there is more data to process before trying to parse it out
					std::string sURL;
					if (sFirstLine.length() > sVerb.length())
					{
						sURL = sFirstLine.substr(sVerb.length() + 1, sFirstLine.find_first_of(' ', sVerb.length() + 1));
					}
					else
					{
						_log.Log(LOG_ERROR, "malformed request response received (verb: %s/%s)", sVerb.c_str(), sFirstLine.c_str());
					}

					PyNewRef pURL(sURL);
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
						PyNewRef pObj = PyBytes_FromStringAndSize(sPayload.c_str(), sPayload.length());
						if (PyDict_SetItemString(DataDict, "Data", pObj) == -1)
							_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "HTTP", "Data", sPayload.c_str());
					}

					Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, DataDict));
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
		if (PyBorrowedRef(WriteMessage->m_Object).Type() != "dict")
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

			if (!pVerb.IsString())
			{
				_log.Log(LOG_ERROR, "(%s) HTTP 'Verb' dictionary entry not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}
			sHttp = PyUnicode_AsUTF8(pVerb);
			stdupper(sHttp);
			sHttp += " ";

			PyBorrowedRef	pURL = PyDict_GetItemString(WriteMessage->m_Object, "URL");
			std::string	sHttpURL = "/";
			if (pURL.IsString())
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
				PyObject* pModule = (PyObject*)WriteMessage->m_pConnection->pPlugin->PythonModule();
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

			if (!pStatus.IsString())
			{
				_log.Log(LOG_ERROR, "(%s) HTTP 'Status' dictionary entry was not a string, ignored. See Python Plugin wiki page for help.", __func__);
				return retVal;
			}

			sHttp = "HTTP/1.1 ";
			sHttp += PyUnicode_AsUTF8(pStatus);
			sHttp += "\r\n";

			// Add Date header if it is not supplied
			PyObject* pHead = nullptr;
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
				if (pHeaders.IsDict())
				{
					PyObject* key, * value;
					Py_ssize_t pos = 0;
					while (PyDict_Next(pHeaders, &pos, &key, &value))
					{
						std::string	sKey = PyUnicode_AsUTF8(key);
						PyBorrowedRef	pValue(value);
						if (pValue.IsString())
						{
							std::string	sValue = PyUnicode_AsUTF8(value);
							sHttp += sKey + ": " + sValue + "\r\n";
						}
						else if (pValue.IsBytes())
						{
							const char* pBytes = PyBytes_AsString(value);
							sHttp += sKey + ": " + pBytes + "\r\n";
						}
						else if (pValue.IsByteArray())
						{
							const char* pByteArray = PyByteArray_AsString(value);
							sHttp += sKey + ": " + pByteArray + "\r\n";
						}
						else if (pValue.IsList())
						{
							PyNewRef	iterator = PyObject_GetIter(value);
							PyObject* item;
							while (item = PyIter_Next(iterator))
							{
								PyBorrowedRef	pItem(item);
								if (pItem.IsString())
								{
									std::string	sValue = PyUnicode_AsUTF8(item);
									sHttp += sKey + ": " + sValue + "\r\n";
								}
								else if (pItem.IsBytes())
								{
									const char* pBytes = PyBytes_AsString(item);
									sHttp += sKey + ": " + pBytes + "\r\n";
								}
								else if (pItem.IsByteArray())
								{
									const char* pByteArray = PyByteArray_AsString(item);
									sHttp += sKey + ": " + pByteArray + "\r\n";
								}
								Py_DECREF(item);
							}
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
				if (pData.IsString())
					iLength = PyUnicode_GetLength(pData);
				else if (pData.IsByteArray())
					iLength = PyByteArray_Size(pData);
				else if (pData.IsBytes())
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
			if (pData.IsString())
				lChunkLength = PyUnicode_GetLength(pData);
			else if (pData.IsByteArray())
				lChunkLength = PyByteArray_Size(pData);
			else if (pData.IsBytes())
				lChunkLength = PyBytes_Size(pData);
			std::stringstream stream;
			stream << std::hex << lChunkLength;
			sHttp += std::string(stream.str());
			sHttp += "\r\n";
		}

		// Append data if supplied (for POST) or Response
		if (pData.IsString())
		{
			sHttp += PyUnicode_AsUTF8(pData);
			retVal.reserve(sHttp.length() + 2);
			retVal.assign(sHttp.c_str(), sHttp.c_str() + sHttp.length());
		}
		else if (pData.IsByteArray())
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
		else if (pData.IsBytes())
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
		PyNewRef pObj;
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
		pObj = PyNewRef((byte*)sData.c_str(), sData.length());
		if (PyDict_SetItemString(pDataDict, "Data", pObj) == -1)
			_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", "ICMP", "Data", sData.c_str());

		if (pDataDict)
		{
			Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, pDataDict));
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

	static void MQTTPushBackLong(long iLong, std::vector<byte>& vVector)
	{
		vVector.push_back(iLong & 0xFF000000 >> 24);
		vVector.push_back(iLong & 0xFF0000 >> 16);
		vVector.push_back(iLong & 0xFF00 >> 8);
		vVector.push_back(iLong & 0xFF);
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

	long CPluginProtocolMQTT::MQTTDecodeVariableByte(std::vector<byte>::iterator& pIt)
	{
		long iRetVal = 0;
		long multiplier = 1;
		byte encodedByte;
		do
		{
			// Make sure the end of the buffer has not been reached
			if (!std::distance(pIt, m_sRetainedData.end()))
			{
				_log.Debug(DEBUG_NORM, "(%s) Not enough data received to determine message length.", __func__);
				return -1;
			}

			encodedByte = *pIt++;
			iRetVal += (encodedByte & 127) * multiplier;
			multiplier *= 128;
			if (multiplier > 128 * 128 * 128)
			{
				_log.Log(LOG_ERROR, "(%s) MQTT: Malformed Variable Byte encoding in message.", __func__);
				return -1;
			}
		} while ((encodedByte & 128) != 0);
		return iRetVal;
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
			auto it = m_sRetainedData.begin();

			byte		header = *it++;
			byte		bResponseType = header & 0xF0;
			byte		flags = header & 0x0F;
			PyObject* pMqttDict = PyDict_New();
			PyObject* pObj = nullptr;
			uint16_t	iPacketIdentifier = 0;
			long iRemainingLength = MQTTDecodeVariableByte(it);

			if ((iRemainingLength < 0) || (iRemainingLength > std::distance(it, m_sRetainedData.end())))
			{
				// Full packet has not arrived, wait for more data
				_log.Debug(DEBUG_NORM, "(%s) Not enough data received (got %ld, expected %ld).", __func__, (long)std::distance(it, m_sRetainedData.end()), iRemainingLength);
				return;
			}

			auto pktend = it + iRemainingLength;

			switch (bResponseType)
			{
			case MQTT_CONNECT:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("CONNECT"));
				int iProtocol = (*it++ << 8) + *it++;
				if (iProtocol != 4)
				{
					AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: Invalid protocol length"));
					break;
				}
				const char* cProtocol = (const char*)&*it;
				std::string sProtocol(cProtocol, iProtocol);
				if (sProtocol != "MQTT")
				{
					AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: Invalid protocol name"));
					break;
				}
				it += iProtocol;
				AddIntToDict(pMqttDict, "Version", *it++);
				flags = *it++;
				AddBoolToDict(pMqttDict, "Username", (flags & 0x80));
				AddBoolToDict(pMqttDict, "Password", (flags & 0x40));
				AddBoolToDict(pMqttDict, "Retain", (flags & 0x20));
				AddIntToDict(pMqttDict, "QoS", (flags & 0x18) >> 3);
				AddBoolToDict(pMqttDict, "Will", (flags & 0x04));
				AddBoolToDict(pMqttDict, "Clean", (flags & 0x02));
				AddIntToDict(pMqttDict, "KeepAlive", ((*it++ << 8) + *it++));
				long iPropertiesLength = MQTTDecodeVariableByte(it);
				AddBoolToDict(pMqttDict, "Properties", iPropertiesLength);
				if (iPropertiesLength)
				{
					AddStringToDict(pMqttDict, "Warning", std::string("MQTT protocol: Properties specified in CONNECT ignored."));
					break;
				}
				// Payload processing - Client ID
				if (it == pktend)
				{
					AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No payload"));
					break;
				}
				int iClientIDLen = *it++;
				if (iClientIDLen)
				{
					const char* cClientID = (const char*)&*it;
					AddStringToDict(pMqttDict, "ClientID", std::string(cClientID, iClientIDLen));
					it += iClientIDLen;
				}
				// Conditional fields - Will Properties   TODO:  These are really handled properly at all
				if (flags & 0x04)
				{
					if (it == pktend)
					{
						AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No 'Will' details in payload"));
						break;
					}
					int iWillPropLen = *it++;
					if (iWillPropLen)
					{
						const char* cWill = (const char*)&*it;
						AddIntToDict(pMqttDict, "WillDelayInterval", atoi(std::string(cWill, iWillPropLen).c_str()));
					}
					else
					{
						AddIntToDict(pMqttDict, "WillDelayInterval", 0);
					}
					it += iWillPropLen;
				}
				// Conditional fields - Will Topic
				if (flags & 0x04)
				{
					if (it == pktend)
					{
						AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No 'Will Topic' details in payload"));
						break;
					}
					int iTopicLen = *it++; // Should be UTF-8 encoded with a 2 byte length but was not by the test client
					if (iTopicLen)
					{
						const char* cTopic = (const char*)&*it;
						AddIntToDict(pMqttDict, "WillTopic", atoi(std::string(cTopic, iTopicLen).c_str()));
					}
					it += iTopicLen;
				}
				// Conditional fields - Will Payload
				if (flags & 0x04)
				{
					if (it == pktend)
					{
						AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No 'Will Payload' details in payload"));
						break;
					}
					int iPayloadLen = (*it++ << 8) + *it++;
					if (iPayloadLen)
					{
						const char* cPayload = (const char*)&*it;
						AddStringToDict(pMqttDict, "WillPayload", std::string(cPayload, iPayloadLen));
					}
					it += iPayloadLen;
				}
				// Conditional fields - User Name
				if (flags & 0x80)
				{
					if (it == pktend)
					{
						AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No 'User Name' details in payload"));
						break;
					}
					int iUsernameLen = (*it++ << 8) + *it++;
					if (iUsernameLen)
					{
						const char* cUsername = (const char*)&*it;
						AddStringToDict(pMqttDict, "Username", std::string(cUsername, iUsernameLen));
					}
					it += iUsernameLen;
				}
				// Conditional fields - Password
				if (flags & 0x40)
				{
					if (it == pktend)
					{
						AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No 'Password' details in payload"));
						break;
					}
					int iPasswordLen = (*it++ << 8) + *it++;
					if (iPasswordLen)
					{
						const char* cPassword = (const char*)&*it;
						AddStringToDict(pMqttDict, "Password", std::string(cPassword, iPasswordLen));
					}
					it += iPasswordLen;
				}
				break;
			}
			case MQTT_CONNACK:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("CONNACK"));
				if (flags != 0)
				{
					AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: Invalid protocol name"));
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
				AddBoolToDict(pMqttDict, "DUP", ((flags & 0x08) >> 3));
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
				std::string	sTopic((char const*)&*it, topicLen);
				AddStringToDict(pMqttDict, "Topic", sTopic);
				it += topicLen;
				if (iQoS)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				// Payload
				const char* pPayload = (it == pktend) ? nullptr : (const char*)&*it;
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
			case MQTT_SUBSCRIBE:
			{
				AddStringToDict(pMqttDict, "Verb", std::string("SUBSCRIBE"));
				if (flags != 2)
				{
					_log.Log(LOG_ERROR, "(%s) MQTT protocol violation: Invalid message flags %u for packet type '%u'", __func__, flags, bResponseType >> 4);
					m_bErrored = true;
					break;
				}
				if (iRemainingLength >= 2)
				{
					iPacketIdentifier = (*it++ << 8) + *it++;
					AddIntToDict(pMqttDict, "PacketIdentifier", iPacketIdentifier);
				}
				// Payload - a series of Topic, Subscription Option pairs
				if (it == pktend)
				{
					AddStringToDict(pMqttDict, "Error", std::string("MQTT protocol violation: No payload, at least one Topic must be specified"));
					m_bErrored = true;
					break;
				}
				PyNewRef pTopicList = PyList_New(0);
				if (PyDict_SetItemString(pMqttDict, "Topics", pTopicList) == -1)
				{
					_log.Log(LOG_ERROR, "(%s) failed to add key 'Topics' to dictionary.", __func__);
					return;
				}
				while (it != pktend)
				{
					PyNewRef pTopic = PyDict_New();
					int iTopicLen = (*it++ << 8) + *it++;
					AddStringToDict(pTopic, "Topic", std::string((const char*)&*it, iTopicLen).c_str());
					it += iTopicLen;
					AddIntToDict(pTopic, "QoS", *it++ & 0x03);
					PyList_Append(pTopicList, pTopic);
				}

				break;
			}
			case MQTT_UNSUBSCRIBE:
				break;
			case MQTT_PINGREQ:
				AddStringToDict(pMqttDict, "Verb", std::string("PINGREQ"));
				break;
			case MQTT_DISCONNECT:
				break;
			default:
				_log.Log(LOG_ERROR, "(%s) MQTT data invalid: packet type '%d' is unknown.", __func__, bResponseType);
				m_bErrored = true;
			}

			if (!m_bErrored) Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, pMqttDict));

			m_sRetainedData.erase(m_sRetainedData.begin(), pktend);
		} while (!m_bErrored && !m_sRetainedData.empty());

		if (m_bErrored)
		{
			_log.Log(LOG_ERROR, "(%s) MQTT protocol violation, sending DisconnectedEvent to Connection.", __func__);
			Message->m_pConnection->pPlugin->MessagePlugin(new DisconnectedEvent(Message->m_pConnection));
		}
	}

	std::vector<byte> CPluginProtocolMQTT::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	vVariableHeader;
		std::vector<byte>	vPayload;

		std::vector<byte>	retVal;

		// Sanity check input
		if (!PyBorrowedRef(WriteMessage->m_Object).IsDict())
		{
			_log.Log(LOG_ERROR, "(%s) MQTT Send parameter was not a dictionary, ignored. See Python Plugin wiki page for help.", __func__);
			return retVal;
		}

		// Extract potential values.  Failures return nullptr, success returns borrowed reference
		PyBorrowedRef pVerb = PyDict_GetItemString(WriteMessage->m_Object, "Verb");
		if (pVerb)
		{
			if (!pVerb.IsString())
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
				if (pID.IsString())
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pID)), vPayload);
				}
				else
					MQTTPushBackStringWLen("Domoticz", vPayload); // TODO: default ID should be more unique, for example "Domoticz_<plugin_name>_<HwID>"

				byte	bCleanSession = 1;
				PyBorrowedRef pCleanSession = PyDict_GetItemString(WriteMessage->m_Object, "CleanSession");
				if (pCleanSession.IsLong())
				{
					bCleanSession = (byte)PyLong_AsLong(pCleanSession);
				}
				bControlFlags |= (bCleanSession & 1) << 1;

				// Will topic
				PyBorrowedRef pTopic = PyDict_GetItemString(WriteMessage->m_Object, "WillTopic");
				if (pTopic.IsString())
				{
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
					bControlFlags |= 4;
				}

				// Will QoS, Retain and Message
				if (bControlFlags & 4)
				{
					PyBorrowedRef pQoS = PyDict_GetItemString(WriteMessage->m_Object, "WillQoS");
					if (pQoS.IsLong())
					{
						byte bQoS = (byte)PyLong_AsLong(pQoS);
						bControlFlags |= (bQoS & 3) << 3; // Set QoS flag
					}

					PyBorrowedRef pRetain = PyDict_GetItemString(WriteMessage->m_Object, "WillRetain");
					if (pRetain.IsLong())
					{
						byte bRetain = (byte)PyLong_AsLong(pRetain);
						bControlFlags |= (bRetain & 1) << 5; // Set retain flag
					}

					std::string sPayload;
					PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "WillPayload");
					// Support both string and bytes
					//if (pPayload && PyByteArray_Check(pPayload)) // Gives linker error, why?
					if (pPayload.IsByteArray())
					{
						sPayload = std::string(PyByteArray_AsString(pPayload), PyByteArray_Size(pPayload));
					}
					else if (pPayload.IsString())
					{
						sPayload = std::string(PyUnicode_AsUTF8(pPayload));
					}
					MQTTPushBackStringWLen(sPayload, vPayload);
				}

				// Username / Password
				std::string		User;
				std::string		Pass;
				PyObject* pModule = (PyObject*)WriteMessage->m_pConnection->pPlugin->PythonModule();
				PyNewRef	pDict = PyObject_GetAttrString(pModule, "Parameters");
				if (pDict.IsDict())
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
			else if (sVerb == "CONNACK")
			{
				// Connect acknowledge flags
				// Session Present flag
				PyBorrowedRef pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "SessionPresent");
				byte byteValue = 0;
				if (pDictEntry && PyObject_IsTrue(pDictEntry))
				{
					byteValue = 1;
				}
				vVariableHeader.push_back(byteValue);

				// Connect Reason Code
				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "ReasonCode");
				byteValue = 0;
				if (pDictEntry.IsLong())
				{
					byteValue = PyLong_AsLong(pDictEntry) & 0xFF;
				}
				vVariableHeader.push_back(byteValue);

				// CONNACK Properties
				std::vector<byte> vProperties;
				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "SessionExpiryInterval");
				if (pDictEntry.IsLong())
				{
					vProperties.push_back(17);
					MQTTPushBackLong(PyLong_AsLong(pDictEntry), vProperties);
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "MaximumQoS");
				if (pDictEntry.IsLong())
				{
					vProperties.push_back(36);
					vProperties.push_back((byte)PyLong_AsLong(pDictEntry));
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "RetainAvailable");
				if (pDictEntry.IsLong())
				{
					vProperties.push_back(37);
					vProperties.push_back((byte)PyLong_AsLong(pDictEntry));
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "MaximumPacketSize");
				if (pDictEntry.IsLong())
				{
					vProperties.push_back(39);
					MQTTPushBackLong(PyLong_AsLong(pDictEntry), vProperties);
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "AssignedClientID");
				if (pDictEntry && !pDictEntry.IsNone())
				{
					PyNewRef pStr = PyObject_Str(pDictEntry);
					vProperties.push_back(18);
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pStr)), vProperties);
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "ReasonString");
				if (pDictEntry && !pDictEntry.IsNone())
				{
					PyNewRef pStr = PyObject_Str(pDictEntry);
					vProperties.push_back(26);
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pStr)), vProperties);
				}

				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "ResponseInformation");
				if (pDictEntry && !pDictEntry.IsNone())
				{
					PyNewRef pStr = PyObject_Str(pDictEntry);
					vProperties.push_back(18);
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pStr)), vProperties);
				}

				vPayload.push_back((uint8_t)vProperties.size());
				vPayload.insert(vPayload.end(), vProperties.begin(), vProperties.end());

				retVal.push_back(MQTT_CONNACK);
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
				if (pID.IsLong())
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is list of topics and QoS numbers
				PyBorrowedRef pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList.IsList())
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyBorrowedRef pTopicDict = PyList_GetItem(pTopicList, i);
					if (!pTopicDict.IsDict())
					{
						_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: Topics list entry is not a dictionary (Topic, QoS), nothing to subscribe to. See Python Plugin wiki page for help.", __func__);
						return retVal;
					}
					PyBorrowedRef pTopic = PyDict_GetItemString(pTopicDict, "Topic");
					if (pTopic.IsString())
					{
						MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pTopic)), vPayload);
						PyBorrowedRef pQoS = PyDict_GetItemString(pTopicDict, "QoS");
						if (pQoS.IsLong())
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
			else if (sVerb == "SUBACK")
			{
				// Variable Header
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long iPacketIdentifier = 0;
				if (pID.IsLong())
				{
					iPacketIdentifier = PyLong_AsLong(pID);
					MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe Acknowledgement: No valid PacketIdentifier specified. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				PyBorrowedRef pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "QoS");
				if (pDictEntry.IsLong())
				{
					vPayload.push_back((byte)PyLong_AsLong(pDictEntry));
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe Acknowledgement: No valid QoS specified, 0 assumed. See Python Plugin wiki page for help.", __func__);
					vPayload.push_back(0);
				}

				retVal.push_back(MQTT_SUBACK);
			}
			else if (sVerb == "UNSUBSCRIBE")
			{
				// Variable Header
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID.IsLong())
				{
					iPacketIdentifier = PyLong_AsLong(pID);
				}
				else iPacketIdentifier = m_PacketID++;
				MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);

				// Payload is a Python list of topics
				PyBorrowedRef pTopicList = PyDict_GetItemString(WriteMessage->m_Object, "Topics");
				if (!pTopicList.IsList())
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Subscribe: No 'Topics' list present, nothing to unsubscribe from. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}
				for (Py_ssize_t i = 0; i < PyList_Size(pTopicList); i++)
				{
					PyBorrowedRef pTopic = PyList_GetItem(pTopicList, i);
					if (pTopic.IsString())
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
				if (pDUP.IsLong())
				{
					long	bDUP = PyLong_AsLong(pDUP);
					if (bDUP) bByte0 |= 0x08; // Set duplicate flag
				}

				PyBorrowedRef pQoS = PyDict_GetItemString(WriteMessage->m_Object, "QoS");
				long	iQoS = 0;
				if (pQoS.IsLong())
				{
					iQoS = PyLong_AsLong(pQoS);
					bByte0 |= ((iQoS & 3) << 1); // Set QoS flag
				}

				PyBorrowedRef pRetain = PyDict_GetItemString(WriteMessage->m_Object, "Retain");
				if (pRetain.IsLong())
				{
					long	bRetain = PyLong_AsLong(pRetain);
					bByte0 |= (bRetain & 1); // Set retain flag
				}

				// Variable Header
				PyBorrowedRef pTopic = PyDict_GetItemString(WriteMessage->m_Object, "Topic");
				if (pTopic && pTopic.IsString())
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
					if (pID.IsLong())
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
				PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "Payload");
				// Support both string and bytes
				//if (pPayload && PyByteArray_Check(pPayload)) // Gives linker error, why?
				if (pPayload)
				{
					PyNewRef	pName = PyObject_GetAttrString((PyObject*)pPayload->ob_type, "__name__");
					_log.Debug(DEBUG_NORM, "(%s) MQTT Publish: payload %p (%s)", __func__, (PyObject*)pPayload, ((std::string)pName).c_str());
				}
				if (pPayload.IsByteArray())
				{
					std::string sPayload = std::string(PyByteArray_AsString(pPayload), PyByteArray_Size(pPayload));
					MQTTPushBackString(sPayload, vPayload);
				}
				else if (pPayload.IsString())
				{
					std::string sPayload = std::string(PyUnicode_AsUTF8(pPayload));
					MQTTPushBackString(sPayload, vPayload);
				}
				else if (pPayload.IsLong())
				{
					MQTTPushBackLong(PyLong_AsLong(pPayload), vPayload);
				}
				retVal.push_back(bByte0);
			}
			else if (sVerb == "PUBREL")
			{
				// Variable Header
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long	iPacketIdentifier = 0;
				if (pID.IsLong())
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
			else if (sVerb == "PUBACK")
			{
				// Variable Header
				PyBorrowedRef pID = PyDict_GetItemString(WriteMessage->m_Object, "PacketIdentifier");
				long iPacketIdentifier = 0;
				if (pID.IsLong())
				{
					iPacketIdentifier = PyLong_AsLong(pID);
					MQTTPushBackNumber((int)iPacketIdentifier, vVariableHeader);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: No valid PacketIdentifier specified. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				// Connect Reason Code
				PyBorrowedRef	pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "ReasonCode");
				if (pDictEntry.IsLong())
				{
					vVariableHeader.push_back((byte)PyLong_AsLong(pDictEntry));
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) MQTT Publish: No valid ReasonCode specified. See Python Plugin wiki page for help.", __func__);
					return retVal;
				}

				std::vector<byte> vProperties;
				pDictEntry = PyDict_GetItemString(WriteMessage->m_Object, "ReasonString");
				if (pDictEntry)
				{
					PyNewRef pStr = PyObject_Str(pDictEntry);
					vProperties.push_back(31);
					MQTTPushBackStringWLen(std::string(PyUnicode_AsUTF8(pStr)), vProperties);
				}

				vPayload.push_back((uint8_t)vProperties.size());
				vPayload.insert(vPayload.end(), vProperties.begin(), vProperties.end());

				retVal.push_back(MQTT_PUBACK);
			}
			else if (sVerb == "PINGRESP")
			{
				retVal.push_back(MQTT_PINGRESP);
			}
			else if (sVerb == "DISCONNECT")
			{
				retVal.push_back(MQTT_DISCONNECT);
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
		if (vMessage.size() > 1)
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

			byte* pbMask = nullptr;
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
			PyNewRef pPayload;

			// Handle full message
			AddBoolToDict(pDataDict, "Finish", bFinish);

			// Masked data?
			if (lMaskingKey)
			{
				// Unmask data
				for (int i = 0; i < lPayloadLength; i++)
				{
					vPayload[i] ^= pbMask[i % 4];
				}

				AddLongToDict(pDataDict, "Mask", lMaskingKey);
			}

			switch (iOpCode)
			{
			case 0x01:	// Text message
			{
				// Force text messages to be returned as Unicode rather than Bytes
				pPayload = PyNewRef(std::string(vPayload.begin(), vPayload.end()));
				break;
			}
			case 0x02:	// Binary message
				break;
			case 0x08:	// Connection Close
			{
				AddStringToDict(pDataDict, "Operation", "Close");
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
				AddStringToDict(pDataDict, "Operation", "Ping");
				break;
			}
			case 0x0A:	// Pong
			{
				pDataDict = (PyObject*)PyDict_New();
				AddStringToDict(pDataDict, "Operation", "Pong");
				break;
			}
			default:
				_log.Log(LOG_ERROR, "(%s) Unknown Operation Code (%d) encountered.", __func__, iOpCode);
			}

			// If there is a payload but not handled then map it as binary
			if (!vPayload.empty() && !pPayload)
			{
				pPayload = PyNewRef(vPayload);
				if (!pPayload)
					_log.Log(LOG_ERROR, "(%s) failed build Python object for payload.", __func__);
			}

			// If there is a payload then add it
			if (pPayload)
			{
				if (PyDict_SetItemString(pDataDict, "Payload", pPayload) == -1)
					_log.Log(LOG_ERROR, "(%s) failed to add key '%s' to dictionary.", __func__, "Payload");
			}

			Message->m_pConnection->pPlugin->MessagePlugin(new onMessageCallback(Message->m_pConnection, pDataDict));

			// Remove the processed message from retained data
			vMessage.erase(vMessage.begin(), vMessage.begin() + iOffset);

			return true;
		}

		return false;
	}

	void CPluginProtocolWS::ProcessInbound(const ReadEvent* Message)
	{
		// Check this isn't an HTTP message (connection/protocol switch may have failed)
		if (Message->m_Buffer.size() >= 4)
		{
			std::string		sData(Message->m_Buffer.begin(), Message->m_Buffer.begin() + 4);
			if (sData == "HTTP")
			{
				CPluginProtocolHTTP::ProcessInbound(Message);
				return;
			}
		}

		// Add new message to retained data, process all messages if this one is the finish of a message
		m_sRetainedData.insert(m_sRetainedData.end(), Message->m_Buffer.begin(), Message->m_Buffer.end());

		// Although messages can be fragmented, control messages can be inserted in between fragments.
		// see https://datatracker.ietf.org/doc/html/rfc6455#section-5.4
		// Always process the whole buffer because we can't know if we have whole, multiple or even complete messages unless we work through from the start
		while (ProcessWholeMessage(m_sRetainedData, Message))
		{
			continue;		// Message processed
		}
	}

	std::vector<byte> CPluginProtocolWS::ProcessOutbound(const WriteDirective* WriteMessage)
	{
		std::vector<byte>	retVal;

		//
		//	Parameters need to be in a dictionary.
		//	if a 'URL' key is found message is assumed to be HTTP otherwise WebSocket is assumed
		//
		if (!PyBorrowedRef(WriteMessage->m_Object).IsDict())
		{
			_log.Log(LOG_ERROR, "(%s) Dictionary parameter expected.", __func__);
		}
		else
		{
			// If a URL is specified then this is an HTTP message (which must be the WebSocket upgrade message in a valid flow)
			PyBorrowedRef pURL = PyDict_GetItemString(WriteMessage->m_Object, "URL");
			if (pURL)
			{
				// Is a verb specified?
				if (!PyDict_GetItemString(WriteMessage->m_Object, "Verb"))
					AddStringToDict(WriteMessage->m_Object, "Verb", "GET");

				// Required headers specified?
				PyBorrowedRef pHeaders = PyDict_GetItemString(WriteMessage->m_Object, "Headers");
				if (!pHeaders)
				{
					pHeaders = (PyObject*)PyDict_New();
					if (PyDict_SetItemString(WriteMessage->m_Object, "Headers", (PyObject*)pHeaders) == -1)
						_log.Log(LOG_ERROR, "(%s) failed to create '%s' missing dictionary.", "WS", "Headers");
					Py_DECREF(pHeaders);
				}

				// Add default values for missing headers
				if (!PyDict_GetItemString(pHeaders, "Accept"))
					AddStringToDict(pHeaders, "Accept", "*/*");

				if (!PyDict_GetItemString(pHeaders, "Accept-Language"))
					AddStringToDict(pHeaders, "Accept-Language", "en-US,en;q=0.9'");

				if (!PyDict_GetItemString(pHeaders, "Accept-Encoding"))
					AddStringToDict(pHeaders, "Accept-Encoding", "gzip, deflate");

				if (!PyDict_GetItemString(pHeaders, "Connection"))
					AddStringToDict(pHeaders, "Connection", "keep-alive, Upgrade");

				if (!PyDict_GetItemString(pHeaders, "Sec-WebSocket-Version"))
					AddStringToDict(pHeaders, "Sec-WebSocket-Version", "13");

				if (!PyDict_GetItemString(pHeaders, "Sec-WebSocket-Extensions"))
					AddStringToDict(pHeaders, "Sec-WebSocket-Extensions", "permessage-deflate");

				if (!PyDict_GetItemString(pHeaders, "Upgrade"))
					AddStringToDict(pHeaders, "Upgrade", "websocket");

				if (!PyDict_GetItemString(pHeaders, "User-Agent"))
					AddStringToDict(pHeaders, "User-Agent", "Domoticz/1.0");

				if (!PyDict_GetItemString(pHeaders, "Pragma"))
					AddStringToDict(pHeaders, "Pragma", "no-cache");

				if (!PyDict_GetItemString(pHeaders, "Cache-Control"))
					AddStringToDict(pHeaders, "Cache-Control", "no-cache");

				// Use parent HTTP protocol object to do the actual formatting and queuing
				return CPluginProtocolHTTP::ProcessOutbound(WriteMessage);
			}
			int iOpCode = 0;
			int64_t llMaskingKey = 0;
			long lPayloadLength = 0;
			byte bMaskBit = 0x00;

			PyBorrowedRef pOperation = PyDict_GetItemString(WriteMessage->m_Object, "Operation");
			PyBorrowedRef pPayload = PyDict_GetItemString(WriteMessage->m_Object, "Payload");
			PyBorrowedRef pMask = PyDict_GetItemString(WriteMessage->m_Object, "Mask");

			if (pOperation)
			{
				if (!pOperation.IsString())
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
			if (pPayload.IsString())
			{
				lPayloadLength = PyUnicode_GetLength(pPayload);
				if (!iOpCode)
					iOpCode = 0x01; // Text message
			}
			else if (pPayload.IsBytes())
			{
				lPayloadLength = PyBytes_Size(pPayload);
				if (!iOpCode)
					iOpCode = 0x02; // Binary message
			}
			else if (pPayload.IsByteArray())
			{
				lPayloadLength = PyByteArray_Size(pPayload);
				if (!iOpCode)
					iOpCode = 0x02; // Binary message
			}

			if (pMask)
			{
				if (pMask.IsLong())
				{
					llMaskingKey = PyLong_AsLongLong(pMask);
					bMaskBit = 0x80; // Set mask bit in header
				}
				else if (pMask.IsString())
				{
					std::string sMask = PyUnicode_AsUTF8(pMask);
					llMaskingKey = atoi(sMask.c_str());
					bMaskBit = 0x80; // Set mask bit in header
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Invalid mask, expected number (integer or string) but got '%s'.", __func__, pMask.Type().c_str());
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

			byte* pbMask = nullptr;
			if (bMaskBit)
			{
				retVal.push_back((byte)(llMaskingKey >> 24));
				pbMask = &retVal[retVal.size() - 1];
				retVal.push_back((llMaskingKey >> 16) & 0xFF);
				retVal.push_back((llMaskingKey >> 8) & 0xFF);
				retVal.push_back(llMaskingKey & 0xFF); // Encode mask
			}

			if (pPayload.IsString())
			{
				std::string sPayload = PyUnicode_AsUTF8(pPayload);
				for (int i = 0; i < lPayloadLength; i++)
				{
					if (bMaskBit)
						retVal.push_back(sPayload[i] ^ pbMask[i % 4]);
					else
						retVal.push_back(sPayload[i]);
				}
			}
			else if (pPayload.IsBytes())
			{
				byte* pByte = (byte*)PyBytes_AsString(pPayload);
				for (int i = 0; i < lPayloadLength; i++)
				{
					if (bMaskBit)
						retVal.push_back(pByte[i] ^ pbMask[i % 4]);
					else
						retVal.push_back(pByte[i]);
				}
			}
			else if (pPayload.IsByteArray())
			{
				byte* pByte = (byte*)PyByteArray_AsString(pPayload);
				for (int i = 0; i < lPayloadLength; i++)
				{
					if (bMaskBit)
						retVal.push_back(pByte[i] ^ pbMask[i % 4]);
					else
						retVal.push_back(pByte[i]);
				}
			}
		}

		return retVal;
	}
} // namespace Plugins
#endif
