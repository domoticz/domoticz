#include "stdafx.h"

#include "Logger.h"
#include "EventsPythonModule.h"

namespace Plugins {

    #define GETSTATE(m) ((struct eventModule_state*)PyModule_GetState(m))

    struct eventModule_state {
        PyObject*	error;
    };

    static PyMethodDef DomoticzEventsMethods[] = {
        { "Log", PyDomoticz_EventsLog, METH_VARARGS, "Write message to Domoticz log." },
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
        PyObject* pModule = PyModule_Create2(&DomoticzEventsModuleDef, PYTHON_API_VERSION);
        return pModule;
    }
}
