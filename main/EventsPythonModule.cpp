#include "stdafx.h"

#include "Logger.h"
#include "EventsPythonModule.h"
#include "EventsPythonDevice.h"
#include "EventSystem.h"
#include "mainworker.h"
#include "../hardware/plugins/Plugins.h"

#include <fstream>

#ifdef ENABLE_PYTHON

namespace Plugins 
{
#define GETSTATE(m) ((struct eventModule_state*)PyModule_GetState(m))

	PyThreadState*	m_PyInterpreter;
    bool			m_ModuleInitialized = false;
	PyObject*		pDeviceType;

    struct eventModule_state {
		PyObject*	error;
    };

	static PyMethodDef DomoticzEventsMethods[] = { 
								{ "Log", PyDomoticz_EventsLog, METH_VARARGS, "Write message to Domoticz log." },
								{ "Command", PyDomoticz_EventsCommand, METH_VARARGS, "Schedule a command." },
								{ nullptr, nullptr, 0, nullptr } };

	static int DomoticzEventsTraverse(PyObject *m, visitproc visit, void *arg)
	{
		Py_VISIT(GETSTATE(m)->error);
		return 0;
	}

	static int DomoticzEventsClear(PyObject *m)
	{
		Py_CLEAR(GETSTATE(m)->error);
		return 0;
	}

	// Module methods

	static PyObject *PyDomoticz_EventsLog(PyObject *self, PyObject *args)
	{
		PyBorrowedRef	pArg(args);
		if (!pArg.IsTuple())
		{
			_log.Log(LOG_ERROR, "%s: Invalid parameter, expected 'tuple' got '%s'.", __func__, pArg.Type().c_str());
			Py_RETURN_NONE;
		}

		Py_ssize_t	tupleSize = PyTuple_Size(pArg);
		if (tupleSize != 1)
		{
			_log.Log(LOG_ERROR, "%s: Invalid parameter, expected single parameter, got %d parameters.", __func__, (int)tupleSize);
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
				_log.Log(LOG_NORM, sMessage);
			}
			else
			{
				_log.Log(LOG_ERROR, "%s: Failed to parse parameters: string expected.", __func__);
			}
		}
		else
		{
			std::string message = msg;
			_log.Log((_eLogLevel)LOG_NORM, message);
		}

		Py_RETURN_NONE;
	}

	static PyObject *PyDomoticz_EventsCommand(PyObject *self, PyObject *args)
	{
		char *action;
		char *device;

		// _log.Log(LOG_STATUS, "Python EventSystem: Running command.");
		// m_eventsystem.CEventSystem::PythonScheduleEvent("Test_Target", "On", "Testing");
		//

		if (!PyArg_ParseTuple(args, "ss", &device, &action))
		{
			_log.Log(LOG_ERROR, "Pyhton EventSystem: Failed to parse parameters: Two strings expected.");
		}
		else
		{
			// std::string	dev = device;
			// std::string act = action;
			// _log.Log((_eLogLevel)LOG_NORM, "Python EventSystem - Command: Target: %s Command: %s", dev.c_str(), act.c_str());
			m_mainworker.m_eventsystem.PythonScheduleEvent(device, action, "Test");
		}

		Py_RETURN_NONE;
	}

	struct PyModuleDef DomoticzEventsModuleDef = {
		PyModuleDef_HEAD_INIT,  
		"DomoticzEvents",	 
		nullptr, 
		sizeof(struct eventModule_state), 
		DomoticzEventsMethods, 
		nullptr,
		DomoticzEventsTraverse, 
		DomoticzEventsClear, 
		nullptr
	};

	PyMODINIT_FUNC PyInit_DomoticzEvents(void)
	{
		// This is called during the import of the plugin module
		// triggered by the "import Domoticz" statement

		_log.Log(LOG_STATUS, "Python EventSystem: Initializing event module.");

		PyObject *pModule = PyModule_Create2(&DomoticzEventsModuleDef, PYTHON_API_VERSION);

		PyType_Slot PDeviceSlots[] = {
			{ Py_tp_doc, (void*)"PDevice objects" },
			{ Py_tp_new, (void*)PDevice_new },
			{ Py_tp_init, (void*)PDevice_init },
			{ Py_tp_dealloc, (void*)PDevice_dealloc },
			{ Py_tp_members, PDevice_members },
			{ Py_tp_methods, PDevice_methods },
			{ 0, nullptr },
		};
		PyType_Spec DeviceSpec = { "DomoticzEvents.PDevice", sizeof(PDevice), 0,
							  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, PDeviceSlots };

		pDeviceType = PyType_FromSpec(&DeviceSpec);
		if (PyType_Ready((PyTypeObject*)pDeviceType) < 0)
		{
			_log.Log(LOG_ERROR, "Python EventSystem: Unable to ready 'PDevice objects'.");
		}

		PyModule_AddObject(pModule, "PDevice", pDeviceType);	// PyModule_AddObject steals a reference

		return pModule;
	}

	int PythonEventsInitialized = 0;

	bool PythonEventsInitialize(const std::string &szUserDataFolder)
	{

		if (!Plugins::Py_LoadLibrary())
		{
			_log.Log(LOG_STATUS, "EventSystem - Python: Failed dynamic library load, install the latest libpython3.x library "
					     "that is available for your platform.");
			return false;
		}

		if (!Plugins::Py_IsInitialized())
		{
			_log.Log(LOG_STATUS, "EventSystem - Python: Failed dynamic library load, install the latest libpython3.x library "
					     "that is available for your platform.");
			return false;
		}

		PyEval_RestoreThread((PyThreadState *)m_mainworker.m_pluginsystem.PythonThread());
		m_PyInterpreter = Py_NewInterpreter();
		if (!m_PyInterpreter)
		{
			_log.Log(LOG_ERROR, "EventSystem - Python: Failed to create interpreter.");
			return false;
		}

		std::string ssPath;
#ifdef WIN32
            ssPath  = szUserDataFolder + "scripts\\python\\;";
#else
            ssPath  = szUserDataFolder + "scripts/python/:";
#endif

            std::wstring sPath = std::wstring(ssPath.begin(), ssPath.end());

            sPath += Plugins::Py_GetPath();
            Plugins::PySys_SetPath((wchar_t*)sPath.c_str());

            PythonEventsInitialized = 1;

            PyObject* pModule = Plugins::PythonEventsGetModule();
			PyEval_SaveThread();
			if (!pModule) {
                _log.Log(LOG_ERROR, "EventSystem - Python: Failed to initialize module.");
                return false;
            }
            m_ModuleInitialized = true;
            return true;
	}

	bool PythonEventsStop()
	{
		if (m_PyInterpreter)
		{
			PyEval_RestoreThread((PyThreadState *)m_PyInterpreter);
			if (Plugins::Py_IsInitialized())
				Py_EndInterpreter((PyThreadState *)m_PyInterpreter);
			m_PyInterpreter = nullptr;
			PyThreadState_Swap((PyThreadState *)m_mainworker.m_pluginsystem.PythonThread());
			PyEval_ReleaseLock();
			_log.Log(LOG_STATUS, "EventSystem - Python stopped...");
			return true;
		}
		return false;
	}

	PyObject *PythonEventsGetModule()
	{
		PyBorrowedRef	pModule = PyState_FindModule(&DomoticzEventsModuleDef);

		if (pModule)
		{
			// _log.Log(LOG_STATUS, "Python Event System: Module found");
			return pModule;
		}
		PyImport_ImportModule("DomoticzEvents");
		pModule = PyState_FindModule(&DomoticzEventsModuleDef);

		if (pModule)
		{
			return pModule;
		}

		return nullptr;
	}

	// main_namespace["otherdevices_temperature"] = toPythonDict(m_tempValuesByName);

	PyObject *mapToPythonDict(const std::map<std::string, float> &floatMap)
	{
		Py_RETURN_NONE;
	}

	void LogPythonException()
	{
		PyNewRef	pTraceback;
		PyNewRef	pExcept;
		PyNewRef	pValue;

		PyErr_Fetch(&pExcept, &pValue, &pTraceback);
		PyErr_NormalizeException(&pExcept, &pValue, &pTraceback);

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "Unable to decode exception.");
		}
		else
		{
			std::string	sTypeText("Unknown");

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
								_log.Log(LOG_ERROR, "%s", token.c_str());
								pStr.erase(0, pos + 1);
							}
						}
					}
					else
					{
						if (pExcept) sTypeText = pExcept.Attribute("__name__");
						_log.Log(LOG_ERROR, "Exception: '%s'.  No traceback available.", sTypeText.c_str());
					}
				}
				else
				{
					if (pExcept) sTypeText = pExcept.Attribute("__name__");
					_log.Log(LOG_ERROR, "'format_exception' lookup failed, exception: '%s'.  No traceback available.", sTypeText.c_str());
				}
			}
			else
			{
				if (pExcept) sTypeText = pExcept.Attribute("__name__");
				_log.Log(LOG_ERROR, "'Traceback' module import failed, exception: '%s'.  No traceback available.", sTypeText.c_str());
			}
		}
		PyErr_Clear();
	}

	void PythonEventsProcessPython(const std::string& reason, const std::string& filename, const std::string& PyString,
		const uint64_t DeviceID, std::map<uint64_t, CEventSystem::_tDeviceStatus> deviceStates,
		std::map<uint64_t, CEventSystem::_tUserVariable> userVariables, int intSunRise, int intSunSet)
	{
		if (!m_ModuleInitialized)
		{
			return;
		}

		if (!Py_IsInitialized())
		{
			_log.Log(LOG_ERROR, "EventSystem: Python not Initialized");
			return;
		}

		if (m_PyInterpreter)
			PyEval_RestoreThread(m_PyInterpreter);

		PyBorrowedRef	pModule = PythonEventsGetModule();
		if (pModule)
		{
			PyBorrowedRef	pModuleDict = PyModule_GetDict(pModule);
			if (!pModuleDict)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to open module dictionary.");
				PyEval_SaveThread();
				return;
			}

			if (!Py_None)
			{
				PyNewRef local_dict = PyDict_New();
				PyNewRef pCode = Py_CompileString("# Eval will return 'None'\n", "<domoticz>", Py_file_input);
				if (pCode)
				{
					PyNewRef pEval = PyEval_EvalCode(pCode, pModuleDict, local_dict);
					Py_None = pEval;
					Py_INCREF(Py_None);
				}
			}


			PyNewRef	pStrVal = PyUnicode_FromString(deviceStates[DeviceID].deviceName.c_str());
			if (PyDict_SetItemString(pModuleDict, "changed_device_name", pStrVal) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to set changed_device_name.");
				return;
			}

			PyNewRef	pDeviceDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Devices", pDeviceDict) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add Device dictionary.");
				PyEval_SaveThread();
				return;
			}

			for (auto it_type = deviceStates.begin(); it_type != deviceStates.end(); ++it_type)
			{
				CEventSystem::_tDeviceStatus sitem = it_type->second;

				PyNewRef nrArgList = Py_BuildValue("(isiiisiss)", static_cast<int>(sitem.ID),
					sitem.deviceName.c_str(),
					sitem.devType,
					sitem.subType,
					sitem.switchtype,
					sitem.sValue.c_str(),
					sitem.nValue,
					sitem.nValueWording.c_str(),
					sitem.lastUpdate.c_str());
				if (!nrArgList)
				{
					_log.Log(LOG_ERROR, "Python EventSystem: Building device argument list failed for key %s.", sitem.deviceName.c_str());
					continue;
				}
				PyNewRef pDevice = PyObject_CallObject((PyObject*)pDeviceType, nrArgList);
				if (!pDevice)
				{
					_log.Log(LOG_ERROR, "Python EventSystem: Event Device object creation failed for key %s.", sitem.deviceName.c_str());
					continue;
				}

				if (sitem.ID == DeviceID)
				{
					if (PyDict_SetItemString(pModuleDict, "changed_device", (PyObject*)pDevice) == -1)
					{
						_log.Log(LOG_ERROR,
							"Python EventSystem: Failed to add device '%s' as changed_device.",
							sitem.deviceName.c_str());
					}
				}

				PyNewRef	pKey = PyUnicode_FromString(sitem.deviceName.c_str());
				if (PyDict_SetItem(pDeviceDict, pKey, (PyObject*)pDevice) == -1)
				{
					_log.Log(LOG_ERROR, "Python EventSystem: Failed to add device '%s' to device dictionary.",
						sitem.deviceName.c_str());
				}
			}

			// Time related

			// Do not correct for DST change - we only need this to compare with intRise and intSet which aren't as well
			time_t now = mytime(nullptr);
			struct tm ltime;
			localtime_r(&now, &ltime);
			int minutesSinceMidnight = (ltime.tm_hour * 60) + ltime.tm_min;

			PyNewRef	pPyLong = PyLong_FromLong(minutesSinceMidnight);
			if (PyDict_SetItemString(pModuleDict, "minutes_since_midnight", pPyLong) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'minutesSinceMidnight' to module_dict");
			}

			pPyLong = PyLong_FromLong(intSunRise);
			if (PyDict_SetItemString(pModuleDict, "sunrise_in_minutes", pPyLong) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'sunrise_in_minutes' to module_dict");
			}

			pPyLong = PyLong_FromLong(intSunSet);
			if (PyDict_SetItemString(pModuleDict, "sunset_in_minutes", pPyLong) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'sunset_in_minutes' to module_dict");
			}

			bool isDaytime = false;
			bool isNightime = false;

			if ((minutesSinceMidnight > intSunRise) && (minutesSinceMidnight < intSunSet))
			{
				isDaytime = true;
			}
			else
			{
				isNightime = true;
			}

			PyNewRef	pPyBool = PyBool_FromLong(isDaytime);
			if (PyDict_SetItemString(pModuleDict, "is_daytime", pPyBool) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'is_daytime' to module_dict");
			}

			pPyBool = PyBool_FromLong(isNightime);
			if (PyDict_SetItemString(pModuleDict, "is_nighttime", pPyBool) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'is_daytime' to module_dict");
			}

			// UserVariables
			PyNewRef	userVariablesDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "user_variables", userVariablesDict) == -1)
			{
				_log.Log(LOG_ERROR, "Python EventSystem: Failed to add uservariables dictionary.");
				PyEval_SaveThread();
				return;
			}

			for (auto it_var = userVariables.begin(); it_var != userVariables.end(); ++it_var)
			{
				CEventSystem::_tUserVariable uvitem = it_var->second;
				PyDict_SetItemString(userVariablesDict, uvitem.variableName.c_str(),
					PyUnicode_FromString(uvitem.variableValue.c_str()));
			}

			// Add __main__ module
			PyBorrowedRef	pMainModule = PyImport_AddModule("__main__");
			PyBorrowedRef	global_dict = PyModule_GetDict(pMainModule);
			PyNewRef		local_dict = PyDict_New();

			// Override sys.stderr
			{
				PyNewRef	pCode = Py_CompileString("import sys\nclass StdErrRedirect:\n    def __init__(self):\n        "
					"self.buffer = ''\n    def write(self, "
					"msg):\n        self.buffer += msg\nstdErrRedirect = "
					"StdErrRedirect()\nsys.stderr = stdErrRedirect\n",
					filename.c_str(), Py_file_input);
				if (pCode)
				{
					PyNewRef	pEval = PyEval_EvalCode(pCode, global_dict, local_dict);
				}
				else
				{
					_log.Log(LOG_ERROR, "EventSystem: Failed to compile stderror redirection for event script '%s'", reason.c_str());
				}
			}

			if (!PyErr_Occurred() && (PyString.length() > 0))
			{
				// Python-string from WebEditor
				PyNewRef	pCode = Py_CompileString(PyString.c_str(), filename.c_str(), Py_file_input);
				if (pCode)
				{
					PyNewRef	pEval = PyEval_EvalCode(pCode, global_dict, local_dict);
				}
				else
				{
					_log.Log(LOG_ERROR, "EventSystem: Failed to compile python '%s' event script '%s'", reason.c_str(), filename.c_str());
				}
			}
			else
			{
				// Script-file
				std::ifstream PythonScriptFile(filename.c_str());
				if (PythonScriptFile.is_open())
				{
					char	PyLine[256];
					std::string	PyString;
					while (PythonScriptFile.getline(PyLine, sizeof(PyLine), '\n'))
					{
						PyString.append(PyLine);
						PyString += '\n';
					}
					PythonScriptFile.close();

					PyNewRef	pCode = Py_CompileString(PyString.c_str(), filename.c_str(), Py_file_input);
					if (pCode)
					{
						PyNewRef	pEval = PyEval_EvalCode(pCode, global_dict, local_dict);
					}
					else
					{
						_log.Log(LOG_ERROR, "EventSystem: Failed to compile python '%s' event script file '%s'", reason.c_str(), filename.c_str());
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "EventSystem: Failed to open python script file '%s'", filename.c_str());
				}
			}

			// Log any exceptions
			if (PyErr_Occurred())
			{
				LogPythonException();
			}

			// Get message from stderr redirect
			std::string logString;
			if (PyObject_HasAttrString(pModule, "stdErrRedirect"))
			{
				PyNewRef	stdErrRedirect = PyObject_GetAttrString(pModule, "stdErrRedirect");
				if (PyObject_HasAttrString(stdErrRedirect, "buffer"))
				{
					PyNewRef	logBuffer = PyObject_GetAttrString(stdErrRedirect, "buffer");
					PyNewRef	logBytes = PyUnicode_AsUTF8String(logBuffer);
					if (logBytes)
					{
						logString.append(PyBytes_AsString(logBytes));
					}
				}
			}

			// Check if there were some errors written to stderr
			if (logString.length() > 0)
			{
				// Print error source
				_log.Log(LOG_ERROR, "EventSystem: Failed to execute python event script \"%s\"", filename.c_str());

				// Loop over all lines of the error message
				std::size_t lineBreakPos;
				while ((lineBreakPos = logString.find('\n')) != std::string::npos)
				{
					// Print line
					_log.Log(LOG_ERROR, "EventSystem: %s", logString.substr(0, lineBreakPos).c_str());

					// Remove line from buffer
					logString = logString.substr(lineBreakPos + 1);
				}
			}

			// Empty dictionaries to free memory
			if (pDeviceDict.IsDict())
			{
				PyDict_Clear(pDeviceDict);
			}
			if (userVariablesDict.IsDict())
			{
				PyDict_Clear(userVariablesDict);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Python EventSystem: Module not available to events");
		}

		PyEval_SaveThread();
	}
} // namespace Plugins
#endif
