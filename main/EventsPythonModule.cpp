#include "stdafx.h"

#include "Logger.h"
#include "EventsPythonModule.h"
#include "EventsPythonDevice.h"
#include "EventSystem.h"
#include "mainworker.h"
#include "localtime_r.h"

#ifdef ENABLE_PYTHON

    namespace Plugins {
        #define GETSTATE(m) ((struct eventModule_state*)PyModule_GetState(m))

		extern boost::mutex PythonMutex;		// only used during startup when multiple threads could use Python

		void*   m_PyInterpreter;
        bool ModuleInitalized = false;
        
        struct eventModule_state {
            PyObject*	error;
        };

        static PyMethodDef DomoticzEventsMethods[] = {
            { "Log", PyDomoticz_EventsLog, METH_VARARGS, "Write message to Domoticz log." },
            { "Command", PyDomoticz_EventsCommand, METH_VARARGS, "Schedule a command." },
            { NULL, NULL, 0, NULL }
        };


        static int DomoticzEventsTraverse(PyObject *m, visitproc visit, void *arg) {
            Py_VISIT(GETSTATE(m)->error);
            return 0;
        }

        static int DomoticzEventsClear(PyObject *m) {
            Py_CLEAR(GETSTATE(m)->error);
            return 0;
        }

        // Module methods

        static PyObject*	PyDomoticz_EventsLog(PyObject *self, PyObject *args) {
            char* msg;

            if (!PyArg_ParseTuple(args, "s", &msg))
            {
                _log.Log(LOG_ERROR, "Pyhton Event System: Failed to parse parameters: string expected.");
                // LogPythonException(pModState->pPlugin, std::string(__func__));
            }
            else
            {
                std::string	message = msg;
                _log.Log((_eLogLevel)LOG_NORM, message.c_str());
            }

            Py_INCREF(Py_None);
    		return Py_None;
        }

        static PyObject*	PyDomoticz_EventsCommand(PyObject *self, PyObject *args) {
            char* action;
            char* device;

            // _log.Log(LOG_STATUS, "Python EventSystem: Running command.");
            // m_eventsystem.CEventSystem::PythonScheduleEvent("Test_Target", "On", "Testing");
            //

            if (!PyArg_ParseTuple(args, "ss", &device, &action))
            {
                _log.Log(LOG_ERROR, "Pyhton EventSystem: Failed to parse parameters: Two strings expected.");
                // LogPythonException(pModState->pPlugin, std::string(__func__));
            }
            else
            {
                std::string	dev = device;
                std::string act = action;
                // _log.Log((_eLogLevel)LOG_NORM, "Python EventSystem - Command: Target: %s Command: %s", dev.c_str(), act.c_str());
                m_mainworker.m_eventsystem.PythonScheduleEvent(device, action, "Test");
            }

            Py_INCREF(Py_None);
            return Py_None;
        }

        struct PyModuleDef DomoticzEventsModuleDef = {
    		PyModuleDef_HEAD_INIT,
    		"DomoticzEvents",
    		NULL,
    		sizeof(struct eventModule_state),
    		DomoticzEventsMethods,
    		NULL,
    		DomoticzEventsTraverse,
    		DomoticzEventsClear,
    		NULL
    	};

        PyMODINIT_FUNC PyInit_DomoticzEvents(void)
        {
            // This is called during the import of the plugin module
            // triggered by the "import Domoticz" statement

            _log.Log(LOG_STATUS, "Python EventSystem: Initalizing event module.");

            PyObject* pModule = PyModule_Create2(&DomoticzEventsModuleDef, PYTHON_API_VERSION);
            return pModule;
        }
        
        int PythonEventsInitalized = 0;

        bool PythonEventsInitialize(std::string szUserDataFolder) {
            
            if (!Plugins::Py_LoadLibrary())
            {
                _log.Log(LOG_STATUS, "EventSystem - Python: Failed dynamic library load, install the latest libpython3.x library that is available for your platform.");
                return false;
            }
            
            if (!Plugins::Py_IsInitialized()) {
                _log.Log(LOG_STATUS, "EventSystem - Python: Failed dynamic library load, install the latest libpython3.x library that is available for your platform.");
                return false;
            }
            
			boost::lock_guard<boost::mutex> l(PythonMutex);
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
            
            PythonEventsInitalized = 1;
            
            PyObject* pModule = Plugins::PythonEventsGetModule();
            if (!pModule) {
                _log.Log(LOG_ERROR, "EventSystem - Python: Failed to initialize module.");
                return false;
            }
            ModuleInitalized = true;
            return true;
        }
        
        bool PythonEventsStop() {
            if (m_PyInterpreter) {
                PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
				if (Plugins::Py_IsInitialized())
					Py_EndInterpreter((PyThreadState*)m_PyInterpreter);
				m_PyInterpreter = NULL;
                _log.Log(LOG_STATUS, "EventSystem - Python stopped...");
                return true;
            } else
                return false;
        }
        
        PyObject* PythonEventsGetModule (void) {
            PyObject* pModule = PyState_FindModule(&DomoticzEventsModuleDef);

            if (pModule) {
                // _log.Log(LOG_STATUS, "Python Event System: Module found");
                return pModule;
            } else {
                Plugins::PyRun_SimpleStringFlags("import DomoticzEvents", NULL);
                pModule = PyState_FindModule(&DomoticzEventsModuleDef);

                if (pModule) {
                    return pModule;
                } else {
                    //Py_INCREF(Py_None);
                    //return Py_None;
                    return NULL;
                }
            }
        }

        // main_namespace["otherdevices_temperature"] = toPythonDict(m_tempValuesByName);
        
        PyObject* mapToPythonDict(std::map<std::string, float> floatMap) {
            
            return Py_None;
        }
       

        void PythonEventsProcessPython(const std::string &reason, const std::string &filename, const std::string &PyString, const uint64_t DeviceID, std::map<uint64_t, CEventSystem::_tDeviceStatus> m_devicestates, std::map<uint64_t, CEventSystem::_tUserVariable> m_uservariables, int intSunRise, int intSunSet) {

       
            if (!ModuleInitalized) {
                return;
            }
            

           if (Plugins::Py_IsInitialized()) {
               
               if (m_PyInterpreter) PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
               
               /*{
                   _log.Log(LOG_ERROR, "EventSystem - Python: Failed to attach to interpreter");
               }*/
            
               PyObject* pModule = Plugins::PythonEventsGetModule();
               if (pModule) {

                   PyObject* pModuleDict = Plugins::PyModule_GetDict((PyObject*)pModule); // borrowed referece

                   if (!pModuleDict) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to open module dictionary.");
                       return;
                   }

                   if (Plugins::PyDict_SetItemString(pModuleDict, "changed_device_name", Plugins::PyUnicode_FromString(m_devicestates[DeviceID].deviceName.c_str())) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to set changed_device_name.");
                       return;
                   }

                   PyObject* m_DeviceDict = Plugins::PyDict_New();

                   if (Plugins::PyDict_SetItemString(pModuleDict, "Devices", (PyObject*)m_DeviceDict) == -1)
                   {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add Device dictionary.");
                       return;
                   }
                   Py_DECREF(m_DeviceDict);

                   if (Plugins::PyType_Ready(&Plugins::PDeviceType) < 0) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Unable to ready DeviceType Object.");
                       return;
                   }

                   // Mutex
                   // boost::shared_lock<boost::shared_mutex> devicestatesMutexLock1(m_devicestatesMutex);

                   std::map<uint64_t, CEventSystem::_tDeviceStatus>::const_iterator it_type;
                   for (it_type = m_devicestates.begin(); it_type != m_devicestates.end(); ++it_type)
                   {
                       CEventSystem::_tDeviceStatus sitem = it_type->second;
                       // object deviceStatus = domoticz_module.attr("Device")(sitem.ID, sitem.deviceName, sitem.devType, sitem.subType, sitem.switchtype, sitem.nValue, sitem.nValueWording, sitem.sValue, sitem.lastUpdate);
                       // devices[sitem.deviceName] = deviceStatus;

                       Plugins::PDevice* aDevice = (Plugins::PDevice*)Plugins::PDevice_new(&Plugins::PDeviceType, (PyObject*)NULL, (PyObject*)NULL);
                       PyObject* pKey = Plugins::PyUnicode_FromString(sitem.deviceName.c_str());

                       if (sitem.ID == DeviceID) {
                           if (Plugins::PyDict_SetItemString(pModuleDict, "changed_device", (PyObject*)aDevice) == -1) {
                               _log.Log(LOG_ERROR, "Python EventSystem: Failed to add device '%s' as changed_device.", sitem.deviceName.c_str());
                           }
                       }

                       if (Plugins::PyDict_SetItem((PyObject*)m_DeviceDict, pKey, (PyObject*)aDevice) == -1)
                       {
                           _log.Log(LOG_ERROR, "Python EventSystem: Failed to add device '%s' to device dictionary.", sitem.deviceName.c_str());
                       } else {

                           // _log.Log(LOG_ERROR, "Python EventSystem: nValueWording '%s' - done. ", sitem.nValueWording.c_str());

                           std::string temp_n_value_string = sitem.nValueWording;

                           // If nValueWording contains %, unicode fails?

                           aDevice->id = static_cast<int>(sitem.ID);
                           aDevice->name = Plugins::PyUnicode_FromString(sitem.deviceName.c_str());
                           aDevice->type = sitem.devType;
                           aDevice->sub_type = sitem.subType;
                           aDevice->switch_type = sitem.switchtype;
                           aDevice->n_value = sitem.nValue;
                           aDevice->n_value_string = Plugins::PyUnicode_FromString(temp_n_value_string.c_str());
                           aDevice->s_value = Plugins::PyUnicode_FromString(sitem.sValue.c_str());
                           aDevice->last_update_string = Plugins::PyUnicode_FromString(sitem.lastUpdate.c_str());
                           // _log.Log(LOG_STATUS, "Python EventSystem: deviceName %s added to device dictionary", sitem.deviceName.c_str());
                       }
                       Py_DECREF(aDevice);
                       Py_DECREF(pKey);
                   }
                   // devicestatesMutexLock1.unlock();

                   // Time related

                   // Do not correct for DST change - we only need this to compare with intRise and intSet which aren't as well
                   time_t now = mytime(NULL);
                   struct tm ltime;
                   localtime_r(&now, &ltime);
                   int minutesSinceMidnight = (ltime.tm_hour * 60) + ltime.tm_min;

                   if (Plugins::PyDict_SetItemString(pModuleDict, "minutes_since_midnight", Plugins::PyLong_FromLong(minutesSinceMidnight)) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'minutesSinceMidnight' to module_dict");
                   }

                   if (Plugins::PyDict_SetItemString(pModuleDict, "sunrise_in_minutes", Plugins::PyLong_FromLong(intSunRise)) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'sunrise_in_minutes' to module_dict");
                   }

                   if (Plugins::PyDict_SetItemString(pModuleDict, "sunset_in_minutes", Plugins::PyLong_FromLong(intSunSet)) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'sunset_in_minutes' to module_dict");
                   }

                   //PyObject* dayTimeBool = Py_False;
                   //PyObject* nightTimeBool = Py_False;

                   bool isDaytime = false;
                   bool isNightime = false;

                   if ((minutesSinceMidnight > intSunRise) && (minutesSinceMidnight < intSunSet)) {
                       isDaytime = true;
                   }
                   else {
                       isNightime = true;
                   }

                   if (Plugins::PyDict_SetItemString(pModuleDict, "is_daytime", Plugins::PyBool_FromLong(isDaytime)) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'is_daytime' to module_dict");
                   }

                   if (Plugins::PyDict_SetItemString(pModuleDict, "is_nighttime", Plugins::PyBool_FromLong(isNightime)) == -1) {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add 'is_daytime' to module_dict");
                   }

                   // UserVariables
                   PyObject* m_uservariablesDict = Plugins::PyDict_New();

                   if (Plugins::PyDict_SetItemString(pModuleDict, "user_variables", (PyObject*)m_uservariablesDict) == -1)
                   {
                       _log.Log(LOG_ERROR, "Python EventSystem: Failed to add uservariables dictionary.");
                       return;
                   }
                   Py_DECREF(m_uservariablesDict);

                   // This doesn't work
                   // boost::unique_lock<boost::shared_mutex> uservariablesMutexLock2 (m_uservariablesMutex);

                   std::map<uint64_t, CEventSystem::_tUserVariable>::const_iterator it_var;
                   for (it_var = m_uservariables.begin(); it_var != m_uservariables.end(); ++it_var) {
                       CEventSystem::_tUserVariable uvitem = it_var->second;
                       Plugins::PyDict_SetItemString(m_uservariablesDict, uvitem.variableName.c_str(), Plugins::PyUnicode_FromString(uvitem.variableValue.c_str()));
                   }

                   // uservariablesMutexLock2.unlock();
                   

                   if(PyString.length() > 0) {
                       // Python-string from WebEditor
                       Plugins::PyRun_SimpleStringFlags(PyString.c_str(), NULL);
                       
                   } else {
                       // Script-file
                       FILE* PythonScriptFile = fopen(filename.c_str(), "r");
                       Plugins::PyRun_SimpleFileExFlags(PythonScriptFile, filename.c_str(), 0, NULL);
                       
                       if (PythonScriptFile!=NULL)
                           fclose(PythonScriptFile);
                   }
                } else {
                    _log.Log(LOG_ERROR, "Python EventSystem: Module not available to events");
                }
            } else {
                _log.Log(LOG_ERROR, "EventSystem: Python not initalized");
            }

        }
    }
#endif
