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
extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWebRoot;
extern std::string dbasefile;
extern std::string szAppVersion;
extern std::string szAppHash;
extern std::string szAppDate;
extern MainWorker m_mainworker;

namespace Plugins {

	extern std::mutex PluginMutex;	// controls access to the message queue
	extern std::queue<CPluginMessageBase*>	PluginMessageQueue;

	std::mutex PythonMutex;			// controls access to Python

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

		if (pPlugin)
			Name = pPlugin->m_Name;

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

	int PyDomoticz_ProfileFunc(PyObject *self, PyFrameObject *frame, int what, PyObject *arg)
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
			int lineno = PyFrame_GetLineNumber(frame);
			std::string	sFuncName = "Unknown";
			PyCodeObject*	pCode = frame->f_code;
			if (pCode && pCode->co_filename)
			{
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				sFuncName = pFileBytes->ob_sval;
			}
			if (pCode && pCode->co_name)
			{
				if (!sFuncName.empty()) sFuncName += "\\";
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
				sFuncName = pFuncBytes->ob_sval;
			}

			switch (what)
			{
			case PyTrace_CALL:
				_log.Log(LOG_NORM, "(%s) Calling function at line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			case PyTrace_RETURN:
				_log.Log(LOG_NORM, "(%s) Returning from line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			case PyTrace_EXCEPTION:
				_log.Log(LOG_NORM, "(%s) Exception at line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			}
		}

		return 0;
	}

	int PyDomoticz_TraceFunc(PyObject *self, PyFrameObject *frame, int what, PyObject *arg)
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
			int lineno = PyFrame_GetLineNumber(frame);
			std::string	sFuncName = "Unknown";
			PyCodeObject*	pCode = frame->f_code;
			if (pCode && pCode->co_filename)
			{
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				sFuncName = pFileBytes->ob_sval;
			}
			if (pCode && pCode->co_name)
			{
				if (!sFuncName.empty()) sFuncName += "\\";
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
				sFuncName = pFuncBytes->ob_sval;
			}

			switch (what)
			{
			case PyTrace_CALL:
				_log.Log(LOG_NORM, "(%s) Calling function at line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			case PyTrace_LINE:
				_log.Log(LOG_NORM, "(%s) Executing line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			case PyTrace_EXCEPTION:
				_log.Log(LOG_NORM, "(%s) Exception at line %d in '%s'", pModState->pPlugin->m_Name.c_str(), lineno, sFuncName.c_str());
				break;
			}
		}

		return 0;
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
			if (pModState->pPlugin->m_bDebug & PDM_PYTHON)
			{
				char* msg;
				if (!PyArg_ParseTuple(args, "s", &msg))
				{
					//TODO: Dump data to aid debugging
					_log.Log(LOG_ERROR, "(%s) PyDomoticz_Debug failed to parse parameters: string expected.", pModState->pPlugin->m_Name.c_str());
					LogPythonException(pModState->pPlugin, std::string(__func__));
				}
				else
				{
					std::string	message = "(" + pModState->pPlugin->m_Name + ") " + msg;
					_log.Log((_eLogLevel)LOG_NORM, message);
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
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Log failed to parse parameters: string expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->m_Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_NORM, message);
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Status(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", std::string(__func__).c_str());
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", std::string(__func__).c_str());
		}
		else
		{
			char* msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				_log.Log(LOG_ERROR, "(%s) %s failed to parse parameters: string expected.", pModState->pPlugin->m_Name.c_str(), std::string(__func__).c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->m_Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_STATUS, message);
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
				//TODO: Dump data to aid debugging
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Error failed to parse parameters: string expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->m_Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_ERROR, message);
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
			unsigned int		type;
			if (!PyArg_ParseTuple(args, "i", &type))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				// Maintain backwards compatibility
				if (type == 1) type = PDM_ALL;

				pModState->pPlugin->m_bDebug = (PluginDebugMask)type;
				_log.Log(LOG_NORM, "(%s) Debug logging mask set to: %s%s%s%s%s%s%s%s%s", pModState->pPlugin->m_Name.c_str(),
																					(type == PDM_NONE ? "NONE" : ""),
																					(type & PDM_PYTHON ? "PYTHON " : ""),
																					(type & PDM_PLUGIN ? "PLUGIN " : ""),
																					(type & PDM_QUEUE ? "QUEUE " : ""),
																					(type & PDM_IMAGE ? "IMAGE " : ""),
																					(type & PDM_DEVICE ? "DEVICE " : ""),
																					(type & PDM_CONNECTION ? "CONNECTION " : ""),
																					(type & PDM_MESSAGE ? "MESSAGE " : ""),
																					(type == PDM_ALL ? "ALL" : ""));
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
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				//	Add heartbeat command to message queue
				pModState->pPlugin->MessagePlugin(new PollIntervalDirective(pModState->pPlugin, iPollinterval));
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
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, Notifier Name expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string		sNotifierName = szNotifier;
				if ((sNotifierName.empty()) || (sNotifierName.find_first_of(' ') != std::string::npos))
				{
					_log.Log(LOG_ERROR, "(%s) failed to parse parameters, valid Notifier Name expected, received '%s'.", pModState->pPlugin->m_Name.c_str(), szNotifier);
				}
				else
				{
					//	Add notifier command to message queue
					pModState->pPlugin->MessagePlugin(new NotifierDirective(pModState->pPlugin, szNotifier));
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Trace(PyObject *self, PyObject *args)
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
			int		bTrace = 0;
			if (!PyArg_ParseTuple(args, "p", &bTrace))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameter, True/False expected.", pModState->pPlugin->m_Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				pModState->pPlugin->m_bTracing = (bool)bTrace;
				_log.Log(LOG_NORM, "(%s) Low level Python tracing %s.", pModState->pPlugin->m_Name.c_str(), (pModState->pPlugin->m_bTracing ? "ENABLED" : "DISABLED"));

				if (pModState->pPlugin->m_bTracing)
				{
					PyEval_SetProfile(PyDomoticz_ProfileFunc, self);
					PyEval_SetTrace(PyDomoticz_TraceFunc, self);
				}
				else
				{
					PyEval_SetProfile(NULL, NULL);
					PyEval_SetTrace(NULL, NULL);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Configuration(PyObject *self, PyObject *args, PyObject* kwds)
	{
		PyObject*		pConfig = Py_None;
		std::string		sConfig;
		std::vector<std::vector<std::string> > result;

		Py_INCREF(Py_None);

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
			CPluginProtocolJSON* pProtocol = (CPluginProtocolJSON*)CPluginProtocol::Create("JSON");
			PyObject* pNewConfig = NULL;
			static char* kwlist[] = { "Config", NULL };
			if (PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &pNewConfig))
			{
				// Python object supplied if it is not a dictionary
				if (!PyDict_Check(pNewConfig))
				{
					_log.Log(LOG_ERROR, "CPlugin:%s, Function expects no parameter or a Dictionary.", __func__);
					return pConfig;
				}
				else
				{
					//  Convert to JSON and store
					sConfig = pProtocol->PythontoJSON(pNewConfig);

					// Update database
					m_sql.safe_query("UPDATE Hardware SET Configuration='%q' WHERE (ID == %d)", sConfig.c_str(), pModState->pPlugin->m_HwdID);
				}
			}
			PyErr_Clear();

			// Read the configuration 
			result = m_sql.safe_query("SELECT Configuration FROM Hardware WHERE (ID==%d)", pModState->pPlugin->m_HwdID);
			if (result.empty())
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, Hardware ID not found in database '%d'.", __func__, pModState->pPlugin->m_HwdID);
				return pConfig;
			}

			// Build a Python structure to return 
			sConfig = result[0][0];
			if (sConfig.empty()) sConfig = "{}";
			pConfig = pProtocol->JSONtoPython(sConfig);
			Py_DECREF(Py_None);
		}

		return pConfig;
	}

	static PyMethodDef DomoticzMethods[] = {
		{ "Debug", PyDomoticz_Debug, METH_VARARGS, "Write a message to Domoticz log only if verbose logging is turned on." },
		{ "Log", PyDomoticz_Log, METH_VARARGS, "Write a message to Domoticz log." },
		{ "Status", PyDomoticz_Status, METH_VARARGS, "Write a status message to Domoticz log." },
		{ "Error", PyDomoticz_Error, METH_VARARGS, "Write an error message to Domoticz log." },
		{ "Debugging", PyDomoticz_Debugging, METH_VARARGS, "Set logging level. 1 set verbose logging, all other values use default level" },
		{ "Heartbeat", PyDomoticz_Heartbeat, METH_VARARGS, "Set the heartbeat interval, default 10 seconds." },
		{ "Notifier", PyDomoticz_Notifier, METH_VARARGS, "Enable notification handling with supplied name." },
		{ "Trace", PyDomoticz_Trace, METH_VARARGS, "Enable/Disable line level Python tracing." },
		{ "Configuration", (PyCFunction)PyDomoticz_Configuration, METH_VARARGS | METH_KEYWORDS, "Retrieve and Store structured plugin configuration." },
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
		m_PluginKey(sPluginKey),
		m_iPollInterval(10),
		m_Notifier(NULL),
		m_bDebug(PDM_NONE),
		m_PyInterpreter(NULL),
		m_PyModule(NULL),
		m_DeviceDict(NULL),
		m_ImageDict(NULL),
		m_SettingsDict(NULL)
	{
		m_HwdID = HwdID;
		m_Name = sName;
		m_bIsStarted = false;
		m_bIsStarting = false;
		m_bTracing = false;
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
			_log.Log(LOG_ERROR, "(%s) Module Import failed, exception: '%s'", m_Name.c_str(), TypeName->tp_name);
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
				if (!sError.empty())
				{
					_log.Log(LOG_ERROR, "(%s) Module Import failed: '%s'", m_Name.c_str(), sError.c_str());
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

				if (!sError.empty())
				{
					if ((lineno > 0) && (lineno < 1000))
					{
						_log.Log(LOG_ERROR, "(%s) Import detail: %s, Line: %lld, offset: %lld", m_Name.c_str(), sError.c_str(), lineno, offset);
					}
					else
					{
						_log.Log(LOG_ERROR, "(%s) Import detail: %s, Line: %lld", m_Name.c_str(), sError.c_str(), offset);
					}
					sError = "";
				}

				if (PyObject_HasAttrString(pExcept, "text"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "text");
					std::string		sUTF = PyUnicode_AsUTF8(pString);
					_log.Log(LOG_ERROR, "(%s) Error Line '%s'", m_Name.c_str(), sUTF.c_str());
					Py_XDECREF(pString);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Error Line details not available.", m_Name.c_str());
				}

				if (!sError.empty())
				{
					_log.Log(LOG_ERROR, "(%s) Import detail: %s", m_Name.c_str(), sError.c_str());
				}
			}
			else _log.Log(LOG_ERROR, "(%s) Module Import failed '%s'", m_Name.c_str(), pErrBytes->ob_sval);
		}

		if (pErrBytes) Py_XDECREF(pErrBytes);

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to import module failed, unable to decode exception.", m_Name.c_str());
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
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s':'%s'.", m_Name.c_str(), sHandler.c_str(), pTypeText, pErrBytes->ob_sval);
		}
		if (pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", m_Name.c_str(), sHandler.c_str(), pTypeText);
		}
		if (!pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", m_Name.c_str(), sHandler.c_str(), pErrBytes->ob_sval);
		}
		if (!pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed, unable to determine error.", m_Name.c_str(), sHandler.c_str());
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
				std::string		FileName = "";
				if (pCode->co_filename)
				{
					PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
					FileName = pFileBytes->ob_sval;
					Py_XDECREF(pFileBytes);
				}
				std::string		FuncName = "Unknown";
				if (pCode->co_name)
				{
					PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
					FuncName = pFuncBytes->ob_sval;
					Py_XDECREF(pFuncBytes);
				}
				if (!FileName.empty())
					_log.Log(LOG_ERROR, "(%s) ----> Line %d in '%s', function %s", m_Name.c_str(), lineno, FileName.c_str(), FuncName.c_str());
				else
					_log.Log(LOG_ERROR, "(%s) ----> Line %d in '%s'", m_Name.c_str(), lineno, FuncName.c_str());
			}
			pTraceback = pTraceback->tb_next;
		}

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to message handler '%s' failed, unable to decode exception.", m_Name.c_str(), sHandler.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	int CPlugin::PollInterval(int Interval)
	{
		if (Interval > 0)
			m_iPollInterval = Interval;
		if (m_bDebug & PDM_PLUGIN) _log.Log(LOG_NORM, "(%s) Heartbeat interval set to: %d.", m_Name.c_str(), m_iPollInterval);
		return m_iPollInterval;
	}

	void CPlugin::Notifier(std::string Notifier)
	{
		if (m_Notifier)
			delete m_Notifier;
		m_Notifier = NULL;
		if (m_bDebug & PDM_PLUGIN) _log.Log(LOG_NORM, "(%s) Notifier Name set to: %s.", m_Name.c_str(), Notifier.c_str());
		m_Notifier = new CPluginNotifier(this, Notifier);
	}

	void CPlugin::AddConnection(CPluginTransport *pTransport)
	{
		std::lock_guard<std::mutex> l(m_TransportsMutex);
		m_Transports.push_back(pTransport);
	}

	void CPlugin::RemoveConnection(CPluginTransport *pTransport)
	{
		std::lock_guard<std::mutex> l(m_TransportsMutex);
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

		RequestStart();

		//	Add start command to message queue
		m_bIsStarting = true;
		MessagePlugin(new InitializeMessage(this));

		_log.Log(LOG_STATUS, "(%s) Started.", m_Name.c_str());

		return true;
	}

	void CPlugin::ClearMessageQueue()
	{
		// Copy the event queue to a temporary one, then copy back the events for other plugins
		std::lock_guard<std::mutex> l(PluginMutex);
		std::queue<CPluginMessageBase*>	TempMessageQueue(PluginMessageQueue);
		while (!PluginMessageQueue.empty())
			PluginMessageQueue.pop();

		while (!TempMessageQueue.empty())
		{
			CPluginMessageBase* FrontMessage = TempMessageQueue.front();
			TempMessageQueue.pop();
			if (FrontMessage->m_pPlugin == this)
			{
				// log events that will not be processed
				CCallbackBase* pCallback = dynamic_cast<CCallbackBase*>(FrontMessage);
				if (pCallback)
					_log.Log(LOG_ERROR, "(%s) Callback event '%s' (Python call '%s') discarded.", m_Name.c_str(), FrontMessage->Name(), pCallback->PythonName());
				else
					_log.Log(LOG_ERROR, "(%s) Non-callback event '%s' discarded.", m_Name.c_str(), FrontMessage->Name());
			}
			else
			{
				// Message is for a different plugin so requeue it
				_log.Log(LOG_NORM, "(%s) requeuing '%s' message for '%s'", m_Name.c_str(), FrontMessage->Name(), FrontMessage->Plugin()->m_Name.c_str());
				PluginMessageQueue.push(FrontMessage);
			}
		}
	}

	bool CPlugin::StopHardware()
	{
		try
		{
			_log.Log(LOG_STATUS, "(%s) Stop directive received.", m_Name.c_str());

			// loop on plugin to finish startup
			while (m_bIsStarting)
			{
				sleep_milliseconds(100);
			}

			RequestStop();

			if (m_bIsStarted)
			{
				// If we have connections queue disconnects
				if (m_Transports.size())
				{
					std::lock_guard<std::mutex> lPython(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
					                                                  // TODO: Must take before m_TransportsMutex to avoid deadlock, try to improve to allow only taking when needed
					std::lock_guard<std::mutex> lTransports(m_TransportsMutex);
					for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
					{
						CPluginTransport*	pPluginTransport = *itt;
						// Tell transport to disconnect if required
						if (pPluginTransport)
						{
							//std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
							MessagePlugin(new DisconnectDirective(this, pPluginTransport->Connection()));
						}
					}
				}
				else
				{
					// otherwise just signal stop
					MessagePlugin(new onStopCallback(this));
				}

				// loop on stop to be processed
				while (m_bIsStarted)
				{
					sleep_milliseconds(100);
				}
			}

			_log.Log(LOG_STATUS, "(%s) Stopping threads.", m_Name.c_str());

			if (m_thread)
			{
				m_thread->join();
				m_thread.reset();
			}

			if (m_Notifier)
			{
				delete m_Notifier;
				m_Notifier = NULL;
			}
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}

		_log.Log(LOG_STATUS, "(%s) Stopped.", m_Name.c_str());

		return true;
	}

	void CPlugin::Do_Work()
	{
		_log.Log(LOG_STATUS, "(%s) Entering work loop.", m_Name.c_str());
		m_LastHeartbeat = mytime(NULL);
		int scounter = m_iPollInterval * 2;
		while (!IsStopRequested(500))
		{
			if (!--scounter)
			{
				//	Add heartbeat to message queue
				MessagePlugin(new onHeartbeatCallback(this));
				scounter = m_iPollInterval * 2;
				m_LastHeartbeat = mytime(NULL);
			}

			// Check all connections are still valid, vector could be affected by a disconnect on another thread
			try
			{
				std::lock_guard<std::mutex> lPython(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
				                                                  // TODO: Must take before m_TransportsMutex to avoid deadlock, try to improve to allow only taking when needed
				std::lock_guard<std::mutex> lTransports(m_TransportsMutex);
				if (m_Transports.size())
				{
					for (std::vector<CPluginTransport*>::iterator itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
					{
						//std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
						CPluginTransport*	pPluginTransport = *itt;
						pPluginTransport->VerifyConnection();
					}
				}
			}
			catch (...)
			{
				_log.Log(LOG_NORM, "(%s) Transport vector changed during %s loop, continuing.", m_Name.c_str(), __func__);
			}
		}

		_log.Log(LOG_STATUS, "(%s) Exiting work loop.", m_Name.c_str());
	}

	bool CPlugin::Initialise()
	{
		m_bIsStarted = false;

		try
		{
			PyEval_RestoreThread((PyThreadState*)m_mainworker.m_pluginsystem.PythonThread());
			m_PyInterpreter = Py_NewInterpreter();
			if (!m_PyInterpreter)
			{
				_log.Log(LOG_ERROR, "(%s) failed to create interpreter.", m_PluginKey.c_str());
				goto Error;
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

			try
			{
				//
				//	Python loads the 'site' module automatically and adds extra search directories for module loading
				//	This code makes the plugin framework function the same way
				//
				void*	pSiteModule = PyImport_ImportModule("site");
				if (!pSiteModule)
				{
					_log.Log(LOG_ERROR, "(%s) failed to load 'site' module, continuing.", m_PluginKey.c_str());
				}
				else
				{
					PyObject*	pFunc = PyObject_GetAttrString((PyObject*)pSiteModule, "getsitepackages");
					if (pFunc && PyCallable_Check(pFunc))
					{
						PyObject*	pSites = PyObject_CallObject(pFunc, NULL);
						if (!pSites)
						{
							LogPythonException("getsitepackages");
						}
						else
							for (Py_ssize_t i = 0; i < PyList_Size(pSites); i++)
							{
								PyObject*	pSite = PyList_GetItem(pSites, i);
								if (pSite && PyUnicode_Check(pSite))
								{
									std::wstringstream ssPath;
									ssPath << PyUnicode_AsUTF8(pSite);
									sPath += sSeparator + ssPath.str();
								}
							}
						Py_XDECREF(pSites);
					}
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "(%s) exception loading 'site' module, continuing.", m_PluginKey.c_str());
				PyErr_Clear();
			}

			// Update the path itself
			PySys_SetPath((wchar_t*)sPath.c_str());

			try
			{
				//
				//	Load the 'faulthandler' module to get a python stackdump during a segfault
				//
				void*	pFaultModule = PyImport_ImportModule("faulthandler");
				if (!pFaultModule)
				{
					_log.Log(LOG_ERROR, "(%s) failed to load 'faulthandler' module, continuing.", m_PluginKey.c_str());
				}
				else
				{
					PyObject*	pFunc = PyObject_GetAttrString((PyObject*)pFaultModule, "enable");
					if (pFunc && PyCallable_Check(pFunc))
					{
						PyObject_CallObject(pFunc, NULL);
					}
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "(%s) exception loading 'faulthandler' module, continuing.", m_PluginKey.c_str());
				PyErr_Clear();
			}

			try
			{
				m_PyModule = PyImport_ImportModule("plugin");
				if (!m_PyModule)
				{
					_log.Log(LOG_ERROR, "(%s) failed to load 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
					LogPythonException();
					goto Error;
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "(%s) exception loading 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
				PyErr_Clear();
			}

			// Domoticz callbacks need state so they know which plugin to act on
			PyObject* pMod = PyState_FindModule(&DomoticzModuleDef);
			if (!pMod)
			{
				_log.Log(LOG_ERROR, "(%s) start up failed, Domoticz module not found in interpreter.", m_PluginKey.c_str());
				goto Error;
			}
			module_state*	pModState = ((struct module_state*)PyModule_GetState(pMod));
			pModState->pPlugin = this;

			//Start worker thread
			m_thread = std::make_shared<std::thread>(&CPlugin::Do_Work, this);
			std::string plugin_name = "Plugin_" + m_PluginKey;
			SetThreadName(m_thread->native_handle(), plugin_name.c_str());

			if (!m_thread)
			{
				_log.Log(LOG_ERROR, "(%s) failed start worker thread.", m_PluginKey.c_str());
				goto Error;
			}

			//	Add start command to message queue
			MessagePlugin(new onStartCallback(this));

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
							if (!sExtraDetail.empty()) sExtraDetail += ", ";
							sExtraDetail += "author '";
							sExtraDetail += pAttributeValue;
							sExtraDetail += "'";
						}
					}
				}
			}
			_log.Log(LOG_STATUS, "(%s) Initialized %s", m_Name.c_str(), sExtraDetail.c_str());

			PyEval_SaveThread();
			return true;
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
		}

Error:
		PyEval_SaveThread();
		m_bIsStarting = false;
		return false;
	}

	bool CPlugin::Start()
	{
		try
		{
			PyObject* pModuleDict = PyModule_GetDict((PyObject*)m_PyModule);  // returns a borrowed referece to the __dict__ object for the module
			PyObject *pParamsDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Parameters", pParamsDict) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add Parameters dictionary.", m_PluginKey.c_str());
				goto Error;
			}
			Py_DECREF(pParamsDict);

			PyObject*	pObj = Py_BuildValue("i", m_HwdID);
			if (PyDict_SetItemString(pParamsDict, "HardwareID", pObj) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add key 'HardwareID', value '%d' to dictionary.", m_PluginKey.c_str(), m_HwdID);
				goto Error;
			}
			Py_DECREF(pObj);

			std::string sLanguage = "en";
			m_sql.GetPreferencesVar("Language", sLanguage);

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Name, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID==%d)", m_HwdID);
			if (!result.empty())
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					const char*	pChar = sd[0].c_str();
					ADD_STRING_TO_DICT(pParamsDict, "HomeFolder", m_HomeFolder);
					ADD_STRING_TO_DICT(pParamsDict, "StartupFolder", szStartupFolder);
					ADD_STRING_TO_DICT(pParamsDict, "UserDataFolder", szUserDataFolder);
					ADD_STRING_TO_DICT(pParamsDict, "WebRoot", szWebRoot);
					ADD_STRING_TO_DICT(pParamsDict, "Database", dbasefile);
					ADD_STRING_TO_DICT(pParamsDict, "Language", sLanguage);
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

					ADD_STRING_TO_DICT(pParamsDict, "DomoticzVersion", szAppVersion);
					ADD_STRING_TO_DICT(pParamsDict, "DomoticzHash", szAppHash);
					ADD_STRING_TO_DICT(pParamsDict, "DomoticzBuildTime", szAppDate);
				}
			}

			m_DeviceDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Devices", (PyObject*)m_DeviceDict) == -1)
			{
				_log.Log(LOG_ERROR, "(%s) failed to add Device dictionary.", m_PluginKey.c_str());
				goto Error;
			}

			// load associated devices to make them available to python
			result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) ORDER BY Unit ASC", m_HwdID);
			if (!result.empty())
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
						goto Error;
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
				goto Error;
			}

			// load associated custom images to make them available to python
			result = m_sql.safe_query("SELECT ID, Base, Name, Description FROM CustomImages WHERE Base LIKE '%q%%' ORDER BY ID ASC", m_PluginKey.c_str());
			if (!result.empty())
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
						goto Error;
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
			m_bIsStarting = false;
			return true;
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
		}

Error:
		m_bIsStarting = false;
		return false;
	}

	void CPlugin::ConnectionProtocol(CDirectiveBase * pMess)
	{
		ProtocolDirective*	pMessage = (ProtocolDirective*)pMess;
		CConnection*		pConnection = (CConnection*)pMessage->m_pConnection;
		if (m_Notifier)
		{
			delete pConnection->pProtocol;
			pConnection->pProtocol = NULL;
		}
		std::string	sProtocol = PyUnicode_AsUTF8(pConnection->Protocol);
		pConnection->pProtocol = CPluginProtocol::Create(sProtocol);
		if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Protocol set to: '%s'.", m_Name.c_str(), sProtocol.c_str());
	}

	void CPlugin::ConnectionConnect(CDirectiveBase * pMess)
	{
		ConnectDirective*	pMessage = (ConnectDirective*)pMess;
		CConnection*		pConnection = (CConnection*)pMessage->m_pConnection;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", m_Name.c_str());
			return;
		}

		if (!pConnection->pProtocol)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string	sConnection = PyUnicode_AsUTF8(pConnection->Name);
				_log.Log(LOG_NORM, "(%s) Protocol for '%s' not specified, 'None' assumed.", m_Name.c_str(), sConnection.c_str());
			}
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
		std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
		if ((sTransport == "TCP/IP") || (sTransport == "TLS/IP"))
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			if (sPort.empty())
			{
				_log.Log(LOG_ERROR, "(%s) No port number specified for %s connection to: '%s'.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str());
				return;
			}
			if ((sTransport == "TLS/IP") || pConnection->pProtocol->Secure())
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCPSecure(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
			else
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCP(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
		}
		else if (sTransport == "Serial")
		{
			if (pConnection->pProtocol->Secure())  _log.Log(LOG_ERROR, "(%s) Transport '%s' does not support secure connections.", m_Name.c_str(), sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', '%s', %d.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), pConnection->Baud);
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportSerial(m_HwdID, (PyObject*)pConnection, sAddress, pConnection->Baud);
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) Invalid transport type for connecting specified: '%s', valid types are TCP/IP and Serial.", m_Name.c_str(), sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleConnect())
		{
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Connect directive received, action initiated successfully.", m_Name.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "(%s) Connect directive received, action initiation failed.", m_Name.c_str());
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionListen(CDirectiveBase * pMess)
	{
		ListenDirective*	pMessage = (ListenDirective*)pMess;
		CConnection*		pConnection = (CConnection*)pMessage->m_pConnection;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", m_Name.c_str());
			return;
		}

		if (!pConnection->pProtocol)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string	sConnection = PyUnicode_AsUTF8(pConnection->Name);
				_log.Log(LOG_NORM, "(%s) Protocol for '%s' not specified, 'None' assumed.", m_Name.c_str(), sConnection.c_str());
			}
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
		std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
		if (sTransport == "TCP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			if (!pConnection->pProtocol->Secure())
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCP(m_HwdID, (PyObject*)pConnection, "", sPort);
			else
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportTCPSecure(m_HwdID, (PyObject*)pConnection, "", sPort);
		}
		else if (sTransport == "UDP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (pConnection->pProtocol->Secure())  _log.Log(LOG_ERROR, "(%s) Transport '%s' does not support secure connections.", m_Name.c_str(), sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportUDP(m_HwdID, (PyObject*)pConnection, sAddress.c_str(), sPort);
		}
		else if (sTransport == "ICMP/IP")
		{
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if (pConnection->pProtocol->Secure())  _log.Log(LOG_ERROR, "(%s) Transport '%s' does not support secure connections.", m_Name.c_str(), sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str());
			pConnection->pTransport = (CPluginTransport*) new CPluginTransportICMP(m_HwdID, (PyObject*)pConnection, sAddress.c_str(), sPort);
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) Invalid transport type for listening specified: '%s', valid types are TCP/IP, UDP/IP and ICMP/IP.", m_Name.c_str(), sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleListen())
		{
			if (m_bDebug & PDM_CONNECTION) _log.Log(LOG_NORM, "(%s) Listen directive received, action initiated successfully.", m_Name.c_str());
		}
		else
		{
			_log.Log(LOG_NORM, "(%s) Listen directive received, action initiation failed.", m_Name.c_str());
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionRead(CPluginMessageBase * pMess)
	{
		ReadEvent*	pMessage = (ReadEvent*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

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
				_log.Log(LOG_ERROR, "(%s) Connectionless Transport is listening, write directive to '%s' ignored.", m_Name.c_str(), sConnection.c_str());
				return;
			}

			if ((sTransport != "ICMP/IP") && (!pConnection->pTransport->IsConnected()))
			{
				_log.Log(LOG_ERROR, "(%s) Transport is not connected, write directive to '%s' ignored.", m_Name.c_str(), sConnection.c_str());
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
				if (m_bDebug & PDM_CONNECTION)
				{
					if (!sPort.empty())
						_log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s for '%s'.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), sPort.c_str(), sConnection.c_str());
					else
						_log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s for '%s'.", m_Name.c_str(), sTransport.c_str(), sAddress.c_str(), sConnection.c_str());
				}
				pConnection->pTransport = (CPluginTransport*) new CPluginTransportUDP(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) No transport, write directive to '%s' ignored.", m_Name.c_str(), sConnection.c_str());
				return;
			}
		}

		// Make sure there is a protocol to encode the data
		if (!pConnection->pProtocol)
		{
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::vector<byte>	vWriteData = pConnection->pProtocol->ProcessOutbound(pMessage);
		WriteDebugBuffer(vWriteData, false);

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
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
				std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
				std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
				if ((sTransport == "Serial") || (sPort.empty()))
					_log.Log(LOG_NORM, "(%s) Disconnect directive received for '%s'.", m_Name.c_str(), sAddress.c_str());
				else
					_log.Log(LOG_NORM, "(%s) Disconnect directive received for '%s:%s'.", m_Name.c_str(), sAddress.c_str(), sPort.c_str());
			}

			// If transport is not going to disconnect asynchronously tidy it up here
			if (!pConnection->pTransport->AsyncDisconnect())
			{
				pConnection->pTransport->handleDisconnect();
				RemoveConnection(pConnection->pTransport);
				delete pConnection->pTransport;
				pConnection->pTransport = NULL;

				// Plugin exiting and all connections have disconnect messages queued
				if (IsStopRequested(0) && !m_Transports.size())
				{
					MessagePlugin(new onStopCallback(this));
				}
			}
			else
			{
				pConnection->pTransport->handleDisconnect();
			}
		}
	}

	void CPlugin::onDeviceAdded(int Unit)
	{
		CDevice* pDevice = (CDevice*)CDevice_new(&CDeviceType, (PyObject*)NULL, (PyObject*)NULL);

		PyObject*	pKey = PyLong_FromLong(Unit);
		if (PyDict_SetItem((PyObject*)m_DeviceDict, pKey, (PyObject*)pDevice) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add unit '%d' to device dictionary.", m_PluginKey.c_str(), Unit);
			return;
		}
		pDevice->pPlugin = this;
		pDevice->PluginKey = PyUnicode_FromString(m_PluginKey.c_str());
		pDevice->HwdID = m_HwdID;
		pDevice->Unit = Unit;
		CDevice_refresh(pDevice);
		Py_DECREF(pDevice);
		Py_DECREF(pKey);
	}

	void CPlugin::onDeviceModified(int Unit)
	{
		PyObject*	pKey = PyLong_FromLong(Unit);

		CDevice* pDevice = (CDevice*)PyDict_GetItem((PyObject*)m_DeviceDict, pKey);

		if (!pDevice)
		{
			_log.Log(LOG_ERROR, "(%s) failed to refresh unit '%u' in device dictionary.", m_PluginKey.c_str(), Unit);
			return;
		}

		CDevice_refresh(pDevice);
	}

	void CPlugin::onDeviceRemoved(int Unit)
	{
		PyObject*	pKey = PyLong_FromLong(Unit);
		if (PyDict_DelItem((PyObject*)m_DeviceDict, pKey) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to remove unit '%u' from device dictionary.", m_PluginKey.c_str(), Unit);
		}
	}

	void CPlugin::MessagePlugin(CPluginMessageBase *pMessage)
	{
		if (m_bDebug & PDM_QUEUE)
		{
			_log.Log(LOG_NORM, "(" + m_Name + ") Pushing '" + std::string(pMessage->Name()) + "' on to queue");
		}

		// Add message to queue
		std::lock_guard<std::mutex> l(PluginMutex);
		PluginMessageQueue.push(pMessage);
	}

	void CPlugin::DeviceAdded(int Unit)
	{
		CPluginMessageBase*	pMessage = new onDeviceAddedCallback(this, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DeviceModified(int Unit)
	{
		CPluginMessageBase*	pMessage = new onDeviceModifiedCallback(this, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DeviceRemoved(int Unit)
	{
		CPluginMessageBase*	pMessage = new onDeviceRemovedCallback(this, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DisconnectEvent(CEventBase * pMess)
	{
		DisconnectedEvent*	pMessage = (DisconnectedEvent*)pMess;
		CConnection*	pConnection = (CConnection*)pMessage->m_pConnection;

		// Return any partial data to plugin
		if (pConnection->pProtocol)
		{
			pConnection->pProtocol->Flush(pMessage->m_pPlugin, (PyObject*)pConnection);
		}

		if (pConnection->pTransport)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
				std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
				std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
				if ((sTransport == "Serial") || (sPort.empty()))
					_log.Log(LOG_NORM, "(%s) Disconnect event received for '%s'.", m_Name.c_str(), sAddress.c_str());
				else
					_log.Log(LOG_NORM, "(%s) Disconnect event received for '%s:%s'.", m_Name.c_str(), sAddress.c_str(), sPort.c_str());
			}

			RemoveConnection(pConnection->pTransport);
			delete pConnection->pTransport;
			pConnection->pTransport = NULL;

			// inform the plugin if transport is connection based
			if (pMessage->bNotifyPlugin)
			{
				MessagePlugin(new onDisconnectCallback(this, (PyObject*)pConnection));
			}

			// Plugin exiting and all connections have disconnect messages queued
			if (IsStopRequested(0) && !m_Transports.size())
			{
				MessagePlugin(new onStopCallback(this));
			}
		}
	}

	void CPlugin::RestoreThread()
	{
		if (m_PyInterpreter)
			PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
	}

	void CPlugin::ReleaseThread()
	{
		if (m_PyInterpreter)
			PyEval_SaveThread();
	}

	void CPlugin::Callback(std::string sHandler, void * pParams)
	{
		try
		{
			// Callbacks MUST already have taken the PythonMutex lock otherwise bad things will happen
			if (m_PyModule && !sHandler.empty())
			{
				PyObject*	pFunc = PyObject_GetAttrString((PyObject*)m_PyModule, sHandler.c_str());
				if (pFunc && PyCallable_Check(pFunc))
				{
					if (m_bDebug & PDM_QUEUE) _log.Log(LOG_NORM, "(%s) Calling message handler '%s'.", m_Name.c_str(), sHandler.c_str());

					PyErr_Clear();
					PyObject*	pReturnValue = PyObject_CallObject(pFunc, (PyObject*)pParams);
					if (!pReturnValue)
					{
						LogPythonException(sHandler);
					}
					Py_XDECREF(pReturnValue);
				}
				else if (m_bDebug & PDM_QUEUE) _log.Log(LOG_NORM, "(%s) Message handler '%s' not callable, ignored.", m_Name.c_str(), sHandler.c_str());
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
			PyEval_ReleaseLock();
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown releasing Interpreter: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown releasing Interpreter", __func__);
		}
		ClearMessageQueue();
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
		if (!result.empty())
		{
			PyType_Ready(&CDeviceType);
			// Add settings strings into the settings dictionary with Unit as the key
			for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;

				PyObject*	pKey = PyUnicode_FromString(sd[0].c_str());
				PyObject*	pValue = NULL;
				if (!sd[2].empty())
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
		if (m_bDebug & (PDM_CONNECTION | PDM_MESSAGE))
		{
			if (Incoming)
				_log.Log(LOG_NORM, "(%s) Received %d bytes of data", m_Name.c_str(), (int)Buffer.size());
			else
				_log.Log(LOG_NORM, "(%s) Sending %d bytes of data", m_Name.c_str(), (int)Buffer.size());
		}

		if (m_bDebug & PDM_MESSAGE)
		{
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
				_log.Log(LOG_NORM, "(%s)     %s    %s", m_Name.c_str(), ssHex.str().c_str(), sChars.c_str());
			}
		}
	}

	bool CPlugin::WriteToHardware(const char *pdata, const unsigned char length)
	{
		return true;
	}

	void CPlugin::SendCommand(const int Unit, const std::string &command, const int level, const _tColor color)
	{
		//	Add command to message queue
		std::string JSONColor = color.toJSONString();
		MessagePlugin(new onCommandCallback(this, Unit, command, level, JSONColor));
	}

	void CPlugin::SendCommand(const int Unit, const std::string & command, const float level)
	{
		//	Add command to message queue
		MessagePlugin(new onCommandCallback(this, Unit, command, level));
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
				else 
					szTypeImage = "Light48";
				break;
			case STYPE_Doorbell:
				szTypeImage = "doorbell48";
				break;
			case STYPE_Contact:
				szTypeImage = "Contact48";
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
				szTypeImage = "Push48";
				break;
			case STYPE_PushOff:
				szTypeImage = "Push48";
				break;
			case STYPE_DoorContact:
				szTypeImage = "Door48";
				break;
			case STYPE_DoorLock:
				szTypeImage = "Door48";
				break;
			case STYPE_DoorLockInverted:
				szTypeImage = "Door48";
				break;
			case STYPE_Media:
				if (posCustom >= 0)
				{
					std::string szCustom = ExtraData.substr(posCustom, ExtraData.find("|", posCustom) - posCustom);
					szTypeImage = GetCustomIcon(szCustom);
				}
				else
					szTypeImage = "Media48";
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
		m_pPlugin->MessagePlugin(new onNotificationCallback(m_pPlugin, Subject, Text, sName, sStatus, Priority, Sound, sIconFile));

		return true;
	}
}
#endif
