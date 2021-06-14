#pragma once

#include "Plugins.h"
#include "PythonObjects.h"
#include "DelayedLink.h"

namespace Plugins {

	class CDeviceEx
	{
	public:
		PyObject_HEAD
		PyObject*		DeviceID;
		int				TimedOut;
		PyDictObject*	m_UnitDict;
	};

	void CDeviceEx_dealloc(CDeviceEx *self);
	PyObject *CDeviceEx_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
	int CDeviceEx_init(CDeviceEx *self, PyObject *args, PyObject *kwds);
	PyObject *CDeviceEx_refresh(CDeviceEx *self);
	PyObject *CDeviceEx_str(CDeviceEx *self);

	static PyMemberDef CDeviceEx_members[] = {
		{ "DeviceID", T_OBJECT, offsetof(CDeviceEx, DeviceID), READONLY, "External device ID" },
		{ "TimedOut", T_INT, offsetof(CDeviceEx, TimedOut), 0, "Time out state" },
		{ "Units", T_OBJECT, offsetof(CDeviceEx, m_UnitDict), READONLY, "Units dictionary" },
		{ "Key", T_OBJECT, offsetof(CDeviceEx, DeviceID), READONLY, "Devices dictionary key" },
		{ nullptr } /* Sentinel */
	};

	static PyMethodDef CDeviceEx_methods[] = {
		{ "Refresh", (PyCFunction)CDeviceEx_refresh, METH_NOARGS, "Refresh the device and it's units" },
		{ nullptr } /* Sentinel */
	};

	static PyTypeObject CDeviceExType = {
		PyVarObject_HEAD_INIT(nullptr, 0) "DomoticzEx.Device", /* tp_name */
		sizeof(CDeviceEx),				/* tp_basicsize */
		0,								/* tp_itemsize */
		(destructor)CDeviceEx_dealloc,	/* tp_dealloc */
		0,								/* tp_print */
		nullptr,					    /* tp_getattr */
		nullptr,					    /* tp_setattr */
		nullptr,					    /* tp_reserved */
		nullptr,					    /* tp_repr */
		nullptr,					    /* tp_as_number */
		nullptr,					    /* tp_as_sequence */
		nullptr,					    /* tp_as_mapping */
		nullptr,					    /* tp_hash  */
		nullptr,					    /* tp_call */
		(reprfunc)CDeviceEx_str,		/* tp_str */
		nullptr,					    /* tp_getattro */
		nullptr,					    /* tp_setattro */
		nullptr,					    /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	     /* tp_flags */
		"DomoticzEx Device",			/* tp_doc */
		nullptr,					    /* tp_traverse */
		nullptr,					    /* tp_clear */
		nullptr,					    /* tp_richcompare */
		0,								/* tp_weaklistoffset */
		nullptr,					    /* tp_iter */
		nullptr,					    /* tp_iternext */
		CDeviceEx_methods,				/* tp_methods */
		CDeviceEx_members,				/* tp_members */
		nullptr,					    /* tp_getset */
		nullptr,					    /* tp_base */
		nullptr,					    /* tp_dict */
		nullptr,					    /* tp_descr_get */
		nullptr,					    /* tp_descr_set */
		0,								/* tp_dictoffset */
		(initproc)CDeviceEx_init,		/* tp_init */
		nullptr,					    /* tp_alloc */
		CDeviceEx_new					/* tp_new */
	};

	class CUnitEx
	{
	public:
		PyObject_HEAD
		int			Unit;
		int			Type;
		int			SubType;
		int			SwitchType;
		int			ID;
		int			LastLevel;
		PyObject*	Name;
		PyObject*	LastUpdate;
		int			nValue;
		int			SignalLevel;
		int			BatteryLevel;
		PyObject*	sValue;
		int			Image;
		PyObject*	Options;
		int			Used;
		PyObject*	Description;
		PyObject*	Color;
		PyObject*	Parent;
	};

	void CUnitEx_dealloc(CUnitEx *self);
	PyObject *CUnitEx_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
	int CUnitEx_init(CUnitEx *self, PyObject *args, PyObject *kwds);
	PyObject *CUnitEx_refresh(CUnitEx *self);
	PyObject *CUnitEx_insert(CUnitEx *self);
	PyObject *CUnitEx_update(CUnitEx *self, PyObject *args, PyObject *kwds);
	PyObject *CUnitEx_delete(CUnitEx *self);
	PyObject *CUnitEx_touch(CUnitEx *self);
	PyObject *CUnitEx_str(CUnitEx *self);

	static PyMemberDef CUnitEx_members[] = {
		{ "ID", T_INT, offsetof(CUnitEx, ID), READONLY, "Domoticz internal ID" },
		{ "Unit", T_INT, offsetof(CUnitEx, Unit), READONLY, "Numeric Unit number" },
		{ "Name", T_STRING, offsetof(CUnitEx, Name), 0, "Name" },
		{ "nValue", T_INT, offsetof(CUnitEx, nValue), 0, "Numeric device value" },
		{ "sValue", T_STRING, offsetof(CUnitEx, sValue), 0, "String device value" },
		{ "SignalLevel", T_INT, offsetof(CUnitEx, SignalLevel), 0, "Numeric signal level" },
		{ "BatteryLevel", T_INT, offsetof(CUnitEx, BatteryLevel), 0, "Numeric battery level" },
		{ "Image", T_INT, offsetof(CUnitEx, Image), 0, "Numeric image number" },
		{ "Type", T_INT, offsetof(CUnitEx, Type), 0, "Numeric device type" },
		{ "SubType", T_INT, offsetof(CUnitEx, SubType), 0, "Numeric device subtype" },
		{ "SwitchType", T_INT, offsetof(CUnitEx, SwitchType), 0, "Numeric device switchtype" },
		{ "LastLevel", T_INT, offsetof(CUnitEx, LastLevel), READONLY, "Previous device level" },
		{ "LastUpdate", T_STRING, offsetof(CUnitEx, LastUpdate), READONLY, "Last update timestamp" },
		{ "Options", T_OBJECT, offsetof(CUnitEx, Options), READONLY, "Device options" },
		{ "Used", T_INT, offsetof(CUnitEx, Used), 0, "Numeric device Used flag" },
		{ "Description", T_STRING, offsetof(CUnitEx, Description), 0, "Description" },
		{ "Color", T_OBJECT, offsetof(CUnitEx, Color), 0, "Color JSON dictionary" },
		{ "Parent", T_OBJECT, offsetof(CUnitEx, Parent), READONLY, "Parent device" },
		{ nullptr } /* Sentinel */
	};

	static PyMethodDef CUnitEx_methods[] = {
		{ "Refresh", (PyCFunction)CUnitEx_refresh, METH_NOARGS, "Refresh device details" },
		{ "Create", (PyCFunction)CUnitEx_insert, METH_NOARGS, "Create the device in Domoticz." },
		{ "Update", (PyCFunction)CUnitEx_update, METH_VARARGS | METH_KEYWORDS, "Update the device values in Domoticz." },
		{ "Delete", (PyCFunction)CUnitEx_delete, METH_NOARGS, "Delete the device in Domoticz." },
		{ "Touch", (PyCFunction)CUnitEx_touch, METH_NOARGS, "Notify Domoticz that device has been seen." },
		{ nullptr } /* Sentinel */
	};

	static PyTypeObject CUnitExType = {
		PyVarObject_HEAD_INIT(nullptr, 0) "DomoticzEx.Unit", /* tp_name */
		sizeof(CUnitEx),				/* tp_basicsize */
		0,								/* tp_itemsize */
		(destructor)CUnitEx_dealloc,	/* tp_dealloc */
		0,								/* tp_print */
		nullptr,					    /* tp_getattr */
		nullptr,					    /* tp_setattr */
		nullptr,					    /* tp_reserved */
		nullptr,					    /* tp_repr */
		nullptr,					    /* tp_as_number */
		nullptr,					    /* tp_as_sequence */
		nullptr,					    /* tp_as_mapping */
		nullptr,					    /* tp_hash  */
		nullptr,					    /* tp_call */
		(reprfunc)CUnitEx_str,			/* tp_str */
		nullptr,					    /* tp_getattro */
		nullptr,					    /* tp_setattro */
		nullptr,					    /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	     /* tp_flags */
		"DomoticzEx Unit",				/* tp_doc */
		nullptr,					    /* tp_traverse */
		nullptr,					    /* tp_clear */
		nullptr,					    /* tp_richcompare */
		0,								/* tp_weaklistoffset */
		nullptr,					    /* tp_iter */
		nullptr,					    /* tp_iternext */
		CUnitEx_methods,				/* tp_methods */
		CUnitEx_members,				/* tp_members */
		nullptr,					    /* tp_getset */
		nullptr,					    /* tp_base */
		nullptr,					    /* tp_dict */
		nullptr,					    /* tp_descr_get */
		nullptr,					    /* tp_descr_set */
		0,								/* tp_dictoffset */
		(initproc)CUnitEx_init,			/* tp_init */
		nullptr,					    /* tp_alloc */
		CUnitEx_new					    /* tp_new */
	};
} // namespace Plugins
