#include "stdafx.h"

#include "EventsPythonDevice.h"

namespace Plugins {



  void
  PDevice_dealloc(PDevice* self)
  {
      Py_XDECREF(self->name);
      Py_XDECREF(self->n_value_string);
      Py_TYPE(self)->tp_free((PyObject*)self);
  }

  PyObject *
  PDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
      PDevice *self;

      self = (PDevice *)type->tp_alloc(type, 0);
      if (self != NULL) {
          self->name = PyUnicode_FromString("");
          if (self->name == NULL) {
              Py_DECREF(self);
              return NULL;
          }

          self->n_value_string = PyUnicode_FromString("");
          if (self->n_value_string == NULL) {
              Py_DECREF(self);
              return NULL;
          }

          self->id = 0;
      }

      return (PyObject *)self;
  }

  int
  PDevice_init(PDevice *self, PyObject *args, PyObject *kwds)
  {
      PyObject *name=NULL, *n_value_string=NULL, *tmp;

      static char *kwlist[] = {"name", "n_value_string", "id", NULL};

      if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist,
                                        &name, &n_value_string,
                                        &self->id))
          return -1;

      if (name) {
          tmp = self->name;
          Py_INCREF(name);
          self->name = name;
          Py_XDECREF(tmp);
      }

      if (n_value_string) {
          tmp = self->n_value_string;
          Py_INCREF(n_value_string);
          self->n_value_string = n_value_string;
          Py_XDECREF(tmp);
      }

      return 0;
  }

  PyObject *
  PDevice_Describe(PDevice* self)
  {
      return PyUnicode_FromFormat("%i %S %S", self->id, self->name, self->n_value_string);
  }

#ifdef BUILD_MODULE
  PyMODINIT_FUNC
  PyInit_DomoticzEvents(
      void)
  {
      PyObject* m;

      if (PyType_Ready(&PDeviceType) < 0)
          return NULL;

      m = PyModule_Create(&PDevicemodule);
      if (m == NULL)
          return NULL;

      Py_INCREF(&PDeviceType);
      PyModule_AddObject(m, "PDevice", (PyObject *)&PDeviceType);
      return m;
  }
#endif
}
