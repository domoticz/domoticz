#pragma once


#include "RFXNames.h"
#include "EventSystem.h"
#include "../hardware/plugins/DelayedLink.h"

namespace Plugins {

PyMODINIT_FUNC PyInit_DomoticzEvents(void);
static PyObject*    PyDomoticz_EventsLog(PyObject *self, PyObject *args);
static PyObject*	PyDomoticz_EventsCommand(PyObject *self, PyObject *args);

PyObject* GetEventModule (void);

}
