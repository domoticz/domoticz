#pragma once

#include "../hardware/plugins/DelayedLink.h"


namespace Plugins {

PyMODINIT_FUNC PyInit_DomoticzEvents(void);
static PyObject* PyDomoticz_EventsLog(PyObject *self, PyObject *args);

PyObject* GetEventModule (void);
}
