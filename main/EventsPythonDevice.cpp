#include "stdafx.h"

#include "EventsPythonDevice.h"

#ifdef ENABLE_PYTHON
    namespace Plugins {



      void
      PDevice_dealloc(PDevice* self)
      {
          Py_XDECREF(self->name);
          Py_XDECREF(self->n_value_string);
          Py_XDECREF(self->s_value);
          Py_XDECREF(self->last_update_string);

		  PyObject*	pType = PyObject_Type((PyObject*)self);
		  freefunc pFree = (freefunc)PyType_GetSlot((PyTypeObject*)pType, Py_tp_free);
		  pFree((PyObject*)self);
		  Py_XDECREF(pType);
	  }

      PyObject *
      PDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
      {
          PDevice *self;

		  allocfunc pAlloc = (allocfunc)PyType_GetSlot(type, Py_tp_alloc);
		  self = (PDevice*)pAlloc(type, 0);

	  if (self != nullptr)
	  {
		  self->name = PyUnicode_FromString("");
		  if (self->name == nullptr)
		  {
			  Py_DECREF(self);
			  return nullptr;
		  }

		  self->n_value_string = PyUnicode_FromString("");
		  if (self->n_value_string == nullptr)
		  {
			  Py_DECREF(self);
			  return nullptr;
		  }

		  self->s_value = PyUnicode_FromString("");
		  if (self->s_value == nullptr)
		  {
			  Py_DECREF(self);
			  return nullptr;
		  }

		  self->last_update_string = PyUnicode_FromString("");
		  if (self->last_update_string == nullptr)
		  {
			  Py_DECREF(self);
			  return nullptr;
		  }

		  self->id = 0;
		  self->n_value = 0;
		  self->type = 0;
		  self->sub_type = 0;
		  self->switch_type = 0;
	  }

	  return (PyObject *)self;
      }

      int
      PDevice_init(PDevice *self, PyObject *args, PyObject *kwds)
      {
	      PyObject *name = nullptr, *s_value = nullptr, *n_value_string = nullptr, *last_update_string, *tmp;

	      static char *kwlist[] = { "id",	   "name",    "type",		"sub_type",	      "switch_type",
					"s_value", "n_value", "n_value_string", "last_update_string", nullptr };

	      if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iOiiiOiOO", kwlist, &self->id, &name, &self->type, &self->sub_type,
					       &self->switch_type, &s_value, &self->n_value, &n_value_string, &last_update_string))
		      return -1;

	      if (name)
	      {
		      tmp = self->name;
		      Py_INCREF(name);
		      self->name = name;
		      Py_XDECREF(tmp);
	      }

	      if (s_value)
	      {
		      tmp = self->s_value;
		      Py_INCREF(s_value);
		      self->s_value = s_value;
		      Py_XDECREF(tmp);
	      }

	      if (n_value_string)
	      {
		      tmp = self->n_value_string;
		      Py_INCREF(n_value_string);
		      self->n_value_string = n_value_string;
		      Py_XDECREF(tmp);
	      }

	      if (last_update_string)
	      {
		      tmp = self->last_update_string;
		      Py_INCREF(last_update_string);
		      self->last_update_string = last_update_string;
		      Py_XDECREF(tmp);
	      }

	      return 0;
      }

      PyObject *
      PDevice_Describe(PDevice* self)
      {
          return PyUnicode_FromFormat("ID: %i Name: %S, Type: %i, subType: %i, switchType: %i, s_value: %S, n_value: %i, n_value_string: %S, last_update_string: %S", self->id, self->name, self->type, self->sub_type, self->switch_type, self->s_value, self->n_value, self->n_value_string, self->last_update_string);
      }

    #ifdef BUILD_MODULE
      PyMODINIT_FUNC
      PyInit_DomoticzEvents(
          void)
      {
          PyObject* m;

          if (PyType_Ready(&PDeviceType) < 0)
		  return nullptr;

	  m = PyModule_Create(&PDevicemodule);
	  if (m == nullptr)
		  return nullptr;

	  Py_INCREF(&PDeviceType);
	  PyModule_AddObject(m, "PDevice", (PyObject *)&PDeviceType);
	  return m;
      }
    #endif
    } // namespace Plugins
#endif
