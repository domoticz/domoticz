#pragma once


#include "RFXNames.h"
#include "EventSystem.h"

#ifdef ENABLE_PYTHON
    #include "../hardware/plugins/DelayedLink.h"

    namespace Plugins {
        PyMODINIT_FUNC PyInit_DomoticzEvents(void);
        static PyObject*    PyDomoticz_EventsLog(PyObject *self, PyObject *args);
        static PyObject*	PyDomoticz_EventsCommand(PyObject *self, PyObject *args);

	PyObject *PythonEventsGetModule();
	bool PythonEventsInitialize(const std::string &szUserDataFolder);
	bool PythonEventsStop();
	void PythonEventsProcessPython(const std::string &reason, const std::string &filename, const std::string &PyString, uint64_t DeviceID,
				       std::map<uint64_t, CEventSystem::_tDeviceStatus> m_devicestates, std::map<uint64_t, CEventSystem::_tUserVariable> m_uservariables, int intSunRise,
				       int intSunSet);
    } // namespace Plugins
#endif
