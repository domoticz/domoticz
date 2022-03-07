#pragma once

#include "RFXNames.h"
#include "EventSystem.h"

#ifdef ENABLE_PYTHON
    #include "../hardware/plugins/DelayedLink.h"
    #include "structmember.h"

    namespace Plugins {
      typedef struct {
          PyObject_HEAD
          PyObject *name; /* device name */
          PyObject *n_value_string;  /* n_value string */
          PyObject *s_value;  /* n_value string */
          PyObject *last_update_string;  /* last_update_string */
          int n_value;
          int type;
          int sub_type;
          int switch_type;
          int id;
      } PDevice;

      PyObject * PDevice_Describe(PDevice* self);

      void PDevice_dealloc(PDevice* self);
      int PDevice_init(PDevice *self, PyObject *args, PyObject *kwds);
      PyObject * PDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

      static PyMemberDef PDevice_members[] = {
	      { "name", T_OBJECT_EX, offsetof(PDevice, name), 0, "Device name" },
	      { "last_update_string", T_OBJECT_EX, offsetof(PDevice, last_update_string), 0, "Device last Update" },
	      { "n_value", T_INT, offsetof(PDevice, n_value), 0, "Device n_value" },
	      { "n_value_string", T_OBJECT_EX, offsetof(PDevice, n_value_string), 0, "Device n_value_string" },
	      { "s_value", T_OBJECT_EX, offsetof(PDevice, s_value), 0, "Device s_value" },
	      { "id", T_INT, offsetof(PDevice, id), 0, "Device id" },
	      { "type", T_INT, offsetof(PDevice, type), 0, "Device type" },
	      { "sub_type", T_INT, offsetof(PDevice, sub_type), 0, "Device subType" },
	      { "switch_type", T_INT, offsetof(PDevice, switch_type), 0, "Device switchType" },
	      { nullptr } /* Sentinel */
      };

      static PyMethodDef PDevice_methods[] = {
	      { "Describe", (PyCFunction)PDevice_Describe, METH_NOARGS, "Return the name, combining the first and last name" },
	      { nullptr } /* Sentinel */
      };

      static PyModuleDef PDevicemodule = { PyModuleDef_HEAD_INIT,
					   "DomoticzEvents",
					   "DomoticzEvents module type.",
					   -1,
					   nullptr,
					   nullptr,
					   nullptr,
					   nullptr,
					   nullptr };
    }
#endif
