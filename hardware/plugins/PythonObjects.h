#pragma once

#include "Plugins.h"
#include "DelayedLink.h"

namespace Plugins {

	typedef struct {
		PyObject_HEAD
		int			ImageID;
		PyObject*	Base;
		PyObject*	Name;
		PyObject*	Description;
		PyObject*	Filename;
		CPlugin*	pPlugin;
	} CImage;

	void CImage_dealloc(CImage* self);
	PyObject* CImage_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
	int CImage_init(CImage *self, PyObject *args, PyObject *kwds);
	PyObject* CImage_insert(CImage* self, PyObject *args);
	PyObject* CImage_delete(CImage* self, PyObject *args);
	PyObject* CImage_str(CImage* self);

	static PyMemberDef CImage_members[] = {
		{ "ID",	T_INT, offsetof(CImage, ImageID), READONLY, "Domoticz internal Custom Image Number" },
		{ "Name", T_OBJECT,	offsetof(CImage, Name), READONLY, "Name" },
		{ "Base", T_OBJECT, offsetof(CImage, Base), READONLY, "Base name, must start with plugin Key" },
		{ "Description", T_INT, offsetof(CImage, Description), READONLY, "Description" },
		{ NULL }  /* Sentinel */
	};

	static PyMethodDef CImage_methods[] = {
		{ "Create", (PyCFunction)CImage_insert, METH_NOARGS, "Create the device in Domoticz." },
		{ "Delete", (PyCFunction)CImage_delete, METH_NOARGS, "Delete the device in Domoticz." },
		{ NULL }  /* Sentinel */
	};

	static PyTypeObject CImageType = {
		PyVarObject_HEAD_INIT(NULL, 0)
		"Domoticz.Image",             /* tp_name */
		sizeof(CImage),             /* tp_basicsize */
		0,                         /* tp_itemsize */
		(destructor)CImage_dealloc, /* tp_dealloc */
		0,                         /* tp_print */
		0,                         /* tp_getattr */
		0,                         /* tp_setattr */
		0,                         /* tp_reserved */
		0,                         /* tp_repr */
		0,                         /* tp_as_number */
		0,                         /* tp_as_sequence */
		0,                         /* tp_as_mapping */
		0,                         /* tp_hash  */
		0,                         /* tp_call */
		(reprfunc)CImage_str,     /* tp_str */
		0,                         /* tp_getattro */
		0,                         /* tp_setattro */
		0,                         /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_BASETYPE,   /* tp_flags */
		"Domoticz Image",           /* tp_doc */
		0,                         /* tp_traverse */
		0,                         /* tp_clear */
		0,                         /* tp_richcompare */
		0,                         /* tp_weaklistoffset */
		0,                         /* tp_iter */
		0,                         /* tp_iternext */
		CImage_methods,             /* tp_methods */
		CImage_members,             /* tp_members */
		0,                         /* tp_getset */
		0,                         /* tp_base */
		0,                         /* tp_dict */
		0,                         /* tp_descr_get */
		0,                         /* tp_descr_set */
		0,                         /* tp_dictoffset */
		(initproc)CImage_init,      /* tp_init */
		0,                         /* tp_alloc */
		CImage_new                 /* tp_new */
	};


	typedef struct {
		PyObject_HEAD
			PyObject*	PluginKey;
		int			HwdID;
		PyObject*	DeviceID;
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
		int			TimedOut;
		CPlugin*	pPlugin;
	} CDevice;

	void CDevice_dealloc(CDevice* self);
	PyObject* CDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
	int CDevice_init(CDevice *self, PyObject *args, PyObject *kwds);
	PyObject* CDevice_refresh(CDevice* self);
	PyObject* CDevice_insert(CDevice* self);
	PyObject* CDevice_update(CDevice *self, PyObject *args, PyObject *kwds);
	PyObject* CDevice_delete(CDevice* self);
	PyObject* CDevice_str(CDevice* self);

	static PyMemberDef CDevice_members[] = {
		{ "ID",	T_INT, offsetof(CDevice, ID), READONLY, "Domoticz internal ID" },
		{ "Name", T_OBJECT,	offsetof(CDevice, Name), READONLY, "Name" },
		{ "DeviceID", T_OBJECT,	offsetof(CDevice, DeviceID), READONLY, "External device ID" },
		{ "nValue", T_INT, offsetof(CDevice, nValue), READONLY, "Numeric device value" },
		{ "sValue", T_OBJECT, offsetof(CDevice, sValue), READONLY, "String device value" },
		{ "SignalLevel", T_INT, offsetof(CDevice, SignalLevel), READONLY, "Numeric signal level" },
		{ "BatteryLevel", T_INT, offsetof(CDevice, BatteryLevel), READONLY, "Numeric battery level" },
		{ "Image", T_INT, offsetof(CDevice, Image), READONLY, "Numeric image number" },
		{ "Type", T_INT, offsetof(CDevice, Type), READONLY, "Numeric device type" },
		{ "SubType", T_INT, offsetof(CDevice, SubType), READONLY, "Numeric device subtype" },
		{ "LastLevel", T_INT, offsetof(CDevice, LastLevel), READONLY, "Previous device level" },
		{ "LastUpdate", T_OBJECT, offsetof(CDevice, LastUpdate), READONLY, "Last update timestamp" },
		{ "Options", T_OBJECT, offsetof(CDevice, Options), READONLY, "Device options" },
		{ "Used", T_INT, offsetof(CDevice, Used), READONLY, "Numeric device Used flag" },
		{ "TimedOut", T_INT, offsetof(CDevice, TimedOut), READONLY, "Is the device marked as timed out" },
		{ NULL }  /* Sentinel */
	};

	static PyMethodDef CDevice_methods[] = {
		{ "Refresh", (PyCFunction)CDevice_refresh, METH_NOARGS, "Refresh device details" },
		{ "Create", (PyCFunction)CDevice_insert, METH_NOARGS, "Create the device in Domoticz." },
		{ "Update", (PyCFunction)CDevice_update, METH_VARARGS | METH_KEYWORDS, "Update the device values in Domoticz." },
		{ "Delete", (PyCFunction)CDevice_delete, METH_NOARGS, "Delete the device in Domoticz." },
		{ NULL }  /* Sentinel */
	};

	static PyTypeObject CDeviceType = {
		PyVarObject_HEAD_INIT(NULL, 0)
		"Domoticz.Device",             /* tp_name */
		sizeof(CDevice),             /* tp_basicsize */
		0,                         /* tp_itemsize */
		(destructor)CDevice_dealloc, /* tp_dealloc */
		0,                         /* tp_print */
		0,                         /* tp_getattr */
		0,                         /* tp_setattr */
		0,                         /* tp_reserved */
		0,                         /* tp_repr */
		0,                         /* tp_as_number */
		0,                         /* tp_as_sequence */
		0,                         /* tp_as_mapping */
		0,                         /* tp_hash  */
		0,                         /* tp_call */
		(reprfunc)CDevice_str,     /* tp_str */
		0,                         /* tp_getattro */
		0,                         /* tp_setattro */
		0,                         /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_BASETYPE,   /* tp_flags */
		"Domoticz Device",           /* tp_doc */
		0,                         /* tp_traverse */
		0,                         /* tp_clear */
		0,                         /* tp_richcompare */
		0,                         /* tp_weaklistoffset */
		0,                         /* tp_iter */
		0,                         /* tp_iternext */
		CDevice_methods,             /* tp_methods */
		CDevice_members,             /* tp_members */
		0,                         /* tp_getset */
		0,                         /* tp_base */
		0,                         /* tp_dict */
		0,                         /* tp_descr_get */
		0,                         /* tp_descr_set */
		0,                         /* tp_dictoffset */
		(initproc)CDevice_init,      /* tp_init */
		0,                         /* tp_alloc */
		CDevice_new                 /* tp_new */
	};

	class CPluginTransport;
	class CPluginProtocol;

	typedef struct {
		PyObject_HEAD
		PyObject*			Name;
		PyObject*			Address;
		PyObject*			Port;
		int					Baud;
		PyObject*			LastSeen;
		CPlugin*			pPlugin;
		PyObject*			Transport;
		CPluginTransport*	pTransport;
		PyObject*			Protocol;
		CPluginProtocol*	pProtocol;
	} CConnection;

	void CConnection_dealloc(CConnection* self);
	PyObject* CConnection_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
	int CConnection_init(CConnection *self, PyObject *args, PyObject *kwds);
	PyObject* CConnection_connect(CConnection* self);
	PyObject* CConnection_listen(CConnection* self);
	PyObject* CConnection_send(CConnection *self, PyObject *args, PyObject *kwds);
	PyObject* CConnection_disconnect(CConnection* self);
	PyObject* CConnection_bytes(CConnection* self);
	PyObject* CConnection_isconnecting(CConnection* self);
	PyObject* CConnection_isconnected(CConnection* self);
	PyObject* CConnection_timestamp(CConnection* self);
	PyObject* CConnection_str(CConnection* self);

	static PyMemberDef CConnection_members[] = {
		{ "Name", T_OBJECT,	offsetof(CConnection, Name), READONLY, "Name" },
		{ "Address", T_OBJECT,	offsetof(CConnection, Address), READONLY, "Address" },
		{ "Port", T_OBJECT,	offsetof(CConnection, Port), READONLY, "Port" },
		{ "Baud", T_INT, offsetof(CConnection, Baud), READONLY, "Baud" },
		{ NULL }  /* Sentinel */
	};

	static PyMethodDef CConnection_methods[] = {
		{ "Connect", (PyCFunction)CConnection_connect, METH_NOARGS, "Connect to specified Address/Port)" },
		{ "Send", (PyCFunction)CConnection_send, METH_VARARGS | METH_KEYWORDS, "Send data to connection." },
		{ "Listen", (PyCFunction)CConnection_listen, METH_NOARGS, "Listen on specified Port." },
		{ "Disconnect", (PyCFunction)CConnection_disconnect, METH_NOARGS, "Disconnect connection or stop listening." },
		{ "BytesTransferred", (PyCFunction)CConnection_bytes, METH_NOARGS, "Bytes transferred since connection was opened." },
		{ "Connecting", (PyCFunction)CConnection_isconnecting, METH_NOARGS, "Connection in progress." },
		{ "Connected", (PyCFunction)CConnection_isconnected, METH_NOARGS, "Connection status." },
		{ "LastSeen", (PyCFunction)CConnection_timestamp, METH_NOARGS, "Last seen timestamp." },
		{ NULL }  /* Sentinel */
	};

	static PyTypeObject CConnectionType = {
		PyVarObject_HEAD_INIT(NULL, 0)
		"Domoticz.Connection",             /* tp_name */
		sizeof(CConnection),             /* tp_basicsize */
		0,                         /* tp_itemsize */
		(destructor)CConnection_dealloc, /* tp_dealloc */
		0,                         /* tp_print */
		0,                         /* tp_getattr */
		0,                         /* tp_setattr */
		0,                         /* tp_reserved */
		0,                         /* tp_repr */
		0,                         /* tp_as_number */
		0,                         /* tp_as_sequence */
		0,                         /* tp_as_mapping */
		0,                         /* tp_hash  */
		0,                         /* tp_call */
		(reprfunc)CConnection_str,     /* tp_str */
		0,                         /* tp_getattro */
		0,                         /* tp_setattro */
		0,                         /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_BASETYPE,   /* tp_flags */
		"Domoticz Connection",           /* tp_doc */
		0,                         /* tp_traverse */
		0,                         /* tp_clear */
		0,                         /* tp_richcompare */
		0,                         /* tp_weaklistoffset */
		0,                         /* tp_iter */
		0,                         /* tp_iternext */
		CConnection_methods,             /* tp_methods */
		CConnection_members,             /* tp_members */
		0,                         /* tp_getset */
		0,                         /* tp_base */
		0,                         /* tp_dict */
		0,                         /* tp_descr_get */
		0,                         /* tp_descr_set */
		0,                         /* tp_dictoffset */
		(initproc)CConnection_init,      /* tp_init */
		0,                         /* tp_alloc */
		CConnection_new                 /* tp_new */
	};
}
