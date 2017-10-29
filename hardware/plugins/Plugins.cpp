#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include <tinyxml.h>

#include "Plugins.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"
#include "PythonObjects.h"

#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"

#include "../../notifications/NotificationHelper.h"

#define ADD_STRING_TO_DICT(pDict, key, value) \
		{	\
			PyObject*	pObj = Py_BuildValue("s", value.c_str());	\
			if (PyDict_SetItemString(pDict, key, pObj) == -1)	\
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", m_PluginKey.c_str(), key, value.c_str());	\
			Py_DECREF(pObj); \
		}

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

extern std::string szWWWFolder;

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessageBase*>	PluginMessageQueue;
	extern boost::asio::io_service ios;

	boost::mutex PythonMutex;		// only used during startup when multiple threads could use Python

	//
	//	Holds per plugin state details, specifically plugin object, read using PyModule_GetState(PyObject *module)
	//
	struct module_state {
		CPlugin*	pPlugin;
		PyObject*	error;
	};

	void LogPythonException(CPlugin* pPlugin, const std::string &sHandler)
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;
		const char*			pTypeText = NULL;
		std::string			Name = "Unknown";

		if (pPlugin) Name = pPlugin->Name;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			pTypeText = TypeName->tp_name;
		}
		if (pValue)
		{
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);
		}
		if (pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s':'%s'.", Name.c_str(), sHandler.c_str(), pTypeText, pErrBytes->ob_sval);
		}
		if (pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pTypeText);
		}
		if (!pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pErrBytes->ob_sval);
		}
		if (!pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed, unable to determine error.", Name.c_str(), sHandler.c_str());
		}
		if (pErrBytes) Py_XDECREF(pErrBytes);

		// Log a stack trace if there is one
		while (pTraceback)
		{
			PyFrameObject *frame = pTraceback->tb_frame;
			if (frame)
			{
				int lineno = PyFrame_GetLineNumber(frame);
				PyCodeObject*	pCode = frame->f_code;
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
				_log.Log(LOG_ERROR, "(%s) ----> Line %d in %s, function %s", Name.c_str(), lineno, pFileBytes->ob_sval, pFuncBytes->ob_sval);
				Py_XDECREF(pFileBytes);
				Py_XDECREF(pFuncBytes);
			}
			pTraceback = pTraceback->tb_next;
		}

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to message handler '%s' failed, unable to decode exception.", Name.c_str(), sHandler.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	static PyObject*	PyDomoticz_Debug(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debug, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debug, illegal operation, Plugin has not started yet.");
		}
		else
		{
			if (pModState->pPlugin->m_bDebug)
			{
				char* msg;
				if (!PyArg_ParseTuple(args, "s", &msg))
				{
					_log.Log(LOG_ERROR, "(%s) PyDomoticz_Debug failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
					LogPythonException(pModState->pPlugin, std::string(__func__));
				}
				else
				{
					std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
					_log.Log((_eLogLevel)LOG_NORM, message.c_str());
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Log(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Log, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Log, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char* msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Log failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_NORM, message.c_str());
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Error(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Error, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Error, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char* msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Error failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_ERROR, message.c_str());
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Debugging(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debugging, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debugging, illegal operation, Plugin has not started yet.");
		}
		else
		{
			int		type;
			if (!PyArg_ParseTuple(args, "i", &type))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				type ? pModState->pPlugin->m_bDebug = true : pModState->pPlugin->m_bDebug = false;
				_log.Log(LOG_NORM, "(%s) Debug log level set to: '%s'.", pModState->pPlugin->Name.c_str(), (type ? "true" : "false"));
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Heartbeat(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Heartbeat, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Heartbeat, illegal operation, Plugin has not started yet.");
		}
		else
		{
			int	iPollinterval;
			if (!PyArg_ParseTuple(args, "i", &iPollinterval))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				//	Add heartbeat command to message queue
				PollIntervalDirective*	Message = new PollIntervalDirective(pModState->pPlugin, iPollinterval);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Notifier(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", __func__);
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			char*	szNotifier;
			if (!PyArg_ParseTuple(args, "s", &szNotifier))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, Notifier Name expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string		sNotifierName = szNotifier;
				if ((!sNotifierName.length()) || (sNotifierName.find_first_of(' ') != -1))
				{
					_log.Log(LOG_ERROR, "(%s) failed to parse parameters, valid Notifier Name expected, received '%s'.", pModState->pPlugin->Name.c_str(), szNotifier);
				}
				else
				{
					//	Add notifier command to message queue
					NotifierDirective*	Message = new NotifierDirective(pModState->pPlugin, szNotifier);
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyMethodDef DomoticzMethods[] = {
		{ "Debug", PyDomoticz_Debug, METH_VARARGS, "Write message to Domoticz log only if verbose logging is turned on." },
		{ "Log", PyDomoticz_Log, METH_VARARGS, "Write message to Domoticz log." },
		{ "Error", PyDomoticz_Error, METH_VARARGS, "Write error message to Domoticz log." },
		{ "Debugging", PyDomoticz_Debugging, METH_VARARGS, "Set logging level. 1 set verbose logging, all other values use default level" },
		{ "Heartbeat", PyDomoticz_Heartbeat, METH_VARARGS, "Set the heartbeat interval, default 10 seconds." },
		{ "Notifier", PyDomoticz_Notifier, METH_VARARGS, "Enable notification handling with supplied name." },
		{ NULL, NULL, 0, NULL }
	};

	static int DomoticzTraverse(PyObject *m, visitproc visit, void *arg) {
		Py_VISIT(GETSTATE(m)->error);
		return 0;
	}

	static int DomoticzClear(PyObject *m) {
		Py_CLEAR(GETSTATE(m)->error);
		return 0;
	}

	struct PyModuleDef DomoticzModuleDef = {
		PyModuleDef_HEAD_INIT,
		"Domoticz",
		NULL,
		sizeof(struct module_state),
		DomoticzMethods,
		NULL,
		DomoticzTraverse,
		DomoticzClear,
		NULL
	};

	PyMODINIT_FUNC PyInit_Domoticz(void)
	{

		// This is called during the import of the plugin module
		// triggered by the "import Domoticz" statement
		PyObject* pModule = PyModule_Create2(&DomoticzModuleDef, PYTHON_API_VERSION);

		if (PyType_Ready(&CDeviceType) < 0)
		{
			_log.Log(LOG_ERROR, "%s, Device Type not ready.", __func__);
			return pModule;
		}
		Py_INCREF((PyObject *)&CDeviceType);
		PyModule_AddObject(pModule, "Device", (PyObject *)&CDeviceType);

		if (PyType_Ready(&CConnectionType) < 0)
		{
			_log.Log(LOG_ERROR, "%s, Connection Type not ready.", __func__);
			return pModule;
		}
		Py_INCREF((PyObject *)&CConnectionType);
		PyModule_AddObject(pModule, "Connection", (PyObject *)&CConnectionType);

		if (PyType_Ready(&CImageType) < 0)
		{
			_log.Log(LOG_ERROR, "%s, Image Type not ready.", __func__);
			return pModule;
		}
		Py_INCREF((PyObject *)&CImageType);
		PyModule_AddObject(pModule, "Image", (PyObject *)&CImageType);

		return pModule;
	}


	CPlugin::CPlugin(const int HwdID, const std::string &sName, const std::string &sPluginKey) :
		m_stoprequested(false),
		m_PluginKey(sPluginKey),
		m_iPollInterval(10),
		m_Notifier(NULL),
		m_bDebug(false),
		m_PyInterpreter(NULL),
		m_PyModule(NULL),
		m_DeviceDict(NULL),
		m_ImageDict(NULL),
		m_SettingsDict(NULL)
	{
		m_HwdID = HwdID;
		Name = sName;
		m_bIsStarted = false;
	}

	CPlugin::~CPlugin(void)
	{
		m_bIsStarted = false;
	}

	void CPlugin::LogPythonException()
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			_log.Log(LOG_ERROR, "(%s) Module Import failed, exception: '%s'", Name.c_str(), TypeName->tp_name);
		}
		if (pValue)
		{
			std::string			sError;
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);	// Won't normally return text for Import related errors
			if (!pErrBytes)
			{
				// ImportError has name and path attributes
				if (PyObject_HasAttrString(pValue, "path"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "path");
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
					if (pBytes)
					{
						sError += "Path: ";
						sError += pBytes->ob_sval;
						Py_XDECREF(pBytes);
					}
					Py_XDECREF(pString);
				}
				if (PyObject_HasAttrString(pValue, "name"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "name");
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
					if (pBytes)
					{
						sError += " Name: ";
						sError += pBytes->ob_sval;
						Py_XDECREF(pBytes);
					}
					Py_XDECREF(pString);
				}
				if (sError.length())
				{
					_log.Log(LOG_ERROR, "(%s) Module Import failed: '%s'", Name.c_str(), sError.c_str());
					sError = "";
				}

				// SyntaxError, IndentationError & TabError have filename, lineno, offset and text attributes
				if (PyObject_HasAttrString(pValue, "filename"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "filename");
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
					sError += "File: ";
					sError += pBytes->ob_sval;
					Py_XDECREF(pString);
					Py_XDECREF(pBytes);
				}
				long long	lineno = -1;
				long long 	offset = -1;
				if (PyObject_HasAttrString(pValue, "lineno"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "lineno");
					lineno = PyLong_AsLongLong(pString);
					Py_XDECREF(pString);
				}
				if (PyObject_HasAttrString(pExcept, "offset"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "offset");
					offset = PyLong_AsLongLong(pString);
					Py_XDECREF(pString);
				}

				if (sError.length())
				{
					if ((lineno > 0) && (lineno < 1000))
					{
						_log.Log(LOG_ERROR, "(%s) Import detail: %s, Line: %d, offset: %d", Name.c_str(), sError.c_str(), lineno, offset);
					}
					else
					{
						_log.Log(LOG_ERROR, "(%s) Import detail: %s, Line: %d", Name.c_str(), sError.c_str(), offset);
					}
					sError = "";
				}

				if (PyObject_HasAttrString(pExcept, "text"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "text");
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
					_log.Log(LOG_ERROR, "(%s) Error Line '%s'", Name.c_str(), pBytes->ob_sval);
					Py_XDECREF(pString);
					Py_XDECREF(pBytes);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Error Line details not available.", Name.c_str());
				}

				if (sError.length())
				{
					_log.Log(LOG_ERROR, "(%s) Import detail: %s", Name.c_str(), sError.c_str());
				}
			}
			else _log.Log(LOG_ERROR, "(%s) Module Import failed '%s'", Name.c_str(), pErrBytes->ob_sval);
		}

		if (pErrBytes) Py_XDECREF(pErrBytes);

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to import module '%s' failed, unable to decode exception.", Name.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	void CPlugin::LogPythonException(const std::string &sHandler)
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;
		const char*			pTypeText = NULL;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			pTypeText = TypeName->tp_name;
		}
		if (pValue)
		{
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);
		}
		if (pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s':'%s'.", Name.c_str(), sHandler.c_str(), pTypeText, pErrBytes->ob_sval);
		}
		if (pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pTypeText);
		}
		if (!pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pErrBytes->ob_sval);
		}
		if (!pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed, unable to determine error.", Name.c_str(), sHandler.c_str());
		}
		if (pErrBytes) Py_XDECREF(pErrBytes);

		// Log a stack trace if there is one
		while (pTraceback)
			{
			PyFrameObject *frame = pTraceback->tb_frame;
			if (frame)
			{
				int lineno = PyFrame_GetLineNumber(frame);
				PyCodeObject*	pCode = frame->f_code;
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
				_log.Log(LOG_ERROR, "(%s) ----> Line %d in %s, function %s", Name.c_str(), lineno, pFileBytes->ob_sval, pFuncBytes->ob_sval);
				Py_XDECREF(pFileBytes);
				Py_XDECREF(pFuncBytes);
			}
			pTraceback = pTraceback->tb_next;
		}

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to message handler '%s' failed, unable to decode exception.", Name.c_str(), sHandler.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	bool CPlugin::IoThreadRequired()
	{
		boost::lock_guard<boost::mutex> l(m_TransportsMutex);
		if (m_Transports.size())
		{
			for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
			{
				CPluginTransport*	pPluginTransport = *itt;
				if (pPluginTransport && (pPluginTransport->IsConnected()) && (pPluginTransport->ThreadPoolRequired()))
					return true;
			}
		}
		return false;
	}

	int CPlugin::PollInterval(int Interval)
	{
		if (Interval > 0)
			m_iPollInterval = Interval;
		if (m_bDebug) _log.Log(LOG_NORM, "(%s) Heartbeat interval set to: %d.", Name.c_str(), m_iPollInterval);
		return m_iPollInterval;
	}

	void CPlugin::Notifier(std::string Notifier)
	{
		if (m_Notifier)
			delete m_Notifier;
		if (m_bDebug) _log.Log(LOG_NORM, "(%s) Notifier Name set to: %s.", Name.c_str(), Notifier.c_str());
		m_Notifier = new CPluginNotifier(this, Notifier);
	}

	void CPlugin::AddConnection(CPluginTransport *pTransport)
	{
		boost::lock_guard<boost::mutex> l(m_TransportsMutex);
		m_Transports.push_back(pTransport);
	}

	void CPlugin::RemoveConnection(CPluginTransport *pTransport)
	{
		boost::lock_guard<boost::mutex> l(m_TransportsMutex);
		for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
		{
			CPluginTransport*	pPluginTransport = *itt;
			if (pTransport == pPluginTransport)
			{
				m_Transports.erase(itt);
				break;
			}
		}
	}


	bool CPlugin::StartHardware()
	{
		if (m_bIsStarted) StopHardware();

		//	Add start command to message queue
		InitializeMessage*	Message = new InitializeMessage(this);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		_log.Log(LOG_STATUS, "(%s) Started.", Name.c_str());

		return true;
	}

	bool CPlugin::StopHardware()
	{
		try
		{
			_log.Log(LOG_STATUS, "(%s) Stop directive received.", Name.c_str());

			m_stoprequested = true;
			if (m_bIsStarted)
			{
				// If we have connections queue disconnects
				if (m_Transports.size())
				{
					boost::lock_guard<boost::mutex> l(m_TransportsMutex);
					for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
					{
						CPluginTransport*	pPluginTransport = *itt;
						// Tell transport to disconnect if required
						if (pPluginTransport)
						{
							DisconnectDirective*	DisconnectMessage = new DisconnectDirective(this, pPluginTransport->Connection());
							boost::lock_guard<boost::mutex> l(PluginMutex);
							PluginMessageQueue.push(DisconnectMessage);
						}
					}
				}
				else
				{
					// otherwise just signal stop
					StopMessage*	Message = new StopMessage(this);
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(Message);
					}
				}
			}

			// loop on stop to be processed
			int scounter = 0;
			while (m_bIsStarted && (scounter++ < 50))
			{
				sleep_milliseconds(100);
			}

			_log.Log(LOG_STATUS, "(%s) Stopping threads.", Name.c_str());

			if (m_thread)
			{
				m_thread->join();
				m_thread.reset();
			}

			if (m_Notifier) delete m_Notifier;
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}

		_log.Log(LOG_STATUS, "(%s) Stopped.", Name.c_str());

		return true;
	}

	void CPlugin::Do_Work()
	{
		_log.Log(LOG_STATUS, "(%s) Entering work loop.", Name.c_str());
		m_LastHeartbeat = mytime(NULL);
		int scounter = m_iPollInterval * 2;
		while (!m_stoprequested)
		{
			if (!--scounter)
			{
				//	Add heartbeat to message queue
				HeartbeatCallback*	Message = new HeartbeatCallback(this);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
				scounter = m_iPollInterval * 2;

				m_LastHeartbeat = mytime(NULL);
			}

			// Check all connections are still valid, vector could be affected by a disconnect on another thread
			try
			{
				boost::lock_guard<boost::mutex> l(m_TransportsMutex);
				if (m_Transports.size())
				{
					for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
					{
						CPluginTransport*	pPluginTransport = *itt;
						pPluginTransport->VerifyConnection();
					}
				}
			}
			catch (...)
			{
				_log.Log(LOG_NORM, "(%s) Transport vector changed during %s loop, continuing.", Name.c_str(), __func__);
			}

			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "(%s) Exiting work loop.", Name.c_str());
	}

	void CPlugin::Restart()
	{
		StopHardware();
		StartHardware();
	}

	bool CPlugin::Initialise()
	{
		m_bIsStarted = false;

		boost::lock_guard<boost::mutex> l(PythonMutex);

		try
		{
			m_PyInterpreter = Py_NewInterpreter();
			if (!m_PyInterpreter)
			{
				_log.Log(LOG_ERROR, "(%s) failed to create interpreter.", m_PluginKey.c_str());
				return false;
			}

			// Prepend plugin directory to path so that python will search it early when importing
	#ifdef WIN32
			std::wstring	sSeparator = L";";
	#else
			std::wstring	sSeparator = L":";
	#endif
			std::wstringstream ssPath;
			std::string		sFind = "key=\"" + m_PluginKey + "\"";
			CPluginSystem Plugins;
			std::map<std::string, std::string>*	mPluginXml = Plugins.GetManifest();
			std::string		sPluginXML;
			for (std::map<std::string, std::string>::iterator it_type = mPluginXml->begin(); it_type != mPluginXml->end(); it_type++)
			{
				if (it_type->second.find(sFind) != std::string::npos)
				{
					m_HomeFolder = it_type->first;
					ssPath << m_HomeFolder.c_str();
					sPluginXML = it_type->second;
					break;
				}
			}
			std::wstring	sPath = ssPath.str() + sSeparator;
			sPath += Py_GetPath();
			PySys_SetPath((wchar_t*)sPath.c_str());

			try
			{
				m_PyModule = PyImport_ImportModule("plugin");
				if (!m_PyModule)
				{
					_log.Log(LOG_ERROR, "(%s) failed to load 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
					LogPythonException();
					return false;
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "(%s) exception loading 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
			}

			// Domoticz callbacks need state so they know which plugin to act on
			PyObject* pMod = PyState_FindModule(&DomoticzModuleDef);
			if (!pMod)
			{
				_log.Log(LOG_ERROR, "(%s) start up failed, Domoticz module not found in interpreter.", m_PluginKey.c_str());
				return false;
			}
			module_state*	pModState = ((struct module_state*)PyModule_GetState(pMod));
			pModState->pPlugin = this;

			//Start worker thread
			m_stoprequested = false;
			m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPlugin::Do_Work, this)));

			if (!m_thread)
			{
				_log.Log(LOG_ERROR, "(%s) failed start worker thread.", m_PluginKey.c_str());
				return false;
			}

			//	Add start command to message queue
			StartCallback*	Message = new StartCallback(this);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			std::string		sExtraDetail;
			TiXmlDocument	XmlDoc;
			XmlDoc.Parse(sPluginXML.c_str());
			if (XmlDoc.Error())
			{
				_log.Log(LOG_ERROR, "%s: Error '%s' at line %d column %d in XML '%s'.", __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), sPluginXML.c_str());
			}
			else
			{
				TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
				for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
				{
					TiXmlElement* pXmlEle = pXmlNode->ToElement();
					if (pXmlEle)
					{
						const char*	pAttributeValue = pXmlEle->Attribute("version");
						if (pAttributeValue)
						{
							m_Version = pAttributeValue;
							sExtraDetail += "version ";
							sExtraDetail += pAttributeValue;
						}
						pAttributeValue = pXmlEle->Attribute("author");
						if (pAttributeValue)
						{
							m_Author = pAttributeValue;
							if (sExtraDetail.length()) sExtraDetail += ", ";
							sExtraDetail += "author '";
							sExtraDetail += pAttributeValue;
							sExtraDetail += "'";
						}
					}
				}
			}
			_log.Log(LOG_STATUS, "(%s) Initialized %s", Name.c_str(), sExtraDetail.c_str());

			return true;
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
		}

		return false;
	}

	bool CPlugin::Start()
	{
		boost::lock_guard<boost::mutex> l(PythonMutex);
		try
		{
			PyObject* pModuleDict = PyModule_GetDict((PyObject*)m_PyModule);  // returns a borrowed referece to the __dict__ object for the module
			PyObject *pParamsDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Parameters", pParamsDict) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add Parameters dictionary.", m_PluginKey.c_str());
				return false;
			}
			Py_DECREF(pParamsDict);

			PyObject*	pObj = Py_BuildValue("i", m_HwdID);
			if (PyDict_SetItemString(pParamsDict, "HardwareID", pObj) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add key 'HardwareID', value '%s' to dictionary.", m_PluginKey.c_str(), m_HwdID);
				return false;
			}
			Py_DECREF(pObj);

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Name, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID==%d)", m_HwdID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					const char*	pChar = sd[0].c_str();
					ADD_STRING_TO_DICT(pParamsDict, "HomeFolder", m_HomeFolder);
					ADD_STRING_TO_DICT(pParamsDict, "Version", m_Version);
					ADD_STRING_TO_DICT(pParamsDict, "Author", m_Author);
					ADD_STRING_TO_DICT(pParamsDict, "Name", sd[0]);
					ADD_STRING_TO_DICT(pParamsDict, "Address", sd[1]);
					ADD_STRING_TO_DICT(pParamsDict, "Port", sd[2]);
					ADD_STRING_TO_DICT(pParamsDict, "SerialPort", sd[3]);
					ADD_STRING_TO_DICT(pParamsDict, "Username", sd[4]);
					ADD_STRING_TO_DICT(pParamsDict, "Password", sd[5]);
					ADD_STRING_TO_DICT(pParamsDict, "Key", sd[6]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode1", sd[7]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode2", sd[8]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode3", sd[9]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode4", sd[10]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode5", sd[11]);
					ADD_STRING_TO_DICT(pParamsDict, "Mode6", sd[12]);

					// Remember these for use with some protocols
					m_Username = sd[4];
					m_Password = sd[5];
				}
			}

			m_DeviceDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Devices", (PyObject*)m_DeviceDict) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add Device dictionary.", m_PluginKey.c_str());
				return false;
			}

			// load associated devices to make them available to python
			result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) ORDER BY Unit ASC", m_HwdID);
			if (result.size() > 0)
			{
				PyType_Ready(&CDeviceType);
				// Add device objects into the device dictionary with Unit as the key
				for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					CDevice* pDevice = (CDevice*)CDevice_new(&CDeviceType, (PyObject*)NULL, (PyObject*)NULL);

					PyObject*	pKey = PyLong_FromLong(atoi(sd[0].c_str()));
					if (PyDict_SetItem((PyObject*)m_DeviceDict, pKey, (PyObject*)pDevice) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to add unit '%s' to device dictionary.", m_PluginKey.c_str(), sd[0].c_str());
						return false;
					}
					pDevice->pPlugin = this;
					pDevice->PluginKey = PyUnicode_FromString(m_PluginKey.c_str());
					pDevice->HwdID = m_HwdID;
					pDevice->Unit = atoi(sd[0].c_str());
					CDevice_refresh(pDevice);
					Py_DECREF(pDevice);
					Py_DECREF(pKey);
				}
			}

			m_ImageDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Images", (PyObject*)m_ImageDict) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add Image dictionary.", m_PluginKey.c_str());
				return false;
			}

			// load associated custom images to make them available to python
			result = m_sql.safe_query("SELECT ID, Base, Name, Description FROM CustomImages WHERE Base LIKE '%q%%' ORDER BY ID ASC", m_PluginKey.c_str());
			if (result.size() > 0)
			{
				PyType_Ready(&CImageType);
				// Add image objects into the image dictionary with ID as the key
				for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					CImage* pImage = (CImage*)CImage_new(&CImageType, (PyObject*)NULL, (PyObject*)NULL);

					PyObject*	pKey = PyUnicode_FromString(sd[1].c_str());
					if (PyDict_SetItem((PyObject*)m_ImageDict, pKey, (PyObject*)pImage) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to add ID '%s' to image dictionary.", m_PluginKey.c_str(), sd[0].c_str());
						return false;
					}
					pImage->ImageID = atoi(sd[0].c_str()) + 100;
					pImage->Base = PyUnicode_FromString(sd[1].c_str());
					pImage->Name = PyUnicode_FromString(sd[2].c_str());
					pImage->Description = PyUnicode_FromString(sd[3].c_str());
					Py_DECREF(pImage);
					Py_DECREF(pKey);
				}
			}

			LoadSettings();

			m_bIsStarted = true;
			return true;
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
		}

		return false;
	}

	void CPlugin::ConnectionProtocol(CDirectiveBase * pMess)
	{
		ProtocolDirective*	pMessage = (ProtocolDirective*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;
		if (pConnection->pProtocol)
		{
			delete pConnection->pProtocol;
			pConnection->pProtocol = NULL;
		}
		std::string	sProtocol = PyUnicode_AsUTF8(pConnection->Protocol);
		if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol set to: '%s'.", Name.c_str(), sProtocol.c_str());
		if (sProtocol == "Line") pConnection->pProtocol = (CPluginProtocol*) new CPluginProtocolLine();
		else if (sProtocol == "XML") pConnection->pProtocol = (CPluginProtocol*) new CPluginProtocolXML();
		else if (sProtocol == "JSON") pConnection->pProtocol = (CPluginProtocol*) new CPluginProtocolJSON();
		else if (sProtocol == "HTTP")
		{
			CPluginProtocolHTTP*	pProtocol = new CPluginProtocolHTTP();
			pProtocol->AuthenticationDetails(m_Username, m_Password);
			pConnection->pProtocol = (CPluginProtocol*)pProtocol;
		}
		else if (sProtocol == "ICMP") pConnection->pProtocol = (CPluginProtocol*) new CPluginProtocolICMP();
		else pConnection->pProtocol = new CPluginProtocol();
	}

	void CPlugin::ConnectionConnect(CDirectiveBase * pMess)
	{
		ConnectDirective*	pMessage = (ConnectDirective*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", Name.c_str());
			return;
		}

		std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
		std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
		if (sTransport == "TCP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCP(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
		}
		else if (sTransport == "Serial")
		{
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', '%s', %d.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), pConnection->Baud);
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportSerial(m_HwdID, (PyObject*)pConnection, sAddress, pConnection->Baud);
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) Invalid transport type for connecting specified: '%s', valid types are TCP/IP and Serial.", Name.c_str(), (PyObject*)pConnection, sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleConnect())
		{
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Connect directive received, action initiated successfully.", Name.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "(%s) Connect directive received, action initiation failed.", Name.c_str());
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionListen(CDirectiveBase * pMess)
	{
		ListenDirective*	pMessage = (ListenDirective*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", Name.c_str());
			return;
		}

		std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
		std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
		if (sTransport == "TCP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCP(m_HwdID, (PyObject*)pConnection, "", sPort);
		}
		else if (sTransport == "UDP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportUDP(m_HwdID, (PyObject*)pConnection, sAddress.c_str(), sPort);
		}
		else if (sTransport == "ICMP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s.", Name.c_str(), sTransport.c_str(), sAddress.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportICMP(m_HwdID, (PyObject*)pConnection, sAddress.c_str(), sPort);
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) Invalid transport type for listening specified: '%s', valid types are TCP/IP, UDP/IP and ICMP/IP.", Name.c_str(), (PyObject*)pConnection, sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleListen())
		{
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Listen directive received, action initiated successfully.", Name.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "(%s) Listen directive received, action initiation failed.", Name.c_str());
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionRead(CPluginMessageBase * pMess)
	{
		ReadMessage*	pMessage = (ReadMessage*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

		if (!pConnection->pProtocol)
		{
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol not specified, 'None' assumed.", Name.c_str());
			pConnection->pProtocol = new CPluginProtocol();
		}
		pConnection->pProtocol->ProcessInbound(pMessage);
	}

	void CPlugin::ConnectionWrite(CDirectiveBase* pMess)
	{
		WriteDirective*	pMessage = (WriteDirective*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;
		std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
		std::string	sConnection = PyUnicode_AsUTF8(pConnection->Name);
		if (pConnection->pTransport)
		{
			if (sTransport == "UDP/IP")
			{
				_log.Log(LOG_ERROR, "(%s) Connectionless Transport is listening, write directive to '%s' ignored.", Name.c_str(), sConnection.c_str());
				return;
			}

			if ((sTransport != "ICMP/IP") && (!pConnection->pTransport->IsConnected()))
			{
				_log.Log(LOG_ERROR, "(%s) Transport is not connected, write directive to '%s' ignored.", Name.c_str(), sConnection.c_str());
				return;
			}
		}

		if (!pConnection->pTransport)
		{
			// UDP is connectionless so create a temporary transport and write to it
			if (sTransport == "UDP/IP")
			{
				std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
				std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
				if (m_bDebug)
					if (sPort.length())
						_log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s for '%s'.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str(), sConnection.c_str());
					else
						_log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s for '%s'.", Name.c_str(), sTransport.c_str(), sAddress.c_str(), sConnection.c_str());
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportUDP(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Transport is not connected, write directive to '%s' ignored.", Name.c_str(), sConnection.c_str());
				return;
			}
		}

		if (!pConnection->pProtocol)
		{
			if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol for '%s' not specified, 'None' assumed.", Name.c_str(), sConnection.c_str());
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::vector<byte>	vWriteData = pConnection->pProtocol->ProcessOutbound(pMessage);
		if (m_bDebug)
		{
			WriteDebugBuffer(vWriteData, false);
		}

		pConnection->pTransport->handleWrite(vWriteData);

		// UDP is connectionless so remove the transport after write
		if (pConnection->pTransport && (sTransport == "UDP/IP"))
		{
			delete pConnection->pTransport;
			pConnection->pTransport = NULL;
		}

	}

	void CPlugin::ConnectionDisconnect(CDirectiveBase * pMess)
	{
		DisconnectDirective*	pMessage = (DisconnectDirective*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

		// Return any partial data to plugin
		if (pConnection->pProtocol)
		{
			pConnection->pProtocol->Flush(pMessage->m_pPlugin, (PyObject*)pConnection);
		}

		if (pConnection->pTransport)
		{
			if (m_bDebug)
			{
				std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
				std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
				std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
				if ((sTransport == "Serial") || (!sPort.length()))
					_log.Log(LOG_NORM, "(%s) Disconnect directive received for '%s'.", Name.c_str(), sAddress.c_str());
				else
					_log.Log(LOG_NORM, "(%s) Disconnect directive received for '%s:%s'.", Name.c_str(), sAddress.c_str(), sPort.c_str());
			}

			// If transport is not connected there won't be a Disconnect Event so tidy it up here
			if (!pConnection->pTransport->IsConnected())
			{
				pConnection->pTransport->handleDisconnect();
				RemoveConnection(pConnection->pTransport);
				CPluginTransport *pTransport = pConnection->pTransport;
				pConnection->pTransport = NULL;
				delete pConnection->pTransport;
			}
			else
			{
				pConnection->pTransport->handleDisconnect();
			}
		}
	}

	void CPlugin::DisconnectEvent(CEventBase * pMess)
	{
		DisconnectedEvent*	pMessage = (DisconnectedEvent*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;
		if (pConnection->pTransport)
		{
			{
				RemoveConnection(pConnection->pTransport);
				CPluginTransport *pTransport = pConnection->pTransport;
				pConnection->pTransport = NULL;
				delete pConnection->pTransport;
			}

			// inform the plugin
			{
				DisconnectMessage*	Message = new DisconnectMessage(this, (PyObject*)pConnection);
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			// Plugin exiting and all connections have disconnect messages queued
			if (m_stoprequested && !m_Transports.size()) 
			{
				StopMessage*	Message = new StopMessage(this);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}
	}

	void CPlugin::Callback(std::string sHandler, void * pParams)
	{
		try
		{
			boost::lock_guard<boost::mutex> l(PythonMutex);
			if (m_PyInterpreter) PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
			if (m_PyModule && sHandler.length())
			{
				PyObject*	pFunc = PyObject_GetAttrString((PyObject*)m_PyModule, sHandler.c_str());
				if (pFunc && PyCallable_Check(pFunc))
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Calling message handler '%s'.", Name.c_str(), sHandler.c_str());

					PyErr_Clear();
					PyObject*	pReturnValue = PyObject_CallObject(pFunc, (PyObject*)pParams);
					if (!pReturnValue)
					{
						LogPythonException(sHandler);
					}
				}
				else if (m_bDebug) _log.Log(LOG_NORM, "(%s) Message handler '%s' not callable, ignored.", Name.c_str(), sHandler.c_str());
			}

			if (pParams) Py_XDECREF(pParams);
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}
	}

	void CPlugin::Stop()
	{
		try
		{
			PyErr_Clear();

			// Stop Python
			if (m_DeviceDict) Py_XDECREF(m_DeviceDict);
			if (m_ImageDict) Py_XDECREF(m_ImageDict);
			if (m_SettingsDict) Py_XDECREF(m_SettingsDict);
			if (m_PyInterpreter) Py_EndInterpreter((PyThreadState*)m_PyInterpreter);
			Py_XDECREF(m_PyModule);
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown releasing Interpreter: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown releasing Interpreter", __func__);
		}
		m_PyModule = NULL;
		m_DeviceDict = NULL;
		m_ImageDict = NULL;
		m_SettingsDict = NULL;
		m_PyInterpreter = NULL;
		m_bIsStarted = false;
	}

	bool CPlugin::LoadSettings()
	{
		PyObject* pModuleDict = PyModule_GetDict((PyObject*)m_PyModule);  // returns a borrowed referece to the __dict__ object for the module
		if (m_SettingsDict) Py_XDECREF(m_SettingsDict);
		m_SettingsDict = PyDict_New();
		if (PyDict_SetItemString(pModuleDict, "Settings", (PyObject*)m_SettingsDict) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add Settings dictionary.", m_PluginKey.c_str());
			return false;
		}

		// load associated settings to make them available to python
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences");
		if (result.size() > 0)
		{
			PyType_Ready(&CDeviceType);
			// Add settings strings into the settings dictionary with Unit as the key
			for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;

				PyObject*	pKey = PyUnicode_FromString(sd[0].c_str());
				PyObject*	pValue = NULL;
				if (sd[2].length())
				{
					pValue = PyUnicode_FromString(sd[2].c_str());
				}
				else
				{
					pValue = PyUnicode_FromString(sd[1].c_str());
				}
				if (PyDict_SetItem((PyObject*)m_SettingsDict, pKey, pValue))
				{
					_log.Log(LOG_ERROR, "(%s) failed to add setting '%s' to settings dictionary.", m_PluginKey.c_str(), sd[0].c_str());
					return false;
				}
				Py_XDECREF(pValue);
				Py_XDECREF(pKey);
			}
		}

		return true;
	}

#define DZ_BYTES_PER_LINE 20
	void CPlugin::WriteDebugBuffer(const std::vector<byte>& Buffer, bool Incoming)
	{
		if (m_bDebug)
		{
			if (Incoming)
				_log.Log(LOG_NORM, "(%s) Received %d bytes of data:", Name.c_str(), Buffer.size());
			else
				_log.Log(LOG_NORM, "(%s) Sending %d bytes of data:", Name.c_str(), Buffer.size());

			for (int i = 0; i < (int)Buffer.size(); i = i + DZ_BYTES_PER_LINE)
			{
				std::stringstream ssHex;
				std::string sChars;
				for (int j = 0; j < DZ_BYTES_PER_LINE; j++)
				{
					if (i + j < (int)Buffer.size())
					{
						if (Buffer[i + j] < 16)
							ssHex << '0' << std::hex << (int)Buffer[i + j] << " ";
						else
							ssHex << std::hex << (int)Buffer[i + j] << " ";
						if ((int)Buffer[i + j] > 32) sChars += Buffer[i + j];
						else sChars += ".";
					}
					else ssHex << ".. ";
				}
				_log.Log(LOG_NORM, "(%s)     %s    %s", Name.c_str(), ssHex.str().c_str(), sChars.c_str());
			}
		}
	}

	bool CPlugin::WriteToHardware(const char *pdata, const unsigned char length)
	{
		return true;
	}

	void CPlugin::SendCommand(const int Unit, const std::string &command, const int level, const int hue)
	{
		//	Add command to message queue
		CommandMessage*	Message = new CommandMessage(this, Unit, command, level, hue);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	void CPlugin::SendCommand(const int Unit, const std::string & command, const float level)
	{
		//	Add command to message queue
		CommandMessage*	Message = new CommandMessage(this, Unit, command, level);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	bool CPlugin::HasNodeFailed(const int Unit)
	{
		if (!m_DeviceDict)	return true;

		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while (PyDict_Next((PyObject*)m_DeviceDict, &pos, &key, &value))
		{
			long iKey = PyLong_AsLong(key);
			if (iKey == -1 && PyErr_Occurred())
			{
				PyErr_Clear();
				return false;
			}

			if (iKey == Unit)
			{
				CDevice*	pDevice = (CDevice*)value;
				return (pDevice->TimedOut != 0);
			}
		}

		return false;
	}


	CPluginNotifier::CPluginNotifier(CPlugin* pPlugin, const std::string &NotifierName) : CNotificationBase(NotifierName, OPTIONS_NONE), m_pPlugin(pPlugin)
	{
		m_notifications.AddNotifier(this);
	}

	CPluginNotifier::~CPluginNotifier()
	{
		m_notifications.RemoveNotifier(this);
	}

	bool CPluginNotifier::IsConfigured()
	{
		return true;
	}

	std::string CPluginNotifier::GetCustomIcon(std::string &szCustom)
	{
		int	iIconLine = atoi(szCustom.c_str());
		std::string szRetVal = "Light48";
		if (iIconLine < 100)  // default set of custom icons
		{
			std::string sLine = "";
			std::ifstream infile;
			std::string switchlightsfile = szWWWFolder + "/switch_icons.txt";
			infile.open(switchlightsfile.c_str());
			if (infile.is_open())
			{
				int index = 0;
				while (!infile.eof())
				{
					getline(infile, sLine);
					if ((sLine.size() != 0) && (index++ == iIconLine))
					{
						std::vector<std::string> results;
						StringSplit(sLine, ";", results);
						if (results.size() == 3)
						{
							szRetVal = results[0] + "48";
							break;
						}
					}
				}
				infile.close();
			}
		}
		else  // Uploaded icons
		{
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Base FROM CustomImages WHERE ID = %d", iIconLine - 100);
			if (result.size() == 1)
			{
				std::string sBase = result[0][0];
				return sBase;
			}
		}

		return szRetVal;
	}

	std::string CPluginNotifier::GetIconFile(const std::string &ExtraData)
	{
		std::string	szImageFile;
#ifdef WIN32
		std::string	szImageFolder = szWWWFolder + "\\images\\";
#else
		std::string	szImageFolder = szWWWFolder + "/images/";
#endif

		std::string	szStatus = "Off";
		int	posStatus = (int)ExtraData.find("|Status=");
		if (posStatus >= 0)
		{
			posStatus += 8;
			szStatus = ExtraData.substr(posStatus, ExtraData.find("|", posStatus) - posStatus);
			if (szStatus != "Off") szStatus = "On";
		}

		// Use image is specified
		int	posImage = (int)ExtraData.find("|Image=");
		if (posImage >= 0)
		{
			posImage += 7;
			szImageFile = szImageFolder + ExtraData.substr(posImage, ExtraData.find("|", posImage) - posImage) + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}
		}

		// Use uploaded and custom images
		int	posCustom = (int)ExtraData.find("|CustomImage=");
		if (posCustom >= 0)
		{
			posCustom += 13;
			std::string szCustom = ExtraData.substr(posCustom, ExtraData.find("|", posCustom) - posCustom);
			int iCustom = atoi(szCustom.c_str());
			if (iCustom)
			{
				szImageFile = szImageFolder + GetCustomIcon(szCustom) + "_" + szStatus + ".png";
				if (file_exist(szImageFile.c_str()))
				{
					return szImageFile;
				}
				szImageFile = szImageFolder + GetCustomIcon(szCustom) + "48_" + szStatus + ".png";
				if (file_exist(szImageFile.c_str()))
				{
					return szImageFile;
				}
				szImageFile = szImageFolder + GetCustomIcon(szCustom) + ".png";
				if (file_exist(szImageFile.c_str()))
				{
					return szImageFile;
				}
			}
		}

		// if a switch type was supplied try and work out the image
		int	posType = (int)ExtraData.find("|SwitchType=");
		if (posType >= 0)
		{
			posType += 12;
			std::string	szType = ExtraData.substr(posType, ExtraData.find("|", posType) - posType);
			std::string	szTypeImage;
			_eSwitchType switchtype = (_eSwitchType)atoi(szType.c_str());
			switch (switchtype)
			{
			case STYPE_OnOff:
				if (posCustom >= 0)
				{
					std::string szCustom = ExtraData.substr(posCustom, ExtraData.find("|", posCustom) - posCustom);
					szTypeImage = GetCustomIcon(szCustom);
				}
				else szTypeImage = "Light48";
				break;
			case STYPE_Doorbell:
				szTypeImage = "doorbell48";
				break;
			case STYPE_Contact:
				szTypeImage = "contact48";
				break;
			case STYPE_Blinds:
			case STYPE_BlindsPercentage:
			case STYPE_VenetianBlindsUS:
			case STYPE_VenetianBlindsEU:
			case STYPE_BlindsPercentageInverted:
			case STYPE_BlindsInverted:
				szTypeImage = "blinds48";
				break;
			case STYPE_X10Siren:
				szTypeImage = "siren";
				break;
			case STYPE_SMOKEDETECTOR:
				szTypeImage = "smoke48";
				break;
			case STYPE_Dimmer:
				szTypeImage = "Dimmer48";
				break;
			case STYPE_Motion:
				szTypeImage = "motion48";
				break;
			case STYPE_PushOn:
				szTypeImage = "pushon48";
				break;
			case STYPE_PushOff:
				szTypeImage = "pushon48";
				break;
			case STYPE_DoorContact:
				szTypeImage = "door48";
				break;
			case STYPE_DoorLock:
				szTypeImage = "door48open";
				break;
			case STYPE_Media:
				if (posCustom >= 0)
				{
					std::string szCustom = ExtraData.substr(posCustom, ExtraData.find("|", posCustom) - posCustom);
					szTypeImage = GetCustomIcon(szCustom);
				}
				else szTypeImage = "Media48";
				break;
			default:
				szTypeImage = "logo";
			}
			szImageFile = szImageFolder + szTypeImage + "_" + szStatus + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}

			szImageFile = szImageFolder + szTypeImage + ((szStatus == "Off") ? "-off" : "-on") + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}

			szImageFile = szImageFolder + szTypeImage + ((szStatus == "Off") ? "off" : "on") + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}

			szImageFile = szImageFolder + szTypeImage + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}
		}

		// Image of last resort is the logo
		szImageFile = szImageFolder + "logo.png";
		if (!file_exist(szImageFile.c_str()))
		{
			_log.Log(LOG_ERROR, "Logo image file does not exist: %s", szImageFile.c_str());
			szImageFile = "";
		}
		return szImageFile;
	}

	bool CPluginNotifier::SendMessageImplementation(const uint64_t Idx, const std::string & Name, const std::string & Subject, const std::string & Text, const std::string & ExtraData, const int Priority, const std::string & Sound, const bool bFromNotification)
	{
		// ExtraData = |Name=Test|SwitchType=9|CustomImage=0|Status=On|

		std::string	sIconFile = GetIconFile(ExtraData);
		std::string	sName = "Unknown";
		int	posName = (int)ExtraData.find("|Name=");
		if (posName >= 0)
		{
			posName += 6;
			sName = ExtraData.substr(posName, ExtraData.find("|", posName) - posName);
		}

		std::string	sStatus = "Unknown";
		int	posStatus = (int)ExtraData.find("|Status=");
		if (posStatus >= 0)
		{
			posStatus += 8;
			sStatus = ExtraData.substr(posStatus, ExtraData.find("|", posStatus) - posStatus);
		}

		//	Add command to message queue for every plugin
		boost::lock_guard<boost::mutex> l(PluginMutex);
		NotificationMessage*	Message = new NotificationMessage(m_pPlugin, Subject, Text, sName, sStatus, Priority, Sound, sIconFile);
		PluginMessageQueue.push(Message);

		return true;
	}
}
#endif
