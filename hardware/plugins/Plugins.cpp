#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "../../main/Helper.h"

#include "Plugins.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"
#include "PythonObjects.h"

#include "../../main/Helper.h"
#include "../../main/Logger.h"
#include "../../main/SQLHelper.h"
#include "../../main/mainworker.h"
#include "../../tinyxpath/tinyxml.h"

#include "../../notifications/NotificationHelper.h"

#define ADD_STRING_TO_DICT(pPlugin, pDict, key, value)                                                                                      \
	{                                                                                                                                       \
		PyNewRef	pStr(value);                                                                               \
		if (!pStr || PyDict_SetItemString(pDict, key, pStr) == -1)                                                                                   \
			pPlugin->Log(LOG_ERROR, "Failed to add key '%s', value '%s' to dictionary.", key, value.c_str());     \
	}

#define GETSTATE(m) ((struct module_state *)PyModule_GetState(m))

extern std::string szWWWFolder;
extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWebRoot;
extern std::string dbasefile;
extern std::string szAppVersion;
extern std::string szAppHash;
extern std::string szAppDate;
extern MainWorker m_mainworker;

namespace Plugins
{
	extern PyTypeObject* CDeviceType;
	extern PyTypeObject* CConnectionType;
	extern PyTypeObject* CImageType;

	AccessPython::AccessPython(CPlugin* pPlugin, const char* sWhat) 
	{
		m_pPlugin = pPlugin;
		m_Text = sWhat;

		if (m_pPlugin)
		{
			if (m_pPlugin->m_bDebug & PDM_LOCKING)
			{
				m_pPlugin->Log(LOG_NORM, "Acquiring GIL for '%s'", m_Text.c_str());
			}
			m_pPlugin->RestoreThread();
		}
		else
		{
			_log.Log(LOG_ERROR, "Attempt to aquire the GIL with NULL Plugin details.");
		}
	}

	AccessPython::~AccessPython()
	{
		if (m_pPlugin)
		{
			if (PyErr_Occurred())
			{
				m_pPlugin->Log(LOG_NORM, "Python error was set during unlock for '%s'", m_Text.c_str());
				m_pPlugin->LogPythonException();
				PyErr_Clear();
			}
			m_pPlugin->ReleaseThread();
		}
	}

	static PyObject *PyDomoticz_Debug(PyObject *self, PyObject *args)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			if (pPlugin->m_bDebug & PDM_PYTHON)
			{
				PyBorrowedRef	pArg(args);
				if (!pArg.IsTuple())
				{
					pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected 'tuple' got '%s'.", __func__, pArg.Type().c_str());
					Py_RETURN_NONE;
				}

				Py_ssize_t	tupleSize = PyTuple_Size(pArg);
				if (tupleSize != 1)
				{
					pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected single parameter, got %d parameters.", __func__, (int)tupleSize);
					Py_RETURN_NONE;
				}

				char *msg;
				if (!PyArg_ParseTuple(args, "s", &msg))
				{
					PyErr_Clear();
					PyObject* pObject;
					if (PyArg_ParseTuple(args, "O", &pObject))
					{
						std::string	sMessage = PyBorrowedRef(pObject);
						pPlugin->Log(LOG_NORM, sMessage);
					}
					else
					{
						pPlugin->Log(LOG_ERROR, "%s: Failed to parse parameters: string expected.", __func__);
						pPlugin->LogPythonException(std::string(__func__));
					}
				}
				else
				{
					pPlugin->Log(LOG_NORM, (std::string)msg);
				}
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Log(PyObject *self, PyObject *args)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			PyBorrowedRef	pArg(args);
			if (!pArg.IsTuple())
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected 'tuple' got '%s'.", __func__, pArg.Type().c_str());
				Py_RETURN_NONE;
			}

			Py_ssize_t	tupleSize = PyTuple_Size(pArg);
			if (tupleSize != 1)
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected single parameter, got %d parameters.", __func__, (int)tupleSize);
				Py_RETURN_NONE;
			}

			char *msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				PyErr_Clear();
				PyObject*	pObject;
				if (PyArg_ParseTuple(args, "O", &pObject))
				{
					std::string	sMessage = PyBorrowedRef(pObject);
					pPlugin->Log(LOG_NORM, sMessage);
				}
				else
				{
					pPlugin->Log(LOG_ERROR, "%s: Failed to parse parameters: string expected.", __func__);
					pPlugin->LogPythonException(std::string(__func__));
				}
			}
			else
			{
				pPlugin->Log(LOG_NORM, (std::string)msg);
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Status(PyObject *self, PyObject *args)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			PyBorrowedRef	pArg(args);
			if (!pArg.IsTuple())
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected 'tuple' got '%s'.", __func__, pArg.Type().c_str());
				Py_RETURN_NONE;
			}

			Py_ssize_t	tupleSize = PyTuple_Size(pArg);
			if (tupleSize != 1)
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected single parameter, got %d parameters.", __func__, (int)tupleSize);
				Py_RETURN_NONE;
			}

			char *msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				PyErr_Clear();
				PyObject* pObject;
				if (PyArg_ParseTuple(args, "O", &pObject))
				{
					std::string	sMessage = PyBorrowedRef(pObject);
					pPlugin->Log(LOG_STATUS, sMessage);
				}
				else
				{
					pPlugin->Log(LOG_ERROR, "%s: Failed to parse parameters: string expected.", __func__);
					pPlugin->LogPythonException(std::string(__func__));
				}
			}
			else
			{
				pPlugin->Log(LOG_STATUS, (std::string)msg);
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Error(PyObject *self, PyObject *args)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			PyBorrowedRef	pArg(args);
			if (!pArg.IsTuple())
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected 'tuple' got '%s'.", __func__, pArg.Type().c_str());
				Py_RETURN_NONE;
			}

			Py_ssize_t	tupleSize = PyTuple_Size(pArg);
			if (tupleSize != 1)
			{
				pPlugin->Log(LOG_ERROR, "%s: Invalid parameter, expected single parameter, got %d parameters.", __func__, (int)tupleSize);
				Py_RETURN_NONE;
			}

			char *msg;
			if ((PyTuple_Size(args) != 1) || !PyArg_ParseTuple(args, "s", &msg))
			{
				PyErr_Clear();
				PyObject* pObject;
				if (PyArg_ParseTuple(args, "O", &pObject))
				{
					std::string	sMessage = PyBorrowedRef(pObject);
					pPlugin->Log(LOG_ERROR, sMessage);
				}
				else
				{
					pPlugin->Log(LOG_ERROR, "%s: Failed to parse parameters: string expected.", __func__);
					pPlugin->LogPythonException(std::string(__func__));
				}
			}
			else
			{
				pPlugin->Log(LOG_ERROR, (std::string)msg);
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Debugging(PyObject *self, PyObject *args)
	{
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			unsigned int type;
			if (!PyArg_ParseTuple(args, "i", &type))
			{
				pModState->pPlugin->Log(LOG_ERROR, "Failed to parse parameters, integer expected.");
				pModState->pPlugin->LogPythonException(std::string(__func__));
			}
			else
			{
				// Maintain backwards compatibility
				if (type == 1)
					type = PDM_ALL;

				pModState->pPlugin->m_bDebug = (PluginDebugMask)type;
				pModState->pPlugin->Log(LOG_NORM, "Debug logging mask set to: %s%s%s%s%s%s%s%s%s", (type == PDM_NONE ? "NONE" : ""),
					 (type & PDM_PYTHON ? "PYTHON " : ""), (type & PDM_PLUGIN ? "PLUGIN " : ""), (type & PDM_QUEUE ? "QUEUE " : ""), (type & PDM_IMAGE ? "IMAGE " : ""),
					 (type & PDM_DEVICE ? "DEVICE " : ""), (type & PDM_CONNECTION ? "CONNECTION " : ""), (type & PDM_MESSAGE ? "MESSAGE " : ""), (type == PDM_ALL ? "ALL" : ""));
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Heartbeat(PyObject *self, PyObject *args)
	{
		int iPollinterval = 0;

		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			iPollinterval = pModState->pPlugin->PollInterval(0);
			if (PyBorrowedRef(args).IsTuple() && PyTuple_Size(args))
			{
				if (!PyArg_ParseTuple(args, "i", &iPollinterval))
				{
					pModState->pPlugin->Log(LOG_ERROR, "failed to parse parameters, integer expected.");
					pModState->pPlugin->LogPythonException(std::string(__func__));
				}
				else
				{
					//	Add heartbeat command to message queue
					pModState->pPlugin->MessagePlugin(new PollIntervalDirective(iPollinterval));
				}
			}
		}

		return PyLong_FromLong(iPollinterval);
	}

	static PyObject *PyDomoticz_Notifier(PyObject *self, PyObject *args)
	{
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			char *szNotifier;
			if (!PyArg_ParseTuple(args, "s", &szNotifier))
			{
				pModState->pPlugin->Log(LOG_ERROR, "Failed to parse parameters, Notifier Name expected.");
				pModState->pPlugin->LogPythonException(std::string(__func__));
			}
			else
			{
				std::string sNotifierName = szNotifier;
				if ((sNotifierName.empty()) || (sNotifierName.find_first_of(' ') != std::string::npos))
				{
					pModState->pPlugin->Log(LOG_ERROR, "Failed to parse parameters, valid Notifier Name expected, received '%s'.", szNotifier);
				}
				else
				{
					//	Add notifier command to message queue
					pModState->pPlugin->MessagePlugin(new NotifierDirective(szNotifier));
				}
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Trace(PyObject *self, PyObject *args)
	{
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			pModState->pPlugin->Log(LOG_ERROR, "CPlugin:%s, Low level trace functions have been removed.", __func__);
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Configuration(PyObject *self, PyObject *args, PyObject *kwds)
	{
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			CPluginProtocolJSON jsonProtocol;
			PyObject *pNewConfig = nullptr;
			static char *kwlist[] = { "Config", nullptr };
			if (PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pNewConfig))
			{
				// Python object supplied if it is not a dictionary
				if (!PyBorrowedRef(pNewConfig).IsDict())
				{
					pModState->pPlugin->Log(LOG_ERROR, "CPlugin:%s, Function expects no parameter or a Dictionary.", __func__);
					Py_RETURN_NONE;
				}
				//  Convert to JSON and store
				std::string sConfig = jsonProtocol.PythontoJSON(pNewConfig);

				// Update database
				m_sql.safe_query("UPDATE Hardware SET Configuration='%q' WHERE (ID == %d)", sConfig.c_str(), pModState->pPlugin->m_HwdID);
			}
			PyErr_Clear();

			// Read the configuration
			std::vector<std::vector<std::string>> result = m_sql.safe_query("SELECT Configuration FROM Hardware WHERE (ID==%d)", pModState->pPlugin->m_HwdID);
			if (result.empty())
			{
				pModState->pPlugin->Log(LOG_ERROR, "CPlugin:%s, Hardware ID not found in database '%d'.", __func__, pModState->pPlugin->m_HwdID);
				Py_RETURN_NONE;
			}

			// Build a Python structure to return
			std::string sConfig = result[0][0];
			if (sConfig.empty())
				sConfig = "{}";

			return jsonProtocol.JSONtoPython(sConfig);
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Register(PyObject *self, PyObject *args, PyObject *kwds)
	{
		static char *kwlist[] = { "Device", "Unit", NULL };
		module_state *pModState = CPlugin::FindModule();
		if (pModState)
		{
			PyTypeObject *pDeviceClass = NULL;
			PyTypeObject *pUnitClass = NULL;
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &pDeviceClass, &pUnitClass))
			{
				// Module import will not have finished so plugin pointer in module state will not have been initiialised
				pModState->pPlugin->Log(LOG_ERROR, "%s failed to parse parameters: Python class name expected.", __func__);
			}
			else
			{
				if (pDeviceClass)
				{
					if (!PyType_IsSubtype(pDeviceClass, pModState->pDeviceClass))
					{
						pModState->pPlugin->Log(LOG_ERROR, "Device class registration failed, Supplied class is not derived from 'DomoticzEx.Device'");
					}
					else
					{
						pModState->pDeviceClass = pDeviceClass;
						PyType_Ready(pModState->pDeviceClass);
					}
				}
				if (pUnitClass)
				{
					if (!PyType_IsSubtype(pUnitClass, pModState->pUnitClass))
					{
						pModState->pPlugin->Log(LOG_ERROR, "Unit class registration failed, Supplied class is not derived from 'DomoticzEx.Unit'");
					}
					else
					{
						pModState->pUnitClass = pUnitClass;
						PyType_Ready(pModState->pUnitClass);
					}
				}
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_Dump(PyObject *self, PyObject *args, PyObject *kwds)
	{
		static char *kwlist[] = { "Object", NULL };
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			Py_RETURN_NONE;
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
		}
		else
		{
			PyObject *pTarget = NULL; // Object reference count not increased
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &pTarget))
			{
				pModState->pPlugin->Log(LOG_ERROR, "%s failed to parse parameters: Object expected (Optional).", __func__);
				pModState->pPlugin->LogPythonException(std::string(__func__));
			}
			else
			{
				PyNewRef pLocals = PyObject_Dir(pModState->lastCallback);
				if (pLocals.IsList()) // && PyIter_Check(pLocals))  // Check fails but iteration works??!?
				{
					pModState->pPlugin->Log(LOG_NORM, "Context dump:");
					PyNewRef pIter = PyObject_GetIter(pLocals);
					PyNewRef pItem = PyIter_Next(pIter);
					while (pItem)
					{
						std::string sAttrName = pItem;
						if (sAttrName.substr(0, 2) != "__") // ignore system stuff
						{
							if (PyObject_HasAttrString(pModState->lastCallback, sAttrName.c_str()))
							{
								PyNewRef pValue = PyObject_GetAttrString(pModState->lastCallback, sAttrName.c_str());
								if (!PyCallable_Check(pValue)) // Filter out methods
								{
									std::string strValue = pValue;
									if (strValue.length())
									{
										std::string sBlank((sAttrName.length() < 20) ? 20 - sAttrName.length() : 0, ' ');
										pModState->pPlugin->Log(LOG_NORM, " ----> '%s'%s '%s'", sAttrName.c_str(), sBlank.c_str(), strValue.c_str());
									}
								}
							}
						}
						pItem = PyIter_Next(pIter);
					}
				}
				PyBorrowedRef pLocalVars = PyEval_GetLocals();
				if (pLocalVars.IsDict())
				{
					pModState->pPlugin->Log(LOG_NORM, "Locals dump:");
					PyBorrowedRef key;
					PyBorrowedRef value;
					Py_ssize_t pos = 0;
					while (PyDict_Next(pLocalVars, &pos, &key, &value))
					{
						std::string sValue = value;
						std::string sKey = key;
						std::string sBlank((sKey.length() < 20) ? 20 - sKey.length() : 0, ' ');
						pModState->pPlugin->Log(LOG_NORM, " ----> '%s'%s '%s'", sKey.c_str(), sBlank.c_str(), sValue.c_str());
					}
				}
				PyBorrowedRef pGlobalVars = PyEval_GetGlobals();
				if (pGlobalVars.IsDict())
				{
					pModState->pPlugin->Log(LOG_NORM, "Globals dump:");
					PyBorrowedRef key;
					PyBorrowedRef value;
					Py_ssize_t pos = 0;
					while (PyDict_Next(pGlobalVars, &pos, &key, &value))
					{
						std::string sKey = key;
						if ((sKey.substr(0, 2) != "__") && !PyCallable_Check(value)) // ignore system stuff and fucntions
						{
							std::string sBlank((sKey.length() < 20) ? 20 - sKey.length() : 0, ' ');
							std::string sValue = value;
							pModState->pPlugin->Log(LOG_NORM, " ----> '%s'%s '%s'", sKey.c_str(), sBlank.c_str(), sValue.c_str());
						}
					}
				}
			}
		}

		Py_RETURN_NONE;
	}

	static PyMethodDef DomoticzMethods[] = { { "Debug", PyDomoticz_Debug, METH_VARARGS, "Write a message to Domoticz log only if verbose logging is turned on." },
						 { "Log", PyDomoticz_Log, METH_VARARGS, "Write a message to Domoticz log." },
						 { "Status", PyDomoticz_Status, METH_VARARGS, "Write a status message to Domoticz log." },
						 { "Error", PyDomoticz_Error, METH_VARARGS, "Write an error message to Domoticz log." },
						 { "Debugging", PyDomoticz_Debugging, METH_VARARGS, "Set logging level. 1 set verbose logging, all other values use default level" },
						 { "Heartbeat", PyDomoticz_Heartbeat, METH_VARARGS, "Set the heartbeat interval, default 10 seconds." },
						 { "Notifier", PyDomoticz_Notifier, METH_VARARGS, "Enable notification handling with supplied name." },
						 { "Trace", PyDomoticz_Trace, METH_VARARGS, "Enable/Disable line level Python tracing." },
						 { "Configuration", (PyCFunction)PyDomoticz_Configuration, METH_VARARGS | METH_KEYWORDS, "Retrieve and Store structured plugin configuration." },
						 { "Register", (PyCFunction)PyDomoticz_Register, METH_VARARGS | METH_KEYWORDS, "Register Device override class." },
						 { "Dump", (PyCFunction)PyDomoticz_Dump, METH_VARARGS | METH_KEYWORDS, "Dump string values of an object or all locals to the log." },
						 { nullptr, nullptr, 0, nullptr } };

	PyType_Slot ConnectionSlots[] = {
		{ Py_tp_doc, (void*)"Domoticz Connection" },
		{ Py_tp_new, (void*)CConnection_new },
		{ Py_tp_init, (void*)CConnection_init },
		{ Py_tp_dealloc, (void*)CConnection_dealloc },
		{ Py_tp_members, CConnection_members },
		{ Py_tp_methods, CConnection_methods },
		{ Py_tp_str, (void*)CConnection_str },
		{ 0 },
	};
	PyType_Spec ConnectionSpec = { "Domoticz.Connection", sizeof(CConnection), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE, ConnectionSlots };

	PyType_Slot ImageSlots[] = {
		{ Py_tp_doc, (void*)"Domoticz Image" },
		{ Py_tp_new, (void*)CImage_new },
		{ Py_tp_init, (void*)CImage_init },
		{ Py_tp_dealloc, (void*)CImage_dealloc },
		{ Py_tp_members, CImage_members },
		{ Py_tp_methods, CImage_methods },
		{ Py_tp_str, (void*)CImage_str },
		{ 0 },
	};
	PyType_Spec ImageSpec = { "Domoticz.Image", sizeof(CImage), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE, ImageSlots };

	static int DomoticzTraverse(PyObject *m, visitproc visit, void *arg)
	{
		Py_VISIT(GETSTATE(m)->error);
		return 0;
	}

	static int DomoticzClear(PyObject *m)
	{
		Py_CLEAR(GETSTATE(m)->error);
		return 0;
	}

	struct PyModuleDef DomoticzModuleDef = { PyModuleDef_HEAD_INIT, "Domoticz", nullptr, sizeof(struct module_state), DomoticzMethods, nullptr, DomoticzTraverse, DomoticzClear, nullptr };

	PyMODINIT_FUNC PyInit_Domoticz(void)
	{
		// This is called during the import of the plugin module
		// triggered by the "import Domoticz" statement
		PyObject *pModule = PyModule_Create2(&DomoticzModuleDef, PYTHON_API_VERSION);
		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));

		if (!CDeviceType)
		{
			PyType_Slot DeviceSlots[] = {
				{ Py_tp_doc, (void*)"Domoticz Device" },
				{ Py_tp_new, (void*)CDevice_new },
				{ Py_tp_init, (void*)CDevice_init },
				{ Py_tp_dealloc, (void*)CDevice_dealloc },
				{ Py_tp_members, CDevice_members },
				{ Py_tp_methods, CDevice_methods },
				{ Py_tp_str, (void*)CDevice_str },
				{ 0 },
			};
			PyType_Spec DeviceSpec = { "Domoticz.Device", sizeof(CDevice), 0,
								  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE, DeviceSlots };

			CDeviceType = (PyTypeObject*)PyType_FromSpec(&DeviceSpec);
			PyType_Ready(CDeviceType);
		}
		pModState->pDeviceClass = CDeviceType;
		pModState->pUnitClass = nullptr;
		Py_INCREF(CDeviceType);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Device", (PyObject*)CDeviceType);

		if (!CConnectionType)
		{
			CConnectionType = (PyTypeObject*)PyType_FromSpec(&ConnectionSpec);
			PyType_Ready(CConnectionType);
		}
		Py_INCREF(CConnectionType);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Connection", (PyObject*)CConnectionType);

		if (!CImageType)
		{
			CImageType = (PyTypeObject*)PyType_FromSpec(&ImageSpec);
			PyType_Ready(CImageType);
		}
		PyObject* refTracker = (PyObject*)CImageType;
		Py_INCREF(CImageType);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Image", (PyObject*)CImageType);

		return pModule;
	}

	struct PyModuleDef DomoticzExModuleDef = { PyModuleDef_HEAD_INIT, "DomoticzEx", nullptr, sizeof(struct module_state), DomoticzMethods, nullptr, DomoticzTraverse, DomoticzClear, nullptr };

	PyMODINIT_FUNC PyInit_DomoticzEx(void)
	{
		// This is called during the import of the plugin module
		// triggered by the "import DomoticzEx" statement
		PyObject *pModule = PyModule_Create2(&DomoticzExModuleDef, PYTHON_API_VERSION);
		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));

		PyType_Slot DeviceExSlots[] = {
			{ Py_tp_doc, (void*)"DomoticzEx Device" },
			{ Py_tp_new, (void*)CDeviceEx_new },
			{ Py_tp_init, (void*)CDeviceEx_init },
			{ Py_tp_dealloc, (void*)CDeviceEx_dealloc },
			{ Py_tp_members, CDeviceEx_members },
			{ Py_tp_methods, CDeviceEx_methods },
			{ Py_tp_str, (void*)CDeviceEx_str },
			{ 0 },
		};
		PyType_Spec DeviceExSpec = { "DomoticzEx.Device", sizeof(CDeviceEx), 0,
							  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE, DeviceExSlots };

		pModState->pDeviceClass = (PyTypeObject*)PyType_FromSpec(&DeviceExSpec);	// Calls PyType_Ready internally from, 3.9 onwards
		Py_INCREF(pModState->pDeviceClass);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Device", (PyObject *)pModState->pDeviceClass);
		PyType_Ready(pModState->pDeviceClass);

		PyType_Slot UnitExSlots[] = {
			{ Py_tp_doc, (void*)"DomoticzEx Unit" },
			{ Py_tp_new, (void*)CUnitEx_new },
			{ Py_tp_init, (void*)CUnitEx_init },
			{ Py_tp_dealloc, (void*)CUnitEx_dealloc },
			{ Py_tp_members, CUnitEx_members },
			{ Py_tp_methods, CUnitEx_methods },
			{ Py_tp_str, (void*)CUnitEx_str },
			{ 0 },
		};
		PyType_Spec UnitExSpec = { "DomoticzEx.Unit", sizeof(CUnitEx), 0,
								Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE, UnitExSlots };

		pModState->pUnitClass = (PyTypeObject*)PyType_FromSpec(&UnitExSpec);
		Py_INCREF(pModState->pUnitClass);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Unit", (PyObject*)pModState->pUnitClass);
		PyType_Ready(pModState->pUnitClass);

		if (!CConnectionType)
		{
			CConnectionType = (PyTypeObject*)PyType_FromSpec(&ConnectionSpec);
			PyType_Ready(CConnectionType);
		}
		Py_INCREF(CConnectionType);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Connection", (PyObject*)CConnectionType);

		if (!CImageType)
		{
			CImageType = (PyTypeObject*)PyType_FromSpec(&ImageSpec);
			PyType_Ready(CImageType);
		}
		Py_INCREF(CImageType);	// PyModule_AddObject steals a reference
		PyModule_AddObject(pModule, "Image", (PyObject*)CImageType);

		return pModule;
	}

	CPlugin::CPlugin(const int HwdID, const std::string &sName, const std::string &sPluginKey)
		: m_iPollInterval(10)
		, m_PyInterpreter(nullptr)
		, m_PyModule(nullptr)
		, m_Notifier(nullptr)
		, m_PluginKey(sPluginKey)
		, m_DeviceDict(nullptr)
		, m_ImageDict(nullptr)
		, m_SettingsDict(nullptr)
		, m_bDebug(PDM_NONE)
	{
		m_HwdID = HwdID;
		m_Name = sName;
		m_bIsStarted = false;
		m_bIsStarting = false;
		m_bTracing = false;
	}

	CPlugin::~CPlugin()
	{
		m_bIsStarted = false;
	}

	module_state* CPlugin::FindModule()
	{
		// Domoticz potentially has only two possible modules and only one can be loaded
		PyBorrowedRef brModule = PyState_FindModule(&DomoticzModuleDef);
		PyBorrowedRef brModuleEx = PyState_FindModule(&DomoticzExModuleDef);

		// Check author has not loaded both Domoticz modules
		if ((brModule) && (brModuleEx))
		{
			_log.Log(LOG_ERROR, "(%s) Domoticz and DomoticzEx modules both found in interpreter, use one or the other.", __func__);
			return nullptr;
		}

		if (!brModule)
		{
			brModule = brModuleEx;
			if (!brModule)
			{
				_log.Log(LOG_ERROR, "(%s) Domoticz/DomoticzEx modules not found in interpreter.", __func__);
				return nullptr;
			}
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(brModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "%s, unable to obtain module state.", __func__);
		}

		return pModState;
	}

	CPlugin *CPlugin::FindPlugin()
	{
		module_state *pModState = FindModule();
		return pModState ? pModState->pPlugin : nullptr;
	}

	void CPlugin::LogPythonException()
	{
		PyNewRef	pTraceback;
		PyNewRef	pExcept;
		PyNewRef	pValue;

		PyErr_Fetch(&pExcept, &pValue, &pTraceback);
		PyErr_NormalizeException(&pExcept, &pValue, &pTraceback);

		if (!pExcept && !pValue && !pTraceback)
		{
			Log(LOG_ERROR, "Unable to decode exception.");
		}
		else
		{
			std::string	sTypeText("Unknown Error");

			/* See if we can get a full traceback */
			PyNewRef	pModule = PyImport_ImportModule("traceback");
			if (pModule)
			{
				PyNewRef	pFunc = PyObject_GetAttrString(pModule, "format_exception");
				if (pFunc && PyCallable_Check(pFunc)) {
					PyNewRef	pList = PyObject_CallFunctionObjArgs(pFunc, (PyObject*)pExcept, (PyObject*)pValue, (PyObject*)pTraceback, NULL);
					if (pList)
					{
						for (Py_ssize_t i = 0; i < PyList_Size(pList); i++)
						{
							PyBorrowedRef	pPyStr = PyList_GetItem(pList, i);
							std::string		pStr(pPyStr);
							size_t pos = 0;
							std::string token;
							while ((pos = pStr.find('\n')) != std::string::npos) {
								token = pStr.substr(0, pos);
								Log(LOG_ERROR, "%s", token.c_str());
								pStr.erase(0, pos + 1);
							}
						}
					}
					else
					{
						if (pExcept) sTypeText = pExcept.Attribute("__name__");
						Log(LOG_ERROR, "Exception: '%s'.  No traceback available.", sTypeText.c_str());
					}
				}
				else
				{
					if (pExcept) sTypeText = pExcept.Attribute("__name__");
					Log(LOG_ERROR, "'format_exception' lookup failed, exception: '%s'.  No traceback available.", sTypeText.c_str());
				}
			}
			else
			{
				if (pExcept) sTypeText = pExcept.Attribute("__name__");
				Log(LOG_ERROR, "'Traceback' module import failed, exception: '%s'.  No traceback available.", sTypeText.c_str());
			}
		}
		PyErr_Clear();
	}

	void CPlugin::LogPythonException(const std::string &sHandler)
	{
		Log(LOG_ERROR, "Call to function '%s' failed, exception details:", sHandler.c_str());
		LogPythonException();
	}

	int CPlugin::PollInterval(int Interval)
	{
		if (Interval > 0)
		{
			m_iPollInterval = Interval;
			if (m_bDebug & PDM_PLUGIN)
				Log(LOG_NORM, "Heartbeat interval set to: %d.", m_iPollInterval);
		}
		return m_iPollInterval;
	}

	void CPlugin::Notifier(const std::string &Notifier)
	{
		delete m_Notifier;
		m_Notifier = nullptr;
		if (m_bDebug & PDM_PLUGIN)
			Log(LOG_NORM, "Notifier Name set to: %s.", Notifier.c_str());
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
		for (auto itt = m_Transports.begin(); itt != m_Transports.end(); itt++)
		{
			CPluginTransport *pPluginTransport = *itt;
			if (pTransport == pPluginTransport)
			{
				m_Transports.erase(itt);
				break;
			}
		}
	}

	bool CPlugin::StartHardware()
	{
		if (m_bIsStarted)
			StopHardware();

		RequestStart();

		// Flush the message queue (should already be empty)
		{
			std::lock_guard<std::mutex> l(m_QueueMutex);
			while (!m_MessageQueue.empty())
			{
				m_MessageQueue.pop_front();
			}
		}

		// Start worker thread
		try
		{
			std::lock_guard<std::mutex> l(m_QueueMutex);
			m_thread = std::make_shared<std::thread>(&CPlugin::Do_Work, this);
			if (!m_thread)
			{
				Log(LOG_ERROR, "Failed start interface worker thread.");
			}
			else
			{
				SetThreadName(m_thread->native_handle(), m_Name.c_str());
				Log(LOG_NORM, "Worker thread started.");
			}
		}
		catch (...)
		{
			Log(LOG_ERROR, "Exception caught in '%s'.", __func__);
		}

		//	Add start command to message queue
		m_bIsStarting = true;
		MessagePlugin(new InitializeMessage());

		Log(LOG_STATUS, "Started.");

		return true;
	}

	bool CPlugin::StopHardware()
	{
		try
		{
			Log(LOG_STATUS, "Stop directive received.");

			// loop on plugin to finish startup
			while (m_bIsStarting)
			{
				sleep_milliseconds(100);
			}

			RequestStop();

			if (m_bIsStarted)
			{
				// If we have connections queue disconnects
				if (!m_Transports.empty())
				{
					AccessPython	Guard(this, m_Name.c_str());
					std::lock_guard<std::mutex> lTransports(m_TransportsMutex);
					for (const auto &pPluginTransport : m_Transports)
					{
						// Tell transport to disconnect if required
						if (pPluginTransport)
						{
							MessagePlugin(new DisconnectDirective(pPluginTransport->Connection()));
						}
					}
				}
				else
				{
					// otherwise just signal stop
					MessagePlugin(new onStopCallback());
				}

				// loop on stop to be processed
				while (m_bIsStarted)
				{
					sleep_milliseconds(100);
				}
			}

			Log(LOG_STATUS, "Stopping threads.");

			if (m_thread)
			{
				m_thread->join();
				m_thread.reset();
			}

			if (m_Notifier)
			{
				delete m_Notifier;
				m_Notifier = nullptr;
			}

			// If plugin failed to start onStop won't be called so force a cleanup here
			if (m_PyInterpreter) {
				AccessPython	Guard(this, m_Name.c_str());
				Stop();
			}
		}
		catch (...)
		{
			// Don't throw from a Stop command
		}

		Log(LOG_STATUS, "Stopped.");

		return true;
	}

	void CPlugin::Do_Work()
	{
		Log(LOG_STATUS, "Entering work loop.");
		m_LastHeartbeat = mytime(nullptr);
		while (!IsStopRequested(50) || !m_bIsStopped)
		{
			time_t Now = time(nullptr);
			bool bProcessed = true;
			while (bProcessed)
			{
				CPluginMessageBase *Message = nullptr;
				bProcessed = false;

				// Cycle once through the queue looking for the 1st message that is ready to process
				{
					std::lock_guard<std::mutex> l(m_QueueMutex);
					for (size_t i = 0; i < m_MessageQueue.size(); i++)
					{
						CPluginMessageBase *FrontMessage = m_MessageQueue.front();
						m_MessageQueue.pop_front();
						if (!FrontMessage->m_Delay || FrontMessage->m_When <= Now)
						{
							// Message is ready now or was already ready (this is the case for almost all messages)
							Message = FrontMessage;
							break;
						}
						// Message is for sometime in the future so requeue it (this happens when the 'Delay' parameter is used on a Send)
						m_MessageQueue.push_back(FrontMessage);
					}
				}

				if (Message)
				{
					bProcessed = true;
					try
					{
						if (m_bDebug & PDM_QUEUE)
						{
							Log(LOG_NORM, "Processing '" + std::string(Message->Name()) + "' message");
						}
						Message->Process(this);
					}
					catch (...)
					{
						Log(LOG_ERROR, "Exception processing '%s' message.", Message->Name());
					}

					// Free the memory for the message
					if (!m_PyInterpreter)
					{
						// Can't lock because there is no interpreter to lock
						delete Message;
					}
					else
					{
						AccessPython	Guard(this, Message->Name());
						delete Message;
					}
				}
			}

			if (Now >= (m_LastHeartbeat + m_iPollInterval))
			{
				//	Add heartbeat to message queue
				MessagePlugin(new onHeartbeatCallback());
				m_LastHeartbeat = mytime(nullptr);
			}

			// Check all connections are still valid, vector could be affected by a disconnect on another thread
			try
			{
				std::lock_guard<std::mutex> lTransports(m_TransportsMutex);
				if (!m_Transports.empty())
				{
					for (const auto &pPluginTransport : m_Transports)
					{
						pPluginTransport->VerifyConnection();
					}
				}
			}
			catch (...)
			{
				Log(LOG_NORM, "Transport vector changed during %s loop, continuing.", __func__);
			}
		}

		Log(LOG_STATUS, "Exiting work loop.");
	}

	bool CPlugin::Initialise()
	{
		m_bIsStarted = false;

		try
		{
			// Only initialise one plugin at a time to prevent issues with module creation
			PyEval_RestoreThread((PyThreadState *)m_mainworker.m_pluginsystem.PythonThread());
			m_PyInterpreter = Py_NewInterpreter();
			if (!m_PyInterpreter)
			{
				Log(LOG_ERROR, "(%s) failed to create interpreter.", m_PluginKey.c_str());
				goto Error;
			}

			// Prepend plugin directory to path so that python will search it early when importing
#ifdef WIN32
			std::wstring sSeparator = L";";
#else
			std::wstring sSeparator = L":";
#endif
			std::wstringstream ssPath;
			std::string sFind = "key=\"" + m_PluginKey + "\"";
			CPluginSystem Plugins;
			std::map<std::string, std::string> *mPluginXml = Plugins.GetManifest();
			std::string sPluginXML;
			for (const auto &type : *mPluginXml)
			{
				if (type.second.find(sFind) != std::string::npos)
				{
					m_HomeFolder = type.first;
					ssPath << m_HomeFolder.c_str();
					sPluginXML = type.second;
					break;
				}
			}

			std::wstring sPath = ssPath.str() + sSeparator;
			sPath += Py_GetPath();

			try
			{
				//
				//	Python loads the 'site' module automatically and adds extra search directories for module loading
				//	This code makes the plugin framework function the same way
				//
				PyNewRef	pSiteModule = PyImport_ImportModule("site");
				if (!pSiteModule)
				{
					Log(LOG_ERROR, "(%s) failed to load 'site' module, continuing.", m_PluginKey.c_str());
				}
				else
				{
					PyNewRef	pFunc = PyObject_GetAttrString((PyObject *)pSiteModule, "getsitepackages");
					if (pFunc && PyCallable_Check(pFunc))
					{
						PyNewRef	pSites = PyObject_CallObject(pFunc, nullptr);
						if (!pSites)
						{
							LogPythonException("getsitepackages");
						}
						else
							for (Py_ssize_t i = 0; i < PyList_Size(pSites); i++)
							{
								PyBorrowedRef	pSite = PyList_GetItem(pSites, i);
								if (pSite.IsString())
								{
									std::wstringstream ssPath;
									ssPath << ((std::string)PyBorrowedRef(pSite)).c_str();
									sPath += sSeparator + ssPath.str();
								}
							}
					}
				}
			}
			catch (...)
			{
				Log(LOG_ERROR, "(%s) exception loading 'site' module, continuing.", m_PluginKey.c_str());
				PyErr_Clear();
			}

			// Update the path itself
			PySys_SetPath((wchar_t *)sPath.c_str());

			// Get reference to global 'Py_None' instance for comparisons
			if (!Py_None)
			{
				PyNewRef		global_dict = PyDict_New();
				PyNewRef		local_dict = PyDict_New();
				PyNewRef		pCode = Py_CompileString("# Eval will return 'None'\n", "<domoticz>", Py_file_input);
				if (pCode)
				{
					PyNewRef	pEval = PyEval_EvalCode(pCode, global_dict, local_dict);
					Py_None = pEval;
					Py_INCREF(Py_None);
				}
				else
				{
					Log(LOG_ERROR, "Failed to compile script to set global Py_None");
				}
			}

			try
			{
				//
				//	Load the 'faulthandler' module to get a python stackdump during a segfault
				//
				PyNewRef	pFaultModule = PyImport_ImportModule("faulthandler");
				if (!pFaultModule)
				{
					Log(LOG_ERROR, "(%s) failed to load 'faulthandler' module, continuing.", m_PluginKey.c_str());
				}
				else
				{
					PyNewRef	pFunc = PyObject_GetAttrString((PyObject*)pFaultModule, "is_enabled");
					if (pFunc && PyCallable_Check(pFunc))
					{
						PyNewRef	pRetObj = PyObject_CallObject(pFunc, nullptr);
						if (!pRetObj.IsTrue())
						{
							PyNewRef	pFunc = PyObject_GetAttrString((PyObject*)pFaultModule, "enable");
							if (pFunc && PyCallable_Check(pFunc))
							{
								PyNewRef pRetObj = PyObject_CallObject(pFunc, nullptr);
							}
						}
					}
				}
			}
			catch (...)
			{
				Log(LOG_ERROR, "(%s) exception loading 'faulthandler' module, continuing.", m_PluginKey.c_str());
				PyErr_Clear();
			}

			try
			{
				m_PyModule = PyImport_ImportModule("plugin");
				if (!m_PyModule)
				{
					Log(LOG_ERROR, "(%s) failed to load 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
					LogPythonException();
					goto Error;
				}
			}
			catch (...)
			{
				Log(LOG_ERROR, "(%s) exception loading 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
				PyErr_Clear();
			}

			module_state *pModState = FindModule();
			if (!pModState)
			{
				Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", __func__);
				goto Error;
			}
			pModState->pPlugin = this;

			//	Add start command to message queue
			MessagePlugin(new onStartCallback());

			std::string sExtraDetail;
			TiXmlDocument XmlDoc;
			XmlDoc.Parse(sPluginXML.c_str());
			if (XmlDoc.Error())
			{
				Log(LOG_ERROR, "%s: Error '%s' at line %d column %d in XML '%s'.", __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), sPluginXML.c_str());
			}
			else
			{
				TiXmlNode *pXmlNode = XmlDoc.FirstChild("plugin");
				for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
				{
					TiXmlElement *pXmlEle = pXmlNode->ToElement();
					if (pXmlEle)
					{
						const char *pAttributeValue = pXmlEle->Attribute("version");
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
							if (!sExtraDetail.empty())
								sExtraDetail += ", ";
							sExtraDetail += "author '";
							sExtraDetail += pAttributeValue;
							sExtraDetail += "'";
						}
					}
				}
			}
			Log(LOG_STATUS, "Initialized %s", sExtraDetail.c_str());

			PyEval_SaveThread();
			return true;
		}
		catch (...)
		{
			Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
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
			PyBorrowedRef pModuleDict = PyModule_GetDict(PythonModule()); // returns a borrowed referece to the __dict__ object for the module
			PyNewRef		pParamsDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Parameters", pParamsDict) == -1)
			{
				Log(LOG_ERROR, "(%s) failed to add Parameters dictionary.", m_PluginKey.c_str());
				goto Error;
			}

			PyNewRef pHwdID(m_HwdID);
			if (PyDict_SetItemString(pParamsDict, "HardwareID", pHwdID) == -1)
			{
				Log(LOG_ERROR, "(%s) failed to add key 'HardwareID', value '%d' to dictionary.", m_PluginKey.c_str(), m_HwdID);
				goto Error;
			}

			std::string sLanguage = "en";
			m_sql.GetPreferencesVar("Language", sLanguage);

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Name, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID==%d)", m_HwdID);
			if (!result.empty())
			{
				for (const auto &sd : result)
				{
					ADD_STRING_TO_DICT(this, pParamsDict, "HomeFolder", m_HomeFolder);
					ADD_STRING_TO_DICT(this, pParamsDict, "StartupFolder", szStartupFolder);
					ADD_STRING_TO_DICT(this, pParamsDict, "UserDataFolder", szUserDataFolder);
					ADD_STRING_TO_DICT(this, pParamsDict, "WebRoot", szWebRoot);
					ADD_STRING_TO_DICT(this, pParamsDict, "Database", dbasefile);
					ADD_STRING_TO_DICT(this, pParamsDict, "Language", sLanguage);
					ADD_STRING_TO_DICT(this, pParamsDict, "Version", m_Version);
					ADD_STRING_TO_DICT(this, pParamsDict, "Author", m_Author);
					ADD_STRING_TO_DICT(this, pParamsDict, "Name", sd[0]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Address", sd[1]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Port", sd[2]);
					ADD_STRING_TO_DICT(this, pParamsDict, "SerialPort", sd[3]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Username", sd[4]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Password", sd[5]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Key", sd[6]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode1", sd[7]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode2", sd[8]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode3", sd[9]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode4", sd[10]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode5", sd[11]);
					ADD_STRING_TO_DICT(this, pParamsDict, "Mode6", sd[12]);

					ADD_STRING_TO_DICT(this, pParamsDict, "DomoticzVersion", szAppVersion);
					ADD_STRING_TO_DICT(this, pParamsDict, "DomoticzHash", szAppHash);
					ADD_STRING_TO_DICT(this, pParamsDict, "DomoticzBuildTime", szAppDate);
				}
			}

			m_DeviceDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Devices", (PyObject *)m_DeviceDict) == -1)
			{
				Log(LOG_ERROR, "(%s) failed to add Device dictionary.", m_PluginKey.c_str());
				goto Error;
			}

			std::string tupleStr = "(si)";
			PyBorrowedRef brModule = PyState_FindModule(&DomoticzModuleDef);

			if (brModule)
			{
				result = m_sql.safe_query("SELECT '', Unit FROM DeviceStatus WHERE (HardwareID==%d) ORDER BY Unit ASC", m_HwdID);
			}
			else
			{
				brModule = PyState_FindModule(&DomoticzExModuleDef);
				if (!brModule)
				{
					Log(LOG_ERROR, "(%s) %s failed, Domoticz/DomoticzEx modules not found in interpreter.", __func__, m_PluginKey.c_str());
					goto Error;
				}
				result = m_sql.safe_query("SELECT DISTINCT DeviceID, '-1' FROM DeviceStatus WHERE (HardwareID==%d) ORDER BY Unit ASC", m_HwdID);
				tupleStr = "(s)";
			}

			module_state *pModState = ((struct module_state *)PyModule_GetState(brModule));
			if (!pModState)
			{
				Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", __func__);
				goto Error;
			}

			// load associated devices to make them available to python
			if (!result.empty())
			{
				// Add device objects into the device dictionary with Unit as the key
				for (const auto &sd : result)
				{
					// Build argument list
					PyNewRef nrArgList = Py_BuildValue(tupleStr.c_str(), sd[0].c_str(), atoi(sd[1].c_str()));
					if (!nrArgList)
					{
						Log(LOG_ERROR, "Building device argument list failed for key %s/%s.", sd[0].c_str(), sd[1].c_str());
						goto Error;
					}
					PyNewRef pDevice = PyObject_CallObject((PyObject *)pModState->pDeviceClass, nrArgList);
					if (!pDevice)
					{
						Log(LOG_ERROR, "Device object creation failed for key %s/%s.", sd[0].c_str(), sd[1].c_str());
						goto Error;
					}

					// Add the object to the dictionary
					PyNewRef pKey = PyObject_GetAttrString(pDevice, "Key");
					if (!PyDict_Contains((PyObject*)m_DeviceDict, pKey))
					{
						if (PyDict_SetItem((PyObject*)m_DeviceDict, pKey, pDevice) == -1)
						{
							Log(LOG_ERROR, "(%s) failed to add key '%s' to device dictionary.", m_PluginKey.c_str(), std::string(pKey).c_str());
							goto Error;
						}
					}

					// Force the object to refresh from the database
					PyNewRef	pRefresh = PyObject_GetAttrString(pDevice, "Refresh");
					if (pRefresh && PyCallable_Check(pRefresh))
					{
						PyNewRef pReturnValue = PyObject_CallObject(pRefresh, NULL);
					}
					else
					{
						pModState->pPlugin->Log(LOG_ERROR, "Failed to refresh object '%s', method missing or not callable.", std::string(pKey).c_str());
					}
				}
			}

			m_ImageDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Images", (PyObject *)m_ImageDict) == -1)
			{
				Log(LOG_ERROR, "(%s) failed to add Image dictionary.", m_PluginKey.c_str());
				goto Error;
			}

			// load associated custom images to make them available to python
			result = m_sql.safe_query("SELECT ID, Base, Name, Description FROM CustomImages WHERE Base LIKE '%q%%' ORDER BY ID ASC", m_PluginKey.c_str());
			if (!result.empty())
			{
				// Add image objects into the image dictionary with ID as the key
				for (const auto &sd : result)
				{
					CImage *pImage = (CImage *)CImage_new(CImageType, (PyObject *)nullptr, (PyObject *)nullptr);

					PyNewRef	pKey = PyUnicode_FromString(sd[1].c_str());
					if (PyDict_SetItem((PyObject *)m_ImageDict, pKey, (PyObject *)pImage) == -1)
					{
						Log(LOG_ERROR, "(%s) failed to add ID '%s' to image dictionary.", m_PluginKey.c_str(), sd[0].c_str());
						goto Error;
					}
					pImage->ImageID = atoi(sd[0].c_str()) + 100;
					pImage->Base = PyUnicode_FromString(sd[1].c_str());
					pImage->Name = PyUnicode_FromString(sd[2].c_str());
					pImage->Description = PyUnicode_FromString(sd[3].c_str());
					Py_DECREF(pImage);
				}
			}

			LoadSettings();

			m_bIsStarted = true;
			m_bIsStarting = false;
			m_bIsStopped = false;
			return true;
		}
		catch (...)
		{
			Log(LOG_ERROR, "(%s) exception caught in '%s'.", m_PluginKey.c_str(), __func__);
		}

	Error:
		m_bIsStarting = false;
		return false;
	}

	void CPlugin::ConnectionProtocol(CDirectiveBase *pMess)
	{
		ProtocolDirective *pMessage = (ProtocolDirective *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;
		if (m_Notifier)
		{
			delete pConnection->pProtocol;
			pConnection->pProtocol = nullptr;
		}
		std::string sProtocol = PyBorrowedRef(pConnection->Protocol);
		pConnection->pProtocol = CPluginProtocol::Create(sProtocol);
		if (m_bDebug & PDM_CONNECTION)
			Log(LOG_NORM, "Protocol set to: '%s'.", sProtocol.c_str());
	}

	void CPlugin::ConnectionConnect(CDirectiveBase *pMess)
	{
		ConnectDirective *pMessage = (ConnectDirective *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;

		if (!pConnection->pPlugin) pConnection->pPlugin = this;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			Log(LOG_ERROR, "Current transport is still connected, directive ignored.");
			return;
		}

		if (!pConnection->pProtocol)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string sConnection = PyBorrowedRef(pConnection->Name);
				Log(LOG_NORM, "Protocol for '%s' not specified, 'None' assumed.", sConnection.c_str());
			}
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::string sTransport = PyBorrowedRef(pConnection->Transport);
		std::string sAddress = PyBorrowedRef(pConnection->Address);
		if ((sTransport == "TCP/IP") || (sTransport == "TLS/IP"))
		{
			std::string sPort = PyBorrowedRef(pConnection->Port);
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Transport set to: '%s', %s:%s.", sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			if (sPort.empty())
			{
				Log(LOG_ERROR, "No port number specified for %s connection to: '%s'.", sTransport.c_str(), sAddress.c_str());
				return;
			}
			if ((sTransport == "TLS/IP") || pConnection->pProtocol->Secure())
				pConnection->pTransport = (CPluginTransport *)new CPluginTransportTCPSecure(m_HwdID, pConnection, sAddress, sPort);
			else
				pConnection->pTransport = (CPluginTransport *)new CPluginTransportTCP(m_HwdID, pConnection, sAddress, sPort);
		}
		else if (sTransport == "Serial")
		{
			if (pConnection->pProtocol->Secure())
				Log(LOG_ERROR, "Transport '%s' does not support secure connections.", sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Transport set to: '%s', '%s', %d.", sTransport.c_str(), sAddress.c_str(), pConnection->Baud);
			pConnection->pTransport = (CPluginTransport *)new CPluginTransportSerial(m_HwdID, pConnection, sAddress, pConnection->Baud);
		}
		else
		{
			Log(LOG_ERROR, "Invalid transport type for connecting specified: '%s', valid types are TCP/IP and Serial.", sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleConnect())
		{
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Connect directive received, action initiated successfully.");
		}
		else
		{
			Log(LOG_NORM, "Connect directive received, action initiation failed.");
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionListen(CDirectiveBase *pMess)
	{
		ListenDirective *pMessage = (ListenDirective *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;

		if (!pConnection->pPlugin) pConnection->pPlugin = this;

		if (pConnection->pTransport && pConnection->pTransport->IsConnected())
		{
			Log(LOG_ERROR, "Current transport is still connected, directive ignored.");
			return;
		}

		if (!pConnection->pProtocol)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string sConnection = PyBorrowedRef(pConnection->Name);
				Log(LOG_NORM, "Protocol for '%s' not specified, 'None' assumed.", sConnection.c_str());
			}
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::string sTransport = PyBorrowedRef(pConnection->Transport);
		std::string sAddress = PyBorrowedRef(pConnection->Address);
		if (sTransport == "TCP/IP")
		{
			std::string sPort = PyBorrowedRef(pConnection->Port);
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Transport set to: '%s', %s:%s.", sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			if (!pConnection->pProtocol->Secure())
				pConnection->pTransport = (CPluginTransport *)new CPluginTransportTCP(m_HwdID, pConnection, "", sPort);
			else
				pConnection->pTransport = (CPluginTransport *)new CPluginTransportTCPSecure(m_HwdID, pConnection, "", sPort);
		}
		else if (sTransport == "UDP/IP")
		{
			std::string sPort = PyBorrowedRef(pConnection->Port);
			if (pConnection->pProtocol->Secure())
				Log(LOG_ERROR, "Transport '%s' does not support secure connections.", sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Transport set to: '%s', %s:%s.", sTransport.c_str(), sAddress.c_str(), sPort.c_str());
			pConnection->pTransport = (CPluginTransport *)new CPluginTransportUDP(m_HwdID, pConnection, sAddress, sPort);
		}
		else if (sTransport == "ICMP/IP")
		{
			std::string sPort = PyBorrowedRef(pConnection->Port);
			if (pConnection->pProtocol->Secure())
				Log(LOG_ERROR, "Transport '%s' does not support secure connections.", sTransport.c_str());
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Transport set to: '%s', %s.", sTransport.c_str(), sAddress.c_str());
			pConnection->pTransport = (CPluginTransport *)new CPluginTransportICMP(m_HwdID, pConnection, sAddress, sPort);
		}
		else
		{
			Log(LOG_ERROR, "Invalid transport type for listening specified: '%s', valid types are TCP/IP, UDP/IP and ICMP/IP.", sTransport.c_str());
			return;
		}
		if (pConnection->pTransport)
		{
			AddConnection(pConnection->pTransport);
		}
		if (pConnection->pTransport->handleListen())
		{
			if (m_bDebug & PDM_CONNECTION)
				Log(LOG_NORM, "Listen directive received, action initiated successfully.");
		}
		else
		{
			Log(LOG_NORM, "Listen directive received, action initiation failed.");
			RemoveConnection(pConnection->pTransport);
		}
	}

	void CPlugin::ConnectionRead(CPluginMessageBase *pMess)
	{
		ReadEvent *pMessage = (ReadEvent *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;

		pConnection->pProtocol->ProcessInbound(pMessage);

		if (PyErr_Occurred())
		{
			LogPythonException("ProcessInbound");
		}
	}

	void CPlugin::ConnectionWrite(CDirectiveBase *pMess)
	{
		WriteDirective *pMessage = (WriteDirective *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;
		std::string sTransport = PyBorrowedRef(pConnection->Transport);
		std::string sConnection = PyBorrowedRef(pConnection->Name);
		if (pConnection->pTransport)
		{
			if (sTransport == "UDP/IP")
			{
				Log(LOG_ERROR, "Connectionless Transport is listening, write directive to '%s' ignored.", sConnection.c_str());
				return;
			}

			if ((sTransport != "ICMP/IP") && (!pConnection->pTransport->IsConnected()))
			{
				Log(LOG_ERROR, "Transport is not connected, write directive to '%s' ignored.", sConnection.c_str());
				return;
			}
		}

		if (!pConnection->pTransport)
		{
			// UDP is connectionless so create a temporary transport and write to it
			if (sTransport == "UDP/IP")
			{
				std::string sAddress = PyBorrowedRef(pConnection->Address);
				std::string sPort = PyBorrowedRef(pConnection->Port);
				if (m_bDebug & PDM_CONNECTION)
				{
					if (!sPort.empty())
						Log(LOG_NORM, "Transport set to: '%s', %s:%s for '%s'.", sTransport.c_str(), sAddress.c_str(), sPort.c_str(),
							 sConnection.c_str());
					else
						Log(LOG_NORM, "Transport set to: '%s', %s for '%s'.", sTransport.c_str(), sAddress.c_str(), sConnection.c_str());
				}
				pConnection->pTransport = (CPluginTransport *)new CPluginTransportUDP(m_HwdID, pConnection, sAddress, sPort);
			}
			else
			{
				Log(LOG_ERROR, "No transport, write directive to '%s' ignored.", sConnection.c_str());
				return;
			}
		}

		// Make sure there is a protocol to encode the data
		if (!pConnection->pProtocol)
		{
			pConnection->pProtocol = new CPluginProtocol();
		}

		std::vector<byte> vWriteData = pConnection->pProtocol->ProcessOutbound(pMessage);
		WriteDebugBuffer(vWriteData, false);

		pConnection->pTransport->handleWrite(vWriteData);

		// UDP is connectionless so remove the transport after write
		if (pConnection->pTransport && (sTransport == "UDP/IP"))
		{
			delete pConnection->pTransport;
			pConnection->pTransport = nullptr;
		}
	}

	void CPlugin::ConnectionDisconnect(CDirectiveBase *pMess)
	{
		DisconnectDirective *pMessage = (DisconnectDirective *)pMess;
		CConnection *pConnection = pMessage->m_pConnection;

		// Return any partial data to plugin
		if (pConnection->pProtocol)
		{
			pConnection->pProtocol->Flush(this, pConnection);
		}

		if (pConnection->pTransport)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string sTransport = PyBorrowedRef(pConnection->Transport);
				std::string sAddress = PyBorrowedRef(pConnection->Address);
				std::string sPort = PyBorrowedRef(pConnection->Port);
				if ((sTransport == "Serial") || (sPort.empty()))
					Log(LOG_NORM, "Disconnect directive received for '%s'.", sAddress.c_str());
				else
					Log(LOG_NORM, "Disconnect directive received for '%s:%s'.", sAddress.c_str(), sPort.c_str());
			}

			// Sanity check the directive

			// If transport is not going to disconnect asynchronously tidy it up here
			if (!pConnection->pTransport->AsyncDisconnect())
			{
				pConnection->pTransport->handleDisconnect();
				RemoveConnection(pConnection->pTransport);
				delete pConnection->pTransport;
				pConnection->pTransport = nullptr;

				// Plugin exiting and all connections have disconnect messages queued
				if (IsStopRequested(0) && m_Transports.empty())
				{
					MessagePlugin(new onStopCallback());
				}
			}
			else
			{
				pConnection->pTransport->handleDisconnect();
			}
		}
	}

	void CPlugin::onDeviceAdded(const std::string DeviceID, int Unit)
	{
		PyBorrowedRef pObject;
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (pModule)
		{
			module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "(%s) unable to obtain module state.", __func__);
				return;
			}

			if (!pModState->pPlugin)
			{
				PyBorrowedRef pyDevice = FindDevice(DeviceID);
				if (!pyDevice)
				{
					// Create the device object if not found
					PyNewRef nrArgList = Py_BuildValue("(s)", DeviceID.c_str());
					if (!nrArgList)
					{
						Log(LOG_ERROR, "Building device argument list failed for key %s.", DeviceID.c_str());
						return;
					}
					PyNewRef pDevice = PyObject_CallObject((PyObject *)pModState->pDeviceClass, nrArgList);
					if (!pDevice)
					{
						Log(LOG_ERROR, "Device object creation failed for key %s.", DeviceID.c_str());
						return;
					}

					// Add the object to the dictionary
					PyNewRef pKey = PyObject_GetAttrString(pDevice, "Key");
					if (!PyDict_Contains((PyObject *)m_DeviceDict, pKey))
					{
						if (PyDict_SetItem((PyObject *)m_DeviceDict, pKey, pDevice) == -1)
						{
							Log(LOG_ERROR, "Failed to add key '%s' to device dictionary.", std::string(pKey).c_str());
							return;
						}
					}

					// now find it
					pyDevice = FindDevice(DeviceID);
				}

				// Create unit object
				PyNewRef nrArgList = Py_BuildValue("(ssi)", "", DeviceID.c_str(), Unit);
				if (!nrArgList)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Building device argument list failed for key %s/%d.", DeviceID.c_str(), Unit);
					return;
				}
				PyNewRef pUnit = PyObject_CallObject((PyObject *)pModState->pUnitClass, nrArgList);
				if (!pUnit)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Unit object creation failed for key %d.", Unit);
					return;
				}

				// and add it to the parent directory
				CDeviceEx *pDevice = pyDevice;
				PyNewRef pKey = PyLong_FromLong(Unit);
				if (PyDict_SetItem((PyObject *)pDevice->m_UnitDict, pKey, pUnit) == -1)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Failed to add key '%s' to Unit dictionary.", std::string(pKey).c_str());
					return;
				}

				// Force the Unit object to refresh from the database
				PyNewRef pRefresh = PyObject_GetAttrString(pUnit, "Refresh");
				if (pRefresh && PyCallable_Check(pRefresh))
				{
					PyNewRef pReturnValue = PyObject_CallObject(pRefresh, NULL);
				}
				else
				{
					pModState->pPlugin->Log(LOG_ERROR, "Failed to refresh object '%s', method missing or not callable.", std::string(pKey).c_str());
				}
			}
		}
		else
		{
			CDevice *pDevice = (CDevice *)CDevice_new(CDeviceType, (PyObject *)nullptr, (PyObject *)nullptr);

			PyNewRef pKey = PyLong_FromLong(Unit);
			if (PyDict_SetItem((PyObject *)m_DeviceDict, pKey, (PyObject *)pDevice) == -1)
			{
				Log(LOG_ERROR, "Failed to add unit '%d' to device dictionary.", Unit);
				return;
			}
			pDevice->pPlugin = this;
			pDevice->PluginKey = PyUnicode_FromString(m_PluginKey.c_str());
			pDevice->HwdID = m_HwdID;
			pDevice->Unit = Unit;
			CDevice_refresh(pDevice);
			Py_DECREF(pDevice);
		}
	}

	void CPlugin::onDeviceModified(const std::string DeviceID, int Unit)
	{
		PyBorrowedRef pObject;
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (pModule)
		{
			pObject = FindUnitInDevice(DeviceID, Unit);
		}
		else
		{
			PyNewRef pKey = PyLong_FromLong(Unit);
			pObject = PyDict_GetItem((PyObject *)m_DeviceDict, pKey);
		}

		if (!pObject)
		{
			Log(LOG_ERROR, "Failed to refresh unit '%u' in device dictionary.", Unit);
			return;
		}

		// Force the object to refresh from the database
		if (PyObject_HasAttrString(pObject, "Refresh"))
		{
			PyNewRef pRefresh = PyObject_GetAttrString(pObject, "Refresh");
			if (pRefresh && PyCallable_Check(pRefresh))
			{
				PyNewRef pReturnValue = PyObject_CallObject(pRefresh, NULL);
			}
			else
			{
				Log(LOG_ERROR, "Failed to refresh object '%s', method missing or not callable.", DeviceID.c_str());
			}
		}
	}

	void CPlugin::onDeviceRemoved(const std::string DeviceID, int Unit)
	{
		PyNewRef pKey = PyLong_FromLong(Unit);
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (pModule)
		{
			PyBorrowedRef pObject = FindDevice(DeviceID.c_str());
			if (pObject)
			{
				CDeviceEx *pDevice = (CDeviceEx *)pObject;
				if (PyDict_DelItem((PyObject *)pDevice->m_UnitDict, pKey) == -1)
				{
					Log(LOG_ERROR, "Failed to remove Unit '%u' from Unit dictionary of '%s'.", Unit, DeviceID.c_str());
				}
			}
		}
		else
		{
			if (PyDict_DelItem((PyObject *)m_DeviceDict, pKey) == -1)
			{
				Log(LOG_ERROR, "Failed to remove Unit '%u' from Device dictionary.", Unit);
			}
		}
	}

	void CPlugin::MessagePlugin(CPluginMessageBase *pMessage)
	{
		if (m_bDebug & PDM_QUEUE)
		{
			Log(LOG_NORM, "Pushing '" + std::string(pMessage->Name()) + "' on to queue");
		}

		// Add message to queue
		std::lock_guard<std::mutex> l(m_QueueMutex);
		m_MessageQueue.push_back(pMessage);
	}

	void CPlugin::DeviceAdded(const std::string DeviceID, int Unit)
	{
		CPluginMessageBase *pMessage = new onDeviceAddedCallback(DeviceID, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DeviceModified(const std::string DeviceID, int Unit)
	{
		CPluginMessageBase *pMessage = new onDeviceModifiedCallback(DeviceID, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DeviceRemoved(const std::string DeviceID, int Unit)
	{
		CPluginMessageBase *pMessage = new onDeviceRemovedCallback(DeviceID, Unit);
		MessagePlugin(pMessage);
	}

	void CPlugin::DisconnectEvent(CEventBase *pMess)
	{
		DisconnectedEvent *pMessage = (DisconnectedEvent *)pMess;
		CConnection *pConnection = (CConnection *)pMessage->m_pConnection;

		// Return any partial data to plugin
		if (pConnection->pProtocol)
		{
			pConnection->pProtocol->Flush(this, pConnection);
		}

		if (pConnection->pTransport)
		{
			if (m_bDebug & PDM_CONNECTION)
			{
				std::string sTransport = PyBorrowedRef(pConnection->Transport);
				std::string sAddress = PyBorrowedRef(pConnection->Address);
				std::string sPort = PyBorrowedRef(pConnection->Port);
				if ((sTransport == "Serial") || (sPort.empty()))
					Log(LOG_NORM, "Disconnect event received for '%s'.", sAddress.c_str());
				else
					Log(LOG_NORM, "Disconnect event received for '%s:%s'.", sAddress.c_str(), sPort.c_str());
			}

			RemoveConnection(pConnection->pTransport);
			delete pConnection->pTransport;
			pConnection->pTransport = nullptr;

			// inform the plugin if transport is connection based
			if (pMessage->bNotifyPlugin)
			{
				MessagePlugin(new onDisconnectCallback(pConnection));
			}

			// Plugin exiting and all connections have disconnect messages queued
			if (IsStopRequested(0) && m_Transports.empty())
			{
				MessagePlugin(new onStopCallback());
			}
		}
	}

	void CPlugin::RestoreThread()
	{
		if (m_PyInterpreter)
		{
			PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
		}
		else
		{
			Log(LOG_ERROR, "Attempt to aquire the GIL with NULL Interpreter details.");
		}
	}

	void CPlugin::ReleaseThread()
	{
		if (m_PyInterpreter)
		{
			if (!PyEval_SaveThread())
			{
				Log(LOG_ERROR, "Attempt to release GIL returned NULL value");
			}
		}
	}

	void CPlugin::Callback(PyBorrowedRef& pTarget, const std::string &sHandler, PyObject *pParams)
	{
		try
		{
			// Callbacks MUST already have taken the PythonMutex lock otherwise bad things will happen
			if (pTarget && !sHandler.empty())
			{
				if (PyErr_Occurred())
				{
					PyErr_Clear();
					Log(LOG_NORM, "Python exception set prior to callback '%s'", sHandler.c_str());
				}

				PyNewRef pFunc = PyObject_GetAttrString(pTarget, sHandler.c_str());
				if (pFunc && PyCallable_Check(pFunc))
				{
					// Store the callback object so the Dump function has context if invoked
					module_state* pModState = FindModule();
					if (pModState)
					{
						pModState->lastCallback = pTarget;
					}

					if (m_bDebug & PDM_QUEUE)
					{
						Log(LOG_NORM, "Calling message handler '%s' on '%s' type object.", sHandler.c_str(), pTarget.Type().c_str());
					}

					PyErr_Clear();

					// Invoke the callback function
					PyNewRef	pReturnValue = PyObject_CallObject(pFunc, pParams);

					if (pModState)
					{
						pModState->lastCallback = nullptr;	
					}
					if (!pReturnValue || PyErr_Occurred())
					{
						LogPythonException(sHandler);
						{
							PyErr_Clear();
						}
						if (m_bDebug & PDM_PLUGIN)
						{
							// See if additional information is available
							PyNewRef pLocals = PyObject_Dir(pTarget);
							if (pLocals.IsList())  // && PyIter_Check(pLocals))  // Check fails but iteration works??!?
							{
								Log(LOG_NORM, "Local context:");
								PyNewRef pIter = PyObject_GetIter(pLocals);
								PyNewRef pItem = PyIter_Next(pIter);
								while (pItem)
								{
									std::string sAttrName = pItem;
									if (sAttrName.substr(0, 2) != "__") // ignore system stuff
									{
										std::string	strValue = pTarget.Attribute(sAttrName);
										if (strValue.length())
										{
											PyNewRef pValue = PyObject_GetAttrString(pTarget, sAttrName.c_str());
											if (!PyCallable_Check(pValue)) // Filter out methods
											{
												std::string sBlank((sAttrName.length() < 20) ? 20 - sAttrName.length() : 0, ' ');
												Log(LOG_NORM, " ----> '%s'%s '%s'", sAttrName.c_str(), sBlank.c_str(), strValue.c_str());
											}
										}
									}
									pItem = PyIter_Next(pIter);
								}
							}
						}
					}
				}
				else
				{
					if (m_bDebug & PDM_QUEUE)
					{
						Log(LOG_NORM, "Message handler '%s' not callable, ignored.", sHandler.c_str());
					}
					if (PyErr_Occurred())
					{
						PyErr_Clear();
					}
				}
			}
		}
		catch (std::exception *e)
		{
			Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}
	}

	long CPlugin::PythonThreadCount()
	{
		long	lRetVal = 0;

		if (m_PyModule)
		{
			PyBorrowedRef	pModuleDict = PyModule_GetDict(m_PyModule);
			if (pModuleDict)
			{
				PyBorrowedRef	pThreadModule = PyDict_GetItemString(pModuleDict, "threading");
				if (pThreadModule)
				{
					PyNewRef pFunc = PyObject_GetAttrString(pThreadModule, "active_count");
					if (pFunc && PyCallable_Check(pFunc))
					{
						PyNewRef	pReturnValue = PyObject_CallObject(pFunc, nullptr);
						if (pReturnValue.IsLong())
						{
							lRetVal = PyLong_AsLong(pReturnValue) - 1;
							if (lRetVal)
							{
								Log(LOG_NORM, "Warning: Plugin has %d Python threads running.", (int)lRetVal);
							}
						}
					}
				}
			}
		}
		return lRetVal;
	}

	void CPlugin::Stop()
	{
		try
		{
			PyErr_Clear();

			// Validate Device dictionary prior to shutdown
			if (m_DeviceDict)
			{
				PyBorrowedRef brModule = PyState_FindModule(&DomoticzModuleDef);
				if (!brModule)
				{
					brModule = PyState_FindModule(&DomoticzExModuleDef);
					if (!brModule)
					{
						Log(LOG_ERROR, "(%s) %s failed, Domoticz/DomoticzEx modules not found in interpreter.", __func__, m_PluginKey.c_str());
						return;
					}
				}

				module_state *pModState = ((struct module_state *)PyModule_GetState(brModule));
				if (!pModState)
				{
					Log(LOG_ERROR, "%s, unable to obtain module state.", __func__);
					return;
				}

				PyBorrowedRef	key;
				PyBorrowedRef	pDevice;
				Py_ssize_t pos = 0;
				// Sanity check to make sure the reference counting is all good.
				while (PyDict_Next((PyObject*)m_DeviceDict, &pos, &key, &pDevice))
				{
					// Dictionary should be full of Devices but Python script can make this assumption false, log warning if this has happened
					int isDevice = PyObject_IsInstance(pDevice, (PyObject *)pModState->pDeviceClass);
					if (isDevice == -1)
					{
						LogPythonException("Error determining type of Python object during dealloc");
					}
					else if (isDevice == 0)
					{
						PyNewRef	pName = PyObject_GetAttrString((PyObject*)pDevice->ob_type, "__name__");
						Log(LOG_NORM, "%s: Device dictionary contained non-Device entry '%s'.", __func__, ((std::string)pName).c_str());
					}
					else
					{
						PyNewRef pUnits = PyObject_GetAttrString(pDevice, "Units");	// Free any Units if the object has them
						if (pUnits)
						{
							PyBorrowedRef key;
							PyBorrowedRef pUnit;
							Py_ssize_t	pos = 0;
							// Sanity check to make sure the reference counting is all good.
							while (PyDict_Next(pUnits, &pos, &key, &pUnit))
							{
								// Dictionary should be full of Units but Python script can make this assumption false, log warning if this has happened
								int isValue = PyObject_IsInstance(pUnit, (PyObject *)pModState->pUnitClass);
								if (isValue == -1)
								{
									_log.Log(LOG_ERROR, "Error determining type of Python object during dealloc");
								}
								else if (isValue == 0)
								{
									PyNewRef	pName = PyObject_GetAttrString((PyObject*)pUnit->ob_type, "__name__");
									_log.Log(LOG_NORM, "%s: Unit dictionary contained non-Unit entry '%s'.", __func__, ((std::string)pName).c_str());
								}
								else
								{
									if (pUnit->ob_refcnt > 1)
									{
										PyNewRef pName = PyObject_GetAttrString(pUnit, "Name");
										std::string sName = PyBorrowedRef(pName);
										_log.Log(LOG_ERROR, "%s: Unit '%s' Reference Count not one: %d.", __func__, sName.c_str(), static_cast<int>(pUnit->ob_refcnt));
									}
									else if (pUnit->ob_refcnt < 1)
									{
										_log.Log(LOG_ERROR, "%s: Unit Reference Count not one: %d.", __func__, static_cast<int>(pUnit->ob_refcnt));
									}
								}
							}
							PyDict_Clear(pUnits);
						}
						else
						{
							PyErr_Clear();
						}

						if (pDevice->ob_refcnt > 1)
						{
							PyNewRef pName = PyObject_GetAttrString(pDevice, "Name");
							if (!pName)
							{
								PyErr_Clear();
								pName = PyObject_GetAttrString(pDevice, "DeviceID");
							}
							Log(LOG_ERROR, "%s: Device '%s' Reference Count not correct, expected %d found %d.", __func__, std::string(pName).c_str(), 1, (int)pDevice->ob_refcnt);
						}
						else if (pDevice->ob_refcnt < 1)
						{
							Log(LOG_ERROR, "%s: Device Reference Count is less than one: %d.", __func__, (int)pDevice->ob_refcnt);
						}
					}
				}
				PyDict_Clear((PyObject*)m_DeviceDict);
			}

			// if threading module is running then check no threads are still running
			for (int i=10; PythonThreadCount() && i; i--)
			{
				sleep_milliseconds(1000);
			}
			if (PythonThreadCount())
				Log(LOG_NORM, "Abandoning wait for Plugin thread shutdown, hang or crash may result.");

			// Stop Python
			Py_XDECREF(m_PyModule);
			m_PyModule = nullptr;
			Py_XDECREF(m_DeviceDict);
			if (m_ImageDict)
				Py_XDECREF(m_ImageDict);
			if (m_SettingsDict)
				Py_XDECREF(m_SettingsDict);
			if (m_PyInterpreter)
				Py_EndInterpreter(m_PyInterpreter);
			// To release the GIL there must be a valid thread state so use
			// the one created during start up of the plugin system because it will always exist
			CPluginSystem pManager;
			PyThreadState_Swap((PyThreadState *)pManager.PythonThread());
			PyEval_ReleaseLock();
		}
		catch (std::exception *e)
		{
			Log(LOG_ERROR, "%s: Exception thrown releasing Interpreter: %s", __func__, e->what());
		}
		catch (...)
		{
			Log(LOG_ERROR, "%s: Unknown exception thrown releasing Interpreter", __func__);
		}

		m_PyModule = nullptr;
		m_DeviceDict = nullptr;
		m_ImageDict = nullptr;
		m_SettingsDict = nullptr;
		m_PyInterpreter = nullptr;
		m_bIsStarted = false;

		// Flush the message queue (should already be empty)
		{
			std::lock_guard<std::mutex> l(m_QueueMutex);
			while (!m_MessageQueue.empty())
			{
				m_MessageQueue.pop_front();
			}
		}

		m_bIsStopped = true;
	}

	bool CPlugin::LoadSettings()
	{
		// Only load/reload settings if the plugin initial import was successful
		if (PythonModule())
		{
			PyBorrowedRef	pModuleDict = PyModule_GetDict(PythonModule()); // returns a borrowed referece to the __dict__ object for the __main__ module
			if (m_SettingsDict)
				Py_XDECREF(m_SettingsDict);
			m_SettingsDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Settings", (PyObject*)m_SettingsDict) == -1)
			{
				Log(LOG_ERROR, "(%s) failed to add Settings dictionary.", m_PluginKey.c_str());
				return false;
			}

			// load associated settings to make them available to python
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Key, nValue, sValue FROM Preferences");
			if (!result.empty())
			{
				// Add settings strings into the settings dictionary with Unit as the key
				for (const auto& sd : result)
				{
					PyNewRef	pValue;
					if (!sd[2].empty())
					{
						pValue = PyUnicode_FromString(sd[2].c_str());
					}
					else
					{
						pValue = PyUnicode_FromString(sd[1].c_str());
					}
					if (PyDict_SetItemString((PyObject*)m_SettingsDict, sd[0].c_str(), pValue))
					{
						Log(LOG_ERROR, "(%s) failed to add setting '%s' to settings dictionary.", m_PluginKey.c_str(), sd[0].c_str());
						return false;
					}
				}
			}
		}

		return true;
	}

#define DZ_BYTES_PER_LINE 20
	void CPlugin::WriteDebugBuffer(const std::vector<byte> &Buffer, bool Incoming)
	{
		if (m_bDebug & (PDM_CONNECTION | PDM_MESSAGE))
		{
			if (Incoming)
				Log(LOG_NORM, "Received %d bytes of data", (int)Buffer.size());
			else
				Log(LOG_NORM, "Sending %d bytes of data", (int)Buffer.size());
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
						if ((int)Buffer[i + j] > 32)
							sChars += Buffer[i + j];
						else
							sChars += ".";
					}
					else
						ssHex << ".. ";
				}
				Log(LOG_NORM, "     %s    %s", ssHex.str().c_str(), sChars.c_str());
			}
		}
	}

	bool CPlugin::WriteToHardware(const char *pdata, const unsigned char length)
	{
		return true;
	}

	void CPlugin::SendCommand(const std::string &DeviceID, const int Unit, const std::string &command, const int level, const _tColor color)
	{
		//	Add command to message queue
		std::string JSONColor = color.toJSONString();
		MessagePlugin(new onCommandCallback(DeviceID, Unit, command, level, JSONColor));
	}

	void CPlugin::SendCommand(const std::string &DeviceID, const int Unit, const std::string &command, const float level)
	{
		//	Add command to message queue
		MessagePlugin(new onCommandCallback(DeviceID, Unit, command, level));
	}

	bool CPlugin::HasNodeFailed(const std::string DeviceID, const int Unit)
	{
		if (!m_DeviceDict)
			return true;

		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while (PyDict_Next((PyObject *)m_DeviceDict, &pos, &key, &value))
		{
			// Handle different Device dictionaries types
			PyBorrowedRef	pKeyType(key);
			if (pKeyType.IsString())
			{
				// Version 2+ of the framework, keyed by DeviceID
				std::string sKey = PyUnicode_AsUTF8(key);
				if (sKey == DeviceID)
				{
					CDeviceEx *pDevice = (CDeviceEx *)value;
					return (pDevice->TimedOut != 0);
				}
			}
			else if (pKeyType.IsLong())
			{
				// Version 1 of the framework, keyed by Unit
				long iKey = PyLong_AsLong(key);
				if (iKey == -1 && PyErr_Occurred())
				{
					PyErr_Clear();
					return false;
				}

				if (iKey == Unit)
				{
					CDevice *pDevice = (CDevice *)value;
					return (pDevice->TimedOut != 0);
				}
			}
			else
			{
				Log(LOG_ERROR, "'%s' Invalid Node key type.", __func__);
			}
		}

		return false;
	}

	PyBorrowedRef CPlugin::FindDevice(const std::string &Key)
	{
		if (m_DeviceDict && PyBorrowedRef(m_DeviceDict).IsDict())
		{
			return PyDict_GetItemString((PyObject*)m_DeviceDict, Key.c_str());
		}
		else
		{
			Log(LOG_ERROR, "Devices dictionary null or not valid in '%s'.", __func__);
		}
		return nullptr;
	}

	PyBorrowedRef	CPlugin::FindUnitInDevice(const std::string &deviceKey, const int unitKey)
	{
		CDeviceEx *pDevice = this->FindDevice(deviceKey);

		if (pDevice)
		{
			PyNewRef pKey = PyLong_FromLong(unitKey);
			return PyBorrowedRef(PyDict_GetItem((PyObject *)pDevice->m_UnitDict, pKey));
		}

		return nullptr;
	}

	CPluginNotifier::CPluginNotifier(CPlugin *pPlugin, const std::string &NotifierName)
		: CNotificationBase(NotifierName, OPTIONS_NONE)
		, m_pPlugin(pPlugin)
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
		int iIconLine = atoi(szCustom.c_str());
		std::string szRetVal = "Light48";
		if (iIconLine < 100) // default set of custom icons
		{
			std::string sLine;
			std::ifstream infile;
			std::string switchlightsfile = szWWWFolder + "/switch_icons.txt";
			infile.open(switchlightsfile.c_str());
			if (infile.is_open())
			{
				int index = 0;
				while (!infile.eof())
				{
					getline(infile, sLine);
					if ((!sLine.empty()) && (index++ == iIconLine))
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
		else // Uploaded icons
		{
			std::vector<std::vector<std::string>> result;
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
		std::string szImageFile;
#ifdef WIN32
		std::string szImageFolder = szWWWFolder + "\\images\\";
#else
		std::string szImageFolder = szWWWFolder + "/images/";
#endif

		std::string szStatus = "Off";
		int posStatus = (int)ExtraData.find("|Status=");
		if (posStatus >= 0)
		{
			posStatus += 8;
			szStatus = ExtraData.substr(posStatus, ExtraData.find('|', posStatus) - posStatus);
			if (szStatus != "Off")
				szStatus = "On";
		}

		// Use image is specified
		int posImage = (int)ExtraData.find("|Image=");
		if (posImage >= 0)
		{
			posImage += 7;
			szImageFile = szImageFolder + ExtraData.substr(posImage, ExtraData.find('|', posImage) - posImage) + ".png";
			if (file_exist(szImageFile.c_str()))
			{
				return szImageFile;
			}
		}

		// Use uploaded and custom images
		int posCustom = (int)ExtraData.find("|CustomImage=");
		if (posCustom >= 0)
		{
			posCustom += 13;
			std::string szCustom = ExtraData.substr(posCustom, ExtraData.find('|', posCustom) - posCustom);
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
		int posType = (int)ExtraData.find("|SwitchType=");
		if (posType >= 0)
		{
			posType += 12;
			std::string szType = ExtraData.substr(posType, ExtraData.find('|', posType) - posType);
			std::string szTypeImage;
			_eSwitchType switchtype = (_eSwitchType)atoi(szType.c_str());
			switch (switchtype)
			{
				case STYPE_OnOff:
					if (posCustom >= 0)
					{
						std::string szCustom = ExtraData.substr(posCustom, ExtraData.find('|', posCustom) - posCustom);
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
				case STYPE_BlindsPercentageWithStop:
				case STYPE_VenetianBlindsUS:
				case STYPE_VenetianBlindsEU:
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
						std::string szCustom = ExtraData.substr(posCustom, ExtraData.find('|', posCustom) - posCustom);
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
			m_pPlugin->Log(LOG_ERROR, "Logo image file does not exist: %s", szImageFile.c_str());
			szImageFile = "";
		}
		return szImageFile;
	}

	bool CPluginNotifier::SendMessageImplementation(const uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData,
							const int Priority, const std::string &Sound, const bool bFromNotification)
	{
		// ExtraData = |Name=Test|SwitchType=9|CustomImage=0|Status=On|

		std::string sIconFile = GetIconFile(ExtraData);
		std::string sName = "Unknown";
		int posName = (int)ExtraData.find("|Name=");
		if (posName >= 0)
		{
			posName += 6;
			sName = ExtraData.substr(posName, ExtraData.find('|', posName) - posName);
		}

		std::string sStatus = "Unknown";
		int posStatus = (int)ExtraData.find("|Status=");
		if (posStatus >= 0)
		{
			posStatus += 8;
			sStatus = ExtraData.substr(posStatus, ExtraData.find('|', posStatus) - posStatus);
		}

		//	Add command to message queue for every plugin
		m_pPlugin->MessagePlugin(new onNotificationCallback(m_pPlugin, Subject, Text, sName, sStatus, Priority, Sound, sIconFile));

		return true;
	}

	bool PyBorrowedRef::TypeCheck(long PyType)
	{
		if (m_pObject)
		{
			PyNewRef	pType = PyObject_Type(m_pObject);
			return pType && (PyType_GetFlags((PyTypeObject*)pType) & PyType);
		}
		return false;
	}

	std::string PyBorrowedRef::Attribute(const char* name)
	{
		std::string	sAttr = "";
		if (m_pObject)
		{
			try
			{
				if (PyObject_HasAttrString(m_pObject, name))
				{
					PyNewRef	pAttr = PyObject_GetAttrString(m_pObject, name);
					sAttr = (std::string)pAttr;
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "[%s] Exception determining Python object attribute '%s'.", __func__, name);
			}
		}
		return sAttr;
	}

	std::string PyBorrowedRef::Type()
	{
		std::string	sType = "";
		if (m_pObject)
		{
			PyNewRef	pType = PyObject_Type(m_pObject);
			sType = pType.Attribute("__name__");
		}
		return sType;
	}
} // namespace Plugins
#endif
