#include "stdafx.h"

#include "Logger.h"
#include "EventsPythonModule.h"
#include "mainworker.h"

#ifdef ENABLE_PYTHON
    namespace Plugins {
        #define GETSTATE(m) ((struct eventModule_state*)PyModule_GetState(m))

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
            char* msg;
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

        PyObject* GetEventModule (void) {
            PyObject* pModule = PyState_FindModule(&DomoticzEventsModuleDef);

            if (pModule) {
                // _log.Log(LOG_STATUS, "Python Event System: Module found");
                return pModule;
            } else {
                _log.Log(LOG_STATUS, "Python EventSystem: Module not found - Trying to initialize.");
                PyRun_SimpleString("import DomoticzEvents");
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
    }
#endif
