#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "PluginManager.h"
#include "Plugins.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/EventSystem.h"
#include "../hardware/hardwaretypes.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include "../json/json.h"
#include "../tinyxpath/tinyxml.h"
#include "../main/localtime_r.h"
#ifdef WIN32
#	include <direct.h>
#else
#	include <sys/stat.h>
#endif

#ifdef WIN32
#	define MS_NO_COREDLL 1
#else
#	pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#include <Python.h>
#include <structmember.h> 
#include <frameobject.h>
#include "DelayedLink.h"

#define MINIMUM_PYTHON_VERSION "3.2.0"

#define ATTRIBUTE_VALUE(pElement, Name, Value) \
		{	\
			Value = ""; \
			const char*	pAttributeValue = NULL;	\
			if (pElement) pAttributeValue = pElement->Attribute(Name);	\
			if (pAttributeValue) Value = pAttributeValue;	\
		}

#define ATTRIBUTE_NUMBER(pElement, Name, Value) \
		{	\
			Value = 0; \
			const char*	pAttributeValue = NULL;	\
			if (pElement) pAttributeValue = pElement->Attribute(Name);	\
			if (pAttributeValue) Value = atoi(pAttributeValue);	\
		}

#define ADD_STRING_TO_DICT(pDict, key, value) \
		{	\
			PyObject*	pObj = Py_BuildValue("s", value.c_str());	\
			if (PyDict_SetItemString(pDict, key, pObj) == -1)	\
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", m_PluginKey.c_str(), key, value.c_str());	\
			Py_DECREF(pObj); \
		}

extern std::string szUserDataFolder;

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

namespace Plugins {

	boost::mutex PythonMutex;	// only used during startup when multiple threads could use Python
	boost::mutex PluginMutex;	// controls accessto the message queue
	std::queue<CPluginMessage>	PluginMessageQueue;
	boost::asio::io_service ios;

	//
	//	Holds per plugin state details, specifically plugin object, read using PyModule_GetState(PyObject *module)
	//
	struct module_state {
		CPlugin*	pPlugin;
		PyObject*	error;
	};

	void LogPythonException(CPlugin* pPlugin, const std::string &sHandler)
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;
		const char*			pTypeText = NULL;
		std::string			Name = "Unknown";

		if (pPlugin) Name = pPlugin->Name;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			pTypeText = TypeName->tp_name;
		}
		if (pValue)
		{
			pErrBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pValue);
		}
		if (pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s':'%s'.", Name.c_str(), sHandler.c_str(), pTypeText, pErrBytes->ob_sval);
		}
		if (pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pTypeText);
		}
		if (!pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pErrBytes->ob_sval);
		}
		if (!pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed, unable to determine error.", Name.c_str(), sHandler.c_str());
		}
		if (pErrBytes) Py_XDECREF(pErrBytes);

		// Log a stack trace if there is one
		while (pTraceback)
		{
			PyFrameObject *frame = pTraceback->tb_frame;
			if (frame)
			{
				int lineno = PyFrame_GetLineNumber(frame);
				PyCodeObject*	pCode = frame->f_code;
				PyBytesObject*	pFileBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pCode->co_name);
				_log.Log(LOG_ERROR, "(%s) ----> Line %d in %s, function %s", Name.c_str(), lineno, pFileBytes->ob_sval, pFuncBytes->ob_sval);
				Py_XDECREF(pFileBytes);
				Py_XDECREF(pFuncBytes);
			}
			pTraceback = pTraceback->tb_next;
		}

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to message handler '%s' failed, unable to decode exception.", Name.c_str(), sHandler.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	static PyObject*	PyDomoticz_Debug(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debug, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debug, illegal operation, Plugin has not started yet.");
		}
		else
		{
			if (pModState->pPlugin->m_bDebug)
			{
				char* msg;
				if (!PyArg_ParseTuple(args, "s", &msg))
				{
					_log.Log(LOG_ERROR, "(%s) PyDomoticz_Debug failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
					return NULL;
				}

				std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_NORM, message.c_str());
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Log(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Log, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Log, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char* msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Log failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
				return NULL;
			}

			std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
			_log.Log((_eLogLevel)LOG_NORM, message.c_str());
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Error(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Error, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Error, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char* msg;
			if (!PyArg_ParseTuple(args, "s", &msg))
			{
				_log.Log(LOG_ERROR, "(%s) PyDomoticz_Error failed to parse parameters: string expected.", pModState->pPlugin->Name.c_str());
				return NULL;
			}

			std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
			_log.Log((_eLogLevel)LOG_ERROR, message.c_str());
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Debugging(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debugging, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Debugging, illegal operation, Plugin has not started yet.");
		}
		else
		{
			int		type;
			if (!PyArg_ParseTuple(args, "i", &type))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->Name.c_str());
			}
			else
			{
				type ? pModState->pPlugin->m_bDebug = true : pModState->pPlugin->m_bDebug = false;
				_log.Log(LOG_NORM, "(%s) Debug log level set to: '%s'.", pModState->pPlugin->Name.c_str(), (type ? "true" : "false"));
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Transport(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Transport, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Transport, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char*	szTransport;
			char*	szAddress;
			char*	szPort;
			int		iBaud = 0;
			if (!PyArg_ParseTuple(args, "sss", &szTransport, &szAddress, &szPort))
			{
				PyErr_Clear();
				if (!PyArg_ParseTuple(args, "ssi", &szTransport, &szPort, &iBaud))
				{
					_log.Log(LOG_ERROR, "(%s) failed to parse parameters. Expected: Transport, IP Address, Port (sss) or Transport, SerialPort, Baud (ssi).", pModState->pPlugin->Name.c_str());
					Py_INCREF(Py_None);
					return Py_None;
				}
			}

			//	Add start command to message queue
			std::string	sTransport = szTransport;
			CPluginMessage	Message(PMT_Directive, PDT_Transport, pModState->pPlugin->m_HwdID, sTransport);
			{
				Message.m_Address = szAddress;
				Message.m_Port = szPort;
				Message.m_iValue = iBaud;
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Protocol(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Protocol, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Protocol, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char*	szProtocol;
			if (!PyArg_ParseTuple(args, "s", &szProtocol))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, string expected.", pModState->pPlugin->Name.c_str());
			}
			else
			{
				//	Add start command to message queue
				std::string	sProtocol = szProtocol;
				CPluginMessage	Message(PMT_Directive, PDT_Protocol, pModState->pPlugin->m_HwdID, sProtocol);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Heartbeat(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Heartbeat, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Heartbeat, illegal operation, Plugin has not started yet.");
		}
		else
		{
			int	iPollinterval;
			if (!PyArg_ParseTuple(args, "i", &iPollinterval))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, integer expected.", pModState->pPlugin->Name.c_str());
			}
			else
			{
				//	Add heartbeat command to message queue
				CPluginMessage	Message(PMT_Directive, PDT_PollInterval, pModState->pPlugin->m_HwdID, iPollinterval);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Connect(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Connect, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Connect, illegal operation, Plugin has not started yet.");
		}
		else
		{
			//	Add connect command to message queue unless already connected
			if ((!pModState->pPlugin->m_pTransport) || (!pModState->pPlugin->m_pTransport->IsConnected()))
			{
				CPluginMessage	Message(PMT_Directive, PDT_Connect, pModState->pPlugin->m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
			else
				_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Connect, connection request from '%s' ignored. Transport is already connected.", pModState->pPlugin->Name.c_str());
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Send(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Send, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Send, illegal operation, Plugin has not started yet.");
		}
		else
		{
			char*	szMessage;
			if (!PyArg_ParseTuple(args, "s", &szMessage))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, string expected.", pModState->pPlugin->Name.c_str());
			}
			else
			{
				//	Add start command to message queue
				std::string	sMessage = szMessage;
				CPluginMessage	Message(PMT_Directive, PDT_Write, pModState->pPlugin->m_HwdID, sMessage);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Disconnect(PyObject *self, PyObject *args)
	{
		module_state*	pModState = ((struct module_state*)PyModule_GetState(self));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Disconnect, unable to obtain module state.");
		}
		else if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Disconnect, illegal operation, Plugin has not started yet.");
		}
		else
		{
			//	Add disconnect command to message queue
			if ((pModState->pPlugin->m_pTransport) && (pModState->pPlugin->m_pTransport->IsConnected()))
			{
				CPluginMessage	Message(PMT_Directive, PDT_Disconnect, pModState->pPlugin->m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
			else
				_log.Log(LOG_ERROR, "CPlugin:PyDomoticz_Disconnect, disconnect request from '%s' ignored. Transport is not connected.", pModState->pPlugin->Name.c_str());
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyMethodDef DomoticzMethods[] = {
		{ "Debug", PyDomoticz_Debug, METH_VARARGS, "Write message to Domoticz log only if verbose logging is turned on." },
		{ "Log", PyDomoticz_Log, METH_VARARGS, "Write message to Domoticz log." },
		{ "Error", PyDomoticz_Error, METH_VARARGS, "Write error message to Domoticz log." },
		{ "Debugging", PyDomoticz_Debugging, METH_VARARGS, "Set logging level. 1 set verbose logging, all other values use default level" },
		{ "Transport", PyDomoticz_Transport, METH_VARARGS, "Set the communication transport: TCP/IP, Serial." },
		{ "Protocol", PyDomoticz_Protocol, METH_VARARGS, "Set the protocol the messages will use: None, HTTP." },
		{ "Heartbeat", PyDomoticz_Heartbeat, METH_VARARGS, "Set the heartbeat interval, default 10 seconds." },
		{ "Connect", PyDomoticz_Connect, METH_NOARGS, "Connect to remote device using transport details." },
		{ "Send", PyDomoticz_Send, METH_VARARGS, "Send the specified message to the remote device." },
		{ "Disconnect", PyDomoticz_Disconnect, METH_NOARGS, "Disconnectfrom remote device." },
		{ NULL, NULL, 0, NULL }
	};

	static int DomoticzTraverse(PyObject *m, visitproc visit, void *arg) {
		Py_VISIT(GETSTATE(m)->error);
		return 0;
	}

	static int DomoticzClear(PyObject *m) {
		Py_CLEAR(GETSTATE(m)->error);
		return 0;
	}

	static struct PyModuleDef DomoticzModuleDef = {
		PyModuleDef_HEAD_INIT,
		"Domoticz",
		NULL,
		sizeof(struct module_state),
		DomoticzMethods,
		NULL,
		DomoticzTraverse,
		DomoticzClear,
		NULL
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
		int			nValue;
		PyObject*	sValue;
		int			Image;
		PyObject*	Options;
		CPlugin*	pPlugin;
	} CDevice;

	static void CDevice_dealloc(CDevice* self)
	{
		Py_XDECREF(self->Name);
		Py_XDECREF(self->sValue);
		Py_TYPE(self)->tp_free((PyObject*)self);
	}

	static PyObject* CDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		CDevice *self = (CDevice *)type->tp_alloc(type, 0);

		try
		{
			if (self == NULL) {
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
//				_log.Log(LOG_NORM, "CPlugin:%s, calling PyUnicode_FromString.", __func__);
				self->PluginKey = pythonLib->PyUnicode_FromString("");
//				_log.Log(LOG_NORM, "CPlugin:%s, PyUnicode_FromString returned.", __func__);
				if (self->PluginKey == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->HwdID = -1;
				self->DeviceID = pythonLib->PyUnicode_FromString("");
				if (self->DeviceID == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Unit = -1;
				self->Type = 0;
				self->SubType = 0;
				self->SwitchType = 0;
				self->ID = -1;
				self->LastLevel;
				self->Name = pythonLib->PyUnicode_FromString("");
				if (self->Name == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->nValue = 0;
				self->sValue = pythonLib->PyUnicode_FromString("");
				if (self->sValue == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Options = pythonLib->PyUnicode_FromString("");
				if (self->Options == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Image = 0;
				self->pPlugin = NULL;
			}
		}
		catch (std::exception e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e.what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}

		return (PyObject *)self;
	}

	static int CDevice_init(CDevice *self, PyObject *args, PyObject *kwds)
	{
		char*		Name = NULL;
		int			Unit = -1;
		int			Type = -1;
		int			SubType = -1;
		int			SwitchType = -1;
		int			Image = -1;
		char*		Options = NULL;
		static char *kwlist[] = { "Name", "Unit", "Type", "Subtype", "Switchtype", "Image", "Options", NULL };

		try
		{
			PyObject*	pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, unable to find module for current interpreter.", __func__);
				return 0;
			}

			module_state*	pModState = ((struct module_state*)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", __func__);
				return 0;
			}

			if (!pModState->pPlugin)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
				return 0;
			}

			if (PyArg_ParseTupleAndKeywords(args, kwds, "si|iiiis", kwlist, &Name, &Unit, &Type, &SubType, &SwitchType, &Image, &Options))
			{
				self->pPlugin = pModState->pPlugin;
				self->PluginKey = pythonLib->PyUnicode_FromString(pModState->pPlugin->m_PluginKey.c_str());
				self->HwdID = pModState->pPlugin->m_HwdID;
				if (Name) {
					Py_DECREF(self->Name);
					self->Name = pythonLib->PyUnicode_FromString(Name);
				}
				if (Unit != -1) self->Unit = Unit;
				if (Type != -1) self->Type = Type;
				if (SubType != -1) self->SubType = SubType;
				if (SwitchType != -1) self->SwitchType = SwitchType;
				if (Image != -1) self->Image = Image;
				if (Options) {
					Py_DECREF(self->Options);
					self->Options = pythonLib->PyUnicode_FromString(Options);
				}
			}
			else
			{
				CPlugin* pPlugin = NULL;
				if (pModState) pPlugin = pModState->pPlugin;
				LogPythonException(pPlugin, __func__);
				_log.Log(LOG_ERROR, "Expected: myVar = Domoticz.Device(Name=\"myDevice\", Unit=0, Type=0, Subtype=0, Switchtype=0, Image=0, Options=\"\")");
			}
		}
		catch (std::exception e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e.what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}

		return 0;
	}

	static PyMemberDef CDevice_members[] = {
		{ "ID",	T_INT, offsetof(CDevice, ID), READONLY, "Domoticz internal ID" },
		{ "Name", T_OBJECT,	offsetof(CDevice, Name), READONLY, "Name" },
		{ "nValue", T_INT, offsetof(CDevice, nValue), READONLY, "Numeric device value" },
		{ "sValue", T_OBJECT, offsetof(CDevice, sValue), READONLY, "String device value" },
		{ "LastLevel", T_INT, offsetof(CDevice, LastLevel), READONLY, "Previous device level" },
		{ NULL }  /* Sentinel */
	};

	static PyObject* CDevice_refresh(CDevice* self)
	{
		return NULL;
	}

	static PyObject* CDevice_insert(CDevice* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			PyObject*	pNameBytes = pythonLib->PyUnicode_AsASCIIString(self->Name);
			if (self->ID == -1)
			{
				if (self->pPlugin->m_bDebug)
				{
					_log.Log(LOG_NORM, "(%s) Creating device '%s'.", self->pPlugin->Name.c_str(), std::string(PyBytes_AsString(pNameBytes)).c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
				if (result.size() == 0)
				{
					char szID[40];
					sprintf(szID, "%X%02X%02X%02X", 0, 0, (self->HwdID & 0xFF00) >> 8, self->HwdID & 0xFF);
					PyObject*	pOptionBytes = pythonLib->PyUnicode_AsASCIIString(self->Options);
					std::string	sLongName = self->pPlugin->Name + " - " + PyBytes_AsString(pNameBytes);
					m_sql.safe_query(
						"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
						"VALUES (%d, '%q', %d, %d, %d, %d, 1, 12, 255, '%q', 0, '', %d, '%q')",
						self->HwdID, szID, self->Unit, self->Type, self->SubType, self->SwitchType, sLongName.c_str(), self->Image, std::string(PyBytes_AsString(pOptionBytes)).c_str());
					Py_DECREF(pOptionBytes);

					result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
					if (result.size())
					{
						PyObject*	pKey = PyLong_FromLong(self->Unit);
						if (PyDict_SetItem((PyObject*)self->pPlugin->m_DeviceDict, pKey, (PyObject*)self) == -1)
						{
							_log.Log(LOG_ERROR, "(%s) failed to add unit '%d' to device dictionary.", self->pPlugin->Name.c_str(), self->Unit);
							Py_INCREF(Py_None);
							return Py_None;
						}
					}
					else
					{
						_log.Log(LOG_ERROR, "(%s) Device creation failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Device creation failed, Hardware/Unit combination (%d:%d) already exists in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Device creation failed, '%s' already exists in Domoticz with Device ID '%d'.", self->pPlugin->Name.c_str(), std::string(PyBytes_AsString(pNameBytes)).c_str(), self->ID);
			}
			Py_DECREF(pNameBytes);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device creation failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_update(CDevice* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			int			nValue;
			char*		sValue;
			PyObject*	pNameBytes = pythonLib->PyUnicode_AsASCIIString(self->Name);
			if (!PyArg_ParseTuple(args, "is", &nValue, &sValue))
			{
				_log.Log(LOG_ERROR, "(%s) %s: failed to parse parameters: integer, string expected.", __func__, PyBytes_AsString(pNameBytes));
				Py_DECREF(pNameBytes);
				return NULL;
			}

			if (self->pPlugin->m_bDebug)
			{
				PyObject*	pValueBytes = pythonLib->PyUnicode_AsASCIIString(self->sValue);
				_log.Log(LOG_NORM, "(%s) Updating device from %d:'%s' to have values %d:'%s'.",
					std::string(PyBytes_AsString(pNameBytes)).c_str(), self->nValue, std::string(PyBytes_AsString(pValueBytes)).c_str(), nValue, sValue);
				Py_DECREF(pValueBytes);
			}
			PyObject*	pDeviceBytes = pythonLib->PyUnicode_AsASCIIString(self->DeviceID);
			std::string	sName = PyBytes_AsString(pNameBytes);
			m_sql.UpdateValue(self->HwdID, std::string(PyBytes_AsString(pDeviceBytes)).c_str(), (const unsigned char)self->Unit, (const unsigned char)self->Type, (const unsigned char)self->SubType, 100, 255, nValue, std::string(sValue).c_str(), sName, true);
			Py_DECREF(pNameBytes);
			Py_DECREF(pDeviceBytes);

			self->nValue = nValue;
			Py_DECREF(self->sValue);
			self->sValue = pythonLib->PyUnicode_FromString(sValue);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device update failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_seticon(CDevice* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			int			icon;
			PyObject*	pNameBytes = pythonLib->PyUnicode_AsASCIIString(self->Name);
			if (!PyArg_ParseTuple(args, "i", &icon))
			{
				_log.Log(LOG_ERROR, "(%s) CDevice_seticon: failed to parse parameters: integer expected.", PyBytes_AsString(pNameBytes));
				Py_DECREF(pNameBytes);
				return NULL;
			}

			if (self->pPlugin->m_bDebug)
			{
				_log.Log(LOG_NORM, "(%s) Updating device image to %d.", std::string(PyBytes_AsString(pNameBytes)).c_str(), icon);
			}

			time_t now = time(0);
			struct tm ltime;
			localtime_r(&now, &ltime);

			m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=%d, LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (HardwareID==%d) and (Unit==%d)",
				icon, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec, self->HwdID, self->Unit);
			Py_DECREF(pNameBytes);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device set image failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_str(CDevice* self)
	{
		PyObject*	pNameBytes = pythonLib->PyUnicode_AsASCIIString(self->Name);
		PyObject*	pValueBytes = pythonLib->PyUnicode_AsASCIIString(self->sValue);
		PyObject*	pRetVal = pythonLib->PyUnicode_FromFormat("ID: %d, Name: '%s', nValue: %d, sValue: '%s'", self->ID, PyBytes_AsString(pNameBytes), self->nValue, PyBytes_AsString(pValueBytes));
		Py_DECREF(pNameBytes);
		Py_DECREF(pValueBytes);
		return pRetVal;
	}

	static PyMethodDef CDevice_methods[] = {
		{ "Refresh", (PyCFunction)CDevice_refresh, METH_NOARGS, "Refresh device details"},
		{ "Create", (PyCFunction)CDevice_insert, METH_NOARGS, "Create a device in Domoticz." },
		{ "Update", (PyCFunction)CDevice_update, METH_VARARGS, "Update the device values in Domoticz." },
		{ "Image", (PyCFunction)CDevice_seticon, METH_VARARGS, "Set the device image in Domoticz." },
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

	static PyModuleDef CDevice_module = {
		PyModuleDef_HEAD_INIT,
		"Device",
		"Domoticz Device",
		-1,
		NULL, NULL, NULL, NULL, NULL
	};

	PyMODINIT_FUNC PyInit_Domoticz(void)
	{
		// This is called during the import of the plugin module
		// triggered by the "import Domoticz" statement
		PyObject* pModule = PyModule_Create2(&DomoticzModuleDef, PYTHON_API_VERSION);

		if (PyType_Ready(&CDeviceType) < 0)
		{
			_log.Log(LOG_ERROR, "CPlugin:PyInit_Domoticz, Device Type not ready.");
			return pModule;
		}
		Py_INCREF((PyObject *)&CDeviceType);
		PyModule_AddObject(pModule, "Device", (PyObject *)&CDeviceType);
		return pModule;
	}


	void CPluginProtocol::ProcessMessage(const int HwdID, std::string &ReadData)
	{
		// Raw protocol is to just always dispatch data to plugin without interpretation
		CPluginMessage	Message(PMT_Message, HwdID, ReadData);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	void CPluginProtocol::Flush(const int HwdID)
	{
		// Forced buffer clear, make sure the plugin gets a look at the data in case it wants it
		CPluginMessage	Message(PMT_Message, HwdID, m_sRetainedData);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
		m_sRetainedData.clear();
	}

	void CPluginProtocolLine::ProcessMessage(const int HwdID, std::string & ReadData)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

		int iPos = sData.find_first_of('\r');		//  Look for message terminator 
		while (iPos != std::string::npos)
		{
			CPluginMessage	Message(PMT_Message, HwdID, sData.substr(0, iPos));
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			if (sData[iPos + 1] == '\n') iPos++;				//  Handle \r\n 
			sData = sData.substr(iPos+1);
			iPos = sData.find_first_of('\r');
		}

		m_sRetainedData = sData; // retain any residual for next time
	}

	void CPluginProtocolJSON::ProcessMessage(const int HwdID, std::string & ReadData)
	{
		//
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in

		int iPos = 1;
		while (iPos) {
			iPos = sData.find("}{", 0) + 1;		//  Look for message separater in case there is more than one
			if (!iPos) // no, just one or part of one
			{
				if ((sData.substr(sData.length() - 1, 1) == "}") &&
					(std::count(sData.begin(), sData.end(), '{') == std::count(sData.begin(), sData.end(), '}'))) // whole message so queue the whole buffer
				{
					CPluginMessage	Message(PMT_Message, HwdID, sData);
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(Message);
					}
					sData = "";
				}
			}
			else  // more than one message so queue the first one
			{
				std::string sMessage = sData.substr(0, iPos);
				sData = sData.substr(iPos);
				CPluginMessage	Message(PMT_Message, HwdID, sMessage);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		m_sRetainedData = sData; // retain any residual for next time
	}

	void CPluginProtocolXML::ProcessMessage(const int HwdID, std::string & ReadData)
	{
		//
		//	Only returns whole XML messages. Does not handle <tag /> as the top level tag
		//	Handles the cases where a read contains a partial message or multiple messages
		//
		std::string	sData = m_sRetainedData + ReadData;  // if there was some data left over from last time add it back in
		try
		{
			while (true)
			{
				//
				//	Find the top level tag name if it is not set
				//
				if (!m_Tag.length())
				{
					int iStart = sData.find_first_of('<');
					if (iStart == std::string::npos)
					{
						// start of a tag not found so discard
						m_sRetainedData.clear();
						break;
					}
					sData = sData.substr(iStart);					// remove any leading data
					int iEnd = sData.find_first_of('>');
					if (iEnd != std::string::npos)
					{
						m_Tag = sData.substr(1, (iEnd - 1));
					}
				}

				int	iPos = sData.find("</" + m_Tag + ">");
				if (iPos != std::string::npos)
				{
					int iEnd = iPos + m_Tag.length() + 3;
					CPluginMessage	Message(PMT_Message, HwdID, sData.substr(0, iEnd));
					{
						boost::lock_guard<boost::mutex> l(PluginMutex);
						PluginMessageQueue.push(Message);
					}

					if (iEnd == sData.length())
					{
						sData.clear();
					}
					else
					{
						sData = sData.substr(++iEnd);
					}
					m_Tag = "";
				}
				else
					break;
			}
		}
		catch (std::exception const &exc)
		{
			_log.Log(LOG_ERROR, "(CPluginProtocolXML::ProcessMessage) Unexpected exception thrown '%s', Data '%s'.", exc.what(), sData.c_str());
		}

		m_sRetainedData = sData; // retain any residual for next time
	}


	void CPluginTransport::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
	}

	void CPluginTransport::handleRead(const char *data, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
	}

	bool CPluginTransportTCP::handleConnect()
	{
		try
		{
			if (!m_Socket)
			{
				m_bConnected = false;
				m_Resolver = new boost::asio::ip::tcp::resolver(ios);
				m_Socket = new boost::asio::ip::tcp::socket(ios);

				boost::system::error_code ec;
				boost::asio::ip::tcp::resolver::query query(m_IP, m_Port);
				boost::asio::ip::tcp::resolver::iterator iter = m_Resolver->resolve(query);
				boost::asio::ip::tcp::endpoint endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/example/http/client/async_client.cpp
				//
				m_Resolver->async_resolve(query, boost::bind(&CPluginTransportTCP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
				if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
				{
					ios.reset();
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}
		}
		catch (std::exception& e)
		{
//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			CPluginMessage	Message(PMT_Connected, m_HwdID, std::string(e.what()));
			Message.m_iValue = -1;
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return true;
	}

	void CPluginTransportTCP::handleAsyncResolve(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			m_Socket->async_connect(endpoint, boost::bind(&CPluginTransportTCP::handleAsyncConnect, this, boost::asio::placeholders::error, ++endpoint_iterator));
		}
		else
		{
			delete m_Resolver;
			m_Resolver = NULL;
			delete m_Socket;
			m_Socket = NULL;

//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
			CPluginMessage	Message(PMT_Connected, m_HwdID, err.message());
			Message.m_iValue = err.value();
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	void CPluginTransportTCP::handleAsyncConnect(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		delete m_Resolver;
		m_Resolver = NULL;

		if (!err)
		{
			m_bConnected = true;
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CPluginTransportTCP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
				_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
			}
		}
		else
		{
			delete m_Socket;
			m_Socket = NULL;
//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
		}

		CPluginMessage	Message(PMT_Connected, m_HwdID);
		Message.m_Message = err.message();
		Message.m_iValue = err.value();
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		if (!e)
		{
			CPluginMessage	Message(PMT_Read, m_HwdID, std::string(m_Buffer, bytes_transferred));
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_Socket)
				m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
					boost::bind(&CPluginTransportTCP::handleRead,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			if (e.value() != 1236)		// local disconnect cause by hardware reload
			{
				if ((e.value() != 2) && (e.value() != 121))	// Semaphore tmieout expiry or end of file aka 'lost contact'
					_log.Log(LOG_ERROR, "Plugin: Async Read Exception: %d, %s", e.value(), e.message().c_str());
			}
			CPluginMessage	Message(PMT_Disconnect, m_HwdID);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}
	}

	void CPluginTransportTCP::handleWrite(const std::string& pMessage)
	{
		if (m_Socket)
		{
			try
			{
				m_Socket->write_some(boost::asio::buffer(pMessage.c_str(), pMessage.length()));
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "Plugin: Socket error durring 'write_some' operation: '%s'", pMessage.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Plugin: Data not sent to NULL socket: '%s'", pMessage.c_str());
		}
	}

	bool CPluginTransportTCP::handleDisconnect()
	{
		if (m_bConnected)
		{
			if (m_Socket)
			{
				m_Socket->close();
				delete m_Socket;
				m_Socket = NULL;
			}
			m_bConnected = false;
		}
		return true;
	}

	CPluginTransportSerial::CPluginTransportSerial(int HwdID, const std::string & Port, int Baud) : CPluginTransport(HwdID), m_Baud(Baud)
	{
		m_Port = Port;
	}

	CPluginTransportSerial::~CPluginTransportSerial(void)
	{
	}

	bool CPluginTransportSerial::handleConnect()
	{
		try
		{
			if (!isOpen())
			{
				m_bConnected = false;
				open(m_Port, m_Baud,
						boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
						boost::asio::serial_port_base::character_size(8),
						boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none),
						boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

				m_bConnected = isOpen();

				CPluginMessage	Message(PMT_Connected, m_HwdID);
				if (m_bConnected)
				{
					Message.m_Message = "SerialPort " + m_Port + " opened successfully.";
					Message.m_iValue = 0;
					setReadCallback(boost::bind(&CPluginTransportSerial::handleRead, this, _1, _2));
				}
				else
				{
					Message.m_Message = "SerialPort " + m_Port + " open failed, check log for details.";
					Message.m_iValue = -1;
				}
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}
		catch (std::exception& e)
		{
			CPluginMessage	Message(PMT_Connected, m_HwdID, std::string(e.what()));
			Message.m_iValue = -1;
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return m_bConnected;
	}

	void CPluginTransportSerial::handleRead(const char *data, std::size_t bytes_transferred)
	{
		if (bytes_transferred)
		{
			CPluginMessage	Message(PMT_Read, m_HwdID, std::string(data, bytes_transferred));
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			m_iTotalBytes += bytes_transferred;
		}
		else
		{
			_log.Log(LOG_ERROR, "CPluginTransportSerial: handleRead called with no data.");
		}
	}

	void CPluginTransportSerial::handleWrite(const std::string & data)
	{
		write(data.c_str(), data.length());
	}

	bool CPluginTransportSerial::handleDisconnect()
	{
		if (m_bConnected)
		{
			if (isOpen())
			{
				terminate();
			}
			m_bConnected = false;
		}
		return true;
	}


	CPlugin::CPlugin(const int HwdID, const std::string &sName, const std::string &sPluginKey) : 
		m_stoprequested(false),
		m_pProtocol(NULL),
		m_pTransport(NULL),
		m_PluginKey(sPluginKey),
		m_iPollInterval(10),
		m_bDebug(false),
		m_PyInterpreter(NULL),
		m_PyModule(NULL)
	{
		m_HwdID = HwdID;
		Name = sName;
		m_bIsStarted = false;
	}

	CPlugin::~CPlugin(void)
	{
		if (m_pProtocol) delete m_pProtocol;
		if (m_pTransport) delete m_pTransport;

		m_bIsStarted = false;
	}

	void CPlugin::LogPythonException()
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			_log.Log(LOG_ERROR, "(%s) Module Import failed, exception: '%s'", Name.c_str(), TypeName->tp_name);
		}
		if (pValue)
		{
			std::string			sError;
			pErrBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pValue);	// Won't normally return text for Import related errors
			if (!pErrBytes)
			{
				// ImportError has name and path attributes
				if (PyObject_HasAttrString(pValue, "path"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "path");
					PyBytesObject*	pBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pString);
					if (pBytes)
					{
						sError += "Path: ";
						sError += pBytes->ob_sval;
						Py_XDECREF(pBytes);
					}
					Py_XDECREF(pString);
				}
				if (PyObject_HasAttrString(pValue, "name"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "name");
					PyBytesObject*	pBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pString);
					if (pBytes)
					{
						sError += " Name: ";
						sError += pBytes->ob_sval;
						Py_XDECREF(pBytes);
					}
					Py_XDECREF(pString);
				}
				if (sError.length())
				{
					_log.Log(LOG_ERROR, "(%s) Module Import failed: '%s'", Name.c_str(), sError.c_str());
					sError = "";
				}

				// SyntaxError, IndentationError & TabError have filename, lineno, offset and text attributes
				if (PyObject_HasAttrString(pValue, "filename"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "filename");
					PyBytesObject*	pBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pString);
					sError += "File: ";
					sError += pBytes->ob_sval;
					Py_XDECREF(pString);
					Py_XDECREF(pBytes);
				}
				long long	lineno = -1;
				long long 	offset = -1;
				if (PyObject_HasAttrString(pValue, "lineno"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "lineno");
					lineno = PyLong_AsLongLong(pString);
					Py_XDECREF(pString);
				}
				if (PyObject_HasAttrString(pExcept, "offset"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "offset");
					offset = PyLong_AsLongLong(pString);
					Py_XDECREF(pString);
				}

				if (sError.length())
				{
					_log.Log(LOG_ERROR, "(%s) Import detail: %s, Line: %d, offset: %d", Name.c_str(), sError.c_str(), lineno, offset);
					sError = "";
				}

				if (PyObject_HasAttrString(pExcept, "text"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "text");
					PyBytesObject*	pBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pString);
					_log.Log(LOG_ERROR, "(%s) Error Line '%s'", Name.c_str(), pBytes->ob_sval);
					Py_XDECREF(pString);
					Py_XDECREF(pBytes);
				}

				if (sError.length())
				{
					_log.Log(LOG_ERROR, "(%s) Import detail: %s", Name.c_str(), sError.c_str());
				}
			}
			else _log.Log(LOG_ERROR, "(%s) Module Import failed '%s'", Name.c_str(), pErrBytes->ob_sval);
		}

		if (pErrBytes) Py_XDECREF(pErrBytes);

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to import module '%s' failed, unable to decode exception.", Name.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	void CPlugin::LogPythonException(const std::string &sHandler)
	{
		PyTracebackObject	*pTraceback;
		PyObject			*pExcept, *pValue;
		PyTypeObject		*TypeName;
		PyBytesObject		*pErrBytes = NULL;
		const char*			pTypeText = NULL;

		PyErr_Fetch(&pExcept, &pValue, (PyObject**)&pTraceback);

		if (pExcept)
		{
			TypeName = (PyTypeObject*)pExcept;
			pTypeText = TypeName->tp_name;
		}
		if (pValue)
		{
			pErrBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pValue);
		}
		if (pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s':'%s'.", Name.c_str(), sHandler.c_str(), pTypeText, pErrBytes->ob_sval);
		}
		if (pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pTypeText);
		}
		if (!pTypeText && pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed '%s'.", Name.c_str(), sHandler.c_str(), pErrBytes->ob_sval);
		}
		if (!pTypeText && !pErrBytes)
		{
			_log.Log(LOG_ERROR, "(%s) '%s' failed, unable to determine error.", Name.c_str(), sHandler.c_str());
		}
		if (pErrBytes) Py_XDECREF(pErrBytes);

		// Log a stack trace if there is one
		while (pTraceback)
			{
			PyFrameObject *frame = pTraceback->tb_frame;
			if (frame)
			{
				int lineno = PyFrame_GetLineNumber(frame);
				PyCodeObject*	pCode = frame->f_code;
				PyBytesObject*	pFileBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)pythonLib->PyUnicode_AsASCIIString(pCode->co_name);
				_log.Log(LOG_ERROR, "(%s) ----> Line %d in %s, function %s", Name.c_str(), lineno, pFileBytes->ob_sval, pFuncBytes->ob_sval);
				Py_XDECREF(pFileBytes);
				Py_XDECREF(pFuncBytes);
			}
			pTraceback = pTraceback->tb_next;
		}

		if (!pExcept && !pValue && !pTraceback)
		{
			_log.Log(LOG_ERROR, "(%s) Call to message handler '%s' failed, unable to decode exception.", Name.c_str(), sHandler.c_str());
		}

		if (pExcept) Py_XDECREF(pExcept);
		if (pValue) Py_XDECREF(pValue);
		if (pTraceback) Py_XDECREF(pTraceback);
	}

	bool CPlugin::StartHardware()
	{
		if (m_bIsStarted) StopHardware();

		//	Add start command to message queue
		CPluginMessage	Message(PMT_Initialise, m_HwdID);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		return true;
	}

	bool CPlugin::StopHardware()
	{
		try
		{
			// Tell transport to disconnect if required
			if ((m_pTransport) && (m_pTransport->IsConnected()))
			{
				CPluginMessage	DisconnectMessage(PMT_Directive, PDT_Disconnect, m_HwdID);
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}

			//	Add stop message to message queue to notify plugin
			CPluginMessage	StopMessage(PMT_Stop, m_HwdID);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(StopMessage);
			}

			// loop on stop to be processed
			int scounter = 0;
			while (m_bIsStarted && (scounter++ < 50))
			{
				sleep_milliseconds(100);
			}

			if (m_thread)
			{
				m_stoprequested = true;
				m_thread->join();
				m_thread.reset();
			}
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}

		_log.Log(LOG_STATUS, "(%s) Stopped.", Name.c_str());

		return true;
	}

	void CPlugin::Do_Work()
	{
		m_LastHeartbeat = mytime(NULL);
		int scounter = m_iPollInterval * 2;
		while (!m_stoprequested)
		{
			if (!--scounter)
			{
				//	Add heartbeat to message queue
				CPluginMessage	Message(PMT_Heartbeat, m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
				scounter = m_iPollInterval * 2;

				m_LastHeartbeat = mytime(NULL);
			}
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "(%s) Exiting work loop...", Name.c_str());
	}

	void CPlugin::Restart()
	{
		StopHardware();
		StartHardware();
	}

	void CPlugin::HandleMessage(const CPluginMessage & Message)
	{
		std::string sHandler = "";
		PyObject* pParams = NULL;
		switch (Message.m_Type)
		{
		case PMT_Initialise:
			HandleInitialise();
			break;
		case PMT_Start:
			HandleStart();
			sHandler = "onStart";
			break;
		case PMT_Directive:
			switch (Message.m_Directive)
			{
			case PDT_Transport:
				if (m_pTransport && m_pTransport->IsConnected())
				{
					_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", Name.c_str());
					return;
				}
				if (m_pTransport)
				{
					delete m_pTransport;
					m_pTransport = NULL;
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Previous transport was not connected and has been deleted.", Name.c_str());
				}
				if (Message.m_Message == "TCP/IP")
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", Name.c_str(), Message.m_Message.c_str(), Message.m_Address.c_str(), Message.m_Port.c_str());
					m_pTransport = (CPluginTransport*) new CPluginTransportTCP(m_HwdID, Message.m_Address, Message.m_Port);
				}
				else if (Message.m_Message == "UDP/IP")
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', %s:%s.", Name.c_str(), Message.m_Message.c_str(), Message.m_Address.c_str(), Message.m_Port.c_str());
					m_pTransport = (CPluginTransport*) new CPluginTransportUDP(m_HwdID, Message.m_Address, Message.m_Port);
				}
				else if (Message.m_Message == "Serial")
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', '%s', %d.", Name.c_str(), Message.m_Message.c_str(), Message.m_Port.c_str(), Message.m_iValue);
					m_pTransport = (CPluginTransport*) new CPluginTransportSerial(m_HwdID, Message.m_Port, Message.m_iValue);
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Unknown transport type specified: '%s'.", Name.c_str(), Message.m_Message.c_str());
				}
				break;
			case PDT_Protocol:
				if (m_pProtocol)
				{
					delete m_pProtocol;
					m_pProtocol = NULL;
				}
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol set to: '%s'.", Name.c_str(), Message.m_Message.c_str());
				if (Message.m_Message == "Line") m_pProtocol = (CPluginProtocol*) new CPluginProtocolLine();
				else if (Message.m_Message == "XML") m_pProtocol = (CPluginProtocol*) new CPluginProtocolXML();
				else if (Message.m_Message == "JSON") m_pProtocol = (CPluginProtocol*) new CPluginProtocolJSON();
				else if (Message.m_Message == "HTML") m_pProtocol = (CPluginProtocol*) new CPluginProtocolHTML();
				else m_pProtocol = new CPluginProtocol();
				break;
			case PDT_PollInterval:
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Heartbeat interval set to: %d.", Name.c_str(), Message.m_iValue);
				this->m_iPollInterval = Message.m_iValue;
				break;
			case PDT_Connect:
				if (!m_pTransport)
				{
					_log.Log(LOG_ERROR, "(%s) No transport specified, directive ignored.", Name.c_str());
					return;
				}
				if (m_pTransport && m_pTransport->IsConnected())
				{
					_log.Log(LOG_ERROR, "(%s) Current transport is still connected, directive ignored.", Name.c_str());
					return;
				}
				if (m_pTransport->handleConnect())
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Connect directive received, transport connect initiated successfully.", Name.c_str());
				}
				else
				{
					_log.Log(LOG_NORM, "(%s) Connect directive received, transport connect initiation failed.", Name.c_str());
				}
				break;
			case PDT_Write:
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Sending data: '%s'.", Name.c_str(), Message.m_Message.c_str());
				m_pTransport->handleWrite((std::string&)Message.m_Message);
				break;
			case PDT_Disconnect:
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Disconnect directive received.", Name.c_str());
				if ((m_pTransport) && (m_pTransport->IsConnected()))
				{
					m_pTransport->handleDisconnect();
				}
				if (m_pProtocol)
				{
					m_pProtocol->Flush(m_HwdID);
				}
				break;
			default:
				_log.Log(LOG_ERROR, "(%s) Unknown directive type in message: %d.", Name.c_str(), Message.m_Directive);
				return;
			}
			break;
		case PMT_Connected:
			sHandler = "onConnect";
			pParams = Py_BuildValue("is", Message.m_iValue, Message.m_Message.c_str());  // 0 is success else socket failure code
			break;
		case PMT_Read:
			if (!m_pProtocol)
			{
				// If no protocol defined default to 'None'
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol not specified. Data received will be passed directly to plugin.", Name.c_str());
				m_pProtocol = new CPluginProtocol();
			}
			m_pProtocol->ProcessMessage(Message.m_HwdID, (std::string&)Message.m_Message);
			break;
		case PMT_Message:
			sHandler = "onMessage";
			pParams = Py_BuildValue("(s)", Message.m_Message.c_str());  // parenthesis needed to force tuple
			break;
		case PMT_Notification:
			sHandler = "onNotification";
			pParams = Py_BuildValue("(s)", Message.m_Message.c_str());  // parenthesis needed to force tuple
			break;
		case PMT_Heartbeat:
			sHandler = "onHeartbeat";
			break;
		case PMT_Disconnect:
			sHandler = "onDisconnect";
			break;
		case PMT_Command:
			sHandler = "onCommand";
			pParams = Py_BuildValue("isii", Message.m_Unit, Message.m_Message.c_str(), Message.m_iLevel, Message.m_iHue);
			break;
		case PMT_Stop:
			sHandler = "onStop";
			break;
		default:
			_log.Log(LOG_ERROR, "(%s) Unknown message type in message: %d.", Name.c_str(), Message.m_Type);
			return;
		}

		try
		{
			if (m_PyInterpreter) PyEval_RestoreThread((PyThreadState*)m_PyInterpreter);
			if (m_PyModule && sHandler.length())
			{
				PyObject*	pFunc = PyObject_GetAttrString((PyObject*)m_PyModule, sHandler.c_str());
				if (pFunc && PyCallable_Check(pFunc))
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Calling message handler '%s'.", Name.c_str(), sHandler.c_str());

					PyErr_Clear();
					PyObject*	pReturnValue = PyObject_CallObject(pFunc, pParams);
					if (!pReturnValue)
					{
						LogPythonException(sHandler);
					}
				}
			}

			Py_XDECREF(pParams);
		}
		catch (std::exception e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e.what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}

		if (Message.m_Type == PMT_Stop)
		{
			// Stop Python
			Py_XDECREF(m_DeviceDict);
			if (m_PyInterpreter) Py_EndInterpreter((PyThreadState*)m_PyInterpreter);
			Py_XDECREF(m_PyModule);
			m_PyModule = NULL;
			m_DeviceDict = NULL;
			m_PyInterpreter = NULL;
			m_bIsStarted = false;
		}
	}

	bool CPlugin::HandleInitialise()
	{
		m_bIsStarted = false;

		boost::lock_guard<boost::mutex> l(PythonMutex);
		m_PyInterpreter = Py_NewInterpreter();
		if (!m_PyInterpreter)
		{
			_log.Log(LOG_ERROR, "(%s) failed to create interpreter.", m_PluginKey.c_str());
			return false;
		}

		m_PyModule = PyImport_ImportModule(m_PluginKey.c_str());
		if (!m_PyModule)
		{
			_log.Log(LOG_ERROR, "(%s) failed to load, Path '%S'.", m_PluginKey.c_str(), Py_GetPath());
			LogPythonException();
			return false;
		}

		// Domoticz callbacks need state so they know which plugin to act on
		PyObject* pMod = PyState_FindModule(&DomoticzModuleDef);
		if (!pMod)
		{
			_log.Log(LOG_ERROR, "(%s) start up failed, Domoticz module not found in interpreter.", m_PluginKey.c_str());
			return false;
		}
		module_state*	pModState = ((struct module_state*)PyModule_GetState(pMod));
		pModState->pPlugin = this;

		//Start worker thread
		m_stoprequested = false;
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPlugin::Do_Work, this)));

		if (!m_thread)
		{
			_log.Log(LOG_ERROR, "(%s) failed start worker thread.", m_PluginKey.c_str());
			return false;
		}

		//	Add start command to message queue
		CPluginMessage	Message(PMT_Start, m_HwdID);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
		_log.Log(LOG_STATUS, "(%s) initialized", Name.c_str());

		return true;
	}

	bool CPlugin::HandleStart()
	{
		PyObject* pModuleDict = PyModule_GetDict((PyObject*)m_PyModule);  // returns a borrowed referece to the __dict__ object for the module
		PyObject *pParamsDict = PyDict_New();
		if (PyDict_SetItemString(pModuleDict, "Parameters", pParamsDict) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add Parameters dictionary.", m_PluginKey.c_str());
			return false;
		}
		Py_DECREF(pParamsDict);

		PyObject*	pObj = Py_BuildValue("i", m_HwdID);
		if (PyDict_SetItemString(pParamsDict, "HardwareID", pObj) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add key 'HardwareID', value '%s' to dictionary.", m_PluginKey.c_str(), m_HwdID);
			return false;
		}
		Py_DECREF(pObj);

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT Name, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6 FROM Hardware WHERE (ID==%d)", m_HwdID);
		if (result.size() > 0)
		{
			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				const char*	pChar = sd[0].c_str();
				ADD_STRING_TO_DICT(pParamsDict, "Name", sd[0]);
				ADD_STRING_TO_DICT(pParamsDict, "Address", sd[1]);
				ADD_STRING_TO_DICT(pParamsDict, "Port", sd[2]);
				ADD_STRING_TO_DICT(pParamsDict, "SerialPort", sd[3]);
				ADD_STRING_TO_DICT(pParamsDict, "Username", sd[4]);
				ADD_STRING_TO_DICT(pParamsDict, "Password", sd[5]);
				ADD_STRING_TO_DICT(pParamsDict, "Key", sd[6]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode1", sd[7]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode2", sd[8]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode3", sd[9]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode4", sd[10]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode5", sd[11]);
				ADD_STRING_TO_DICT(pParamsDict, "Mode6", sd[12]);
			}
		}

		m_DeviceDict = PyDict_New();
		if (PyDict_SetItemString(pModuleDict, "Devices", (PyObject*)m_DeviceDict) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add Device dictionary.", m_PluginKey.c_str());
			return false;
		}

		// load associated devices to make them available to python
		result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, DeviceID, Type, SubType, SwitchType, LastLevel, CustomImage FROM DeviceStatus WHERE (HardwareID==%d) AND (Used==1) ORDER BY Unit ASC", m_HwdID);
		if (result.size() > 0)
		{
			PyType_Ready(&CDeviceType);
			// Add device objects into the device dictionary with Unit as the key
			for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				CDevice* pDevice = (CDevice*)PyObject_New(CDevice, &CDeviceType);

				PyObject*	pKey = PyLong_FromLong(atoi(sd[0].c_str()));
				if (PyDict_SetItem((PyObject*)m_DeviceDict, pKey, (PyObject*)pDevice) == -1)
				{
					_log.Log(LOG_ERROR, "(%s) failed to add unit '%s' to device dictionary.", m_PluginKey.c_str(), sd[0].c_str());
					return false;
				}
				pDevice->pPlugin = this;
				pDevice->PluginKey = pythonLib->PyUnicode_FromString(m_PluginKey.c_str());
				pDevice->HwdID = m_HwdID;
				pDevice->Unit = atoi(sd[0].c_str());
				pDevice->ID = atoi(sd[1].c_str());
				pDevice->Name = pythonLib->PyUnicode_FromString(sd[2].c_str());
				pDevice->nValue = atoi(sd[3].c_str());
				pDevice->sValue = pythonLib->PyUnicode_FromString(sd[4].c_str());
				pDevice->DeviceID = pythonLib->PyUnicode_FromString(sd[5].c_str());
				pDevice->Type = atoi(sd[6].c_str());
				pDevice->SubType = atoi(sd[7].c_str());
				pDevice->SwitchType = atoi(sd[8].c_str());
				pDevice->LastLevel = atoi(sd[9].c_str());
				pDevice->Image = atoi(sd[10].c_str());

				Py_DECREF(pDevice);
			}
		}

		m_bIsStarted = true;
		return true;
	}
	
	bool CPlugin::WriteToHardware(const char *pdata, const unsigned char length)
	{
		return true;
	}

	void CPlugin::SendCommand(const int Unit, const std::string &command, const int level, const int hue)
	{
		//	Add command to message queue
		CPluginMessage	Message(PMT_Command, m_HwdID, Unit, command, level, hue);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	std::map<int, CDomoticzHardwareBase*>	CPluginSystem::m_pPlugins;
	std::map<std::string, std::string>		CPluginSystem::m_PluginXml;

	CPluginSystem::CPluginSystem() : m_stoprequested(false)
	{
		m_bEnabled = false;
		m_bAllPluginsStarted = false;
		m_iPollInterval = 10;
	}

	CPluginSystem::~CPluginSystem(void)
	{
	}

	bool CPluginSystem::StartPluginSystem()
	{
		// Flush the message queue (should already be empty)
		boost::lock_guard<boost::mutex> l(PluginMutex);
		while (!PluginMessageQueue.empty())
		{
			PluginMessageQueue.pop();
		}

		m_pPlugins.clear();

		if (!Py_LoadLibrary())
		{
			_log.Log(LOG_STATUS, "PluginSystem: Failed dynamic library load, install the latest libpython3.x library that is available for your platform.");
			return false;
		}

		// Pull UI elements from plugins and create manifest
		BuildManifest();

		m_thread = new boost::thread(boost::bind(&CPluginSystem::Do_Work, this));

		std::string sVersion(Py_GetVersion());

		try
		{
			// Make sure Python is not running
			if (Py_IsInitialized()) {
				Py_Finalize();
			}

			sVersion = sVersion.substr(0, sVersion.find_first_of(' '));
			if (sVersion < MINIMUM_PYTHON_VERSION)
			{
				_log.Log(LOG_STATUS, "PluginSystem: Invalid Python version '%s' found, '%s' or above required.", sVersion.c_str(), MINIMUM_PYTHON_VERSION);
				return false;
			}

			// Set program name, this prevents it being set to 'python'
			Py_SetProgramName(Py_GetProgramFullPath());

			// Prepend 'plugins' directory to path so that our plugin files are found
#ifdef WIN32
			std::wstring	sPath = L"plugins\\;";
#else
			std::wstring	sPath = L"./plugins/:";
#endif
			sPath += Py_GetPath();
			Py_SetPath((wchar_t*)sPath.c_str());

			PyImport_AppendInittab("Domoticz", PyInit_Domoticz);

			Py_Initialize();

			m_bEnabled = true;
			_log.Log(LOG_STATUS, "PluginSystem: Started, Python version '%s'.", sVersion.c_str());
		}
		catch (...) {
			_log.Log(LOG_ERROR, "PluginSystem: Failed to start, Python version '%s', Program '%S', Path '%S'.", sVersion.c_str(), Py_GetProgramFullPath(), Py_GetPath());
			return false;
		}

		return true;
	}

	void CPluginSystem::BuildManifest()
	{
		//
		//	Scan plugins folder and load XML plugin manifests
		//
		m_PluginXml.clear();
		std::stringstream plugin_DirT;
#ifdef WIN32
		plugin_DirT << szUserDataFolder << "plugins\\";
		std::string plugin_Dir = plugin_DirT.str();
		if (!mkdir(plugin_Dir.c_str()))
		{
			_log.Log(LOG_NORM, "%s: Created directory %s", __func__, plugin_Dir.c_str());
		}
#else
		plugin_DirT << szUserDataFolder << "plugins/";
		std::string plugin_Dir = plugin_DirT.str();
		if (!mkdir(plugin_Dir.c_str(), 0755))
		{
			_log.Log(LOG_NORM, "BuildManifest: Created directory %s", plugin_Dir.c_str());
		}
#endif
		DIR *lDir;
		struct dirent *ent;
		if ((lDir = opendir(plugin_Dir.c_str())) != NULL)
		{
			std::stringstream	FileName;
			while ((ent = readdir(lDir)) != NULL)
			{
				std::string filename = ent->d_name;
				if (dirent_is_file(plugin_Dir, ent))
				{
					if ((filename.length() > 3) && (filename.compare(filename.length() - 3, 3, ".py") == 0))
					{
						try
						{
							std::string sXML;
							std::stringstream	FileName;
							FileName << plugin_DirT.str() << filename;
							std::string line;
							std::ifstream readFile(FileName.str().c_str());
							bool pluginFound = false;
							while (getline(readFile, line)) {
								if (!pluginFound && (line.find("<plugin") != std::string::npos))
									pluginFound = true;
								if (pluginFound)
								{
									sXML += line + "\n";
								}
								if (line.find("</plugin>") != std::string::npos)
									break;
							}
							readFile.close();
							m_PluginXml.insert(std::pair<std::string, std::string>(FileName.str(), sXML));
						}
						catch (...)
						{
							_log.Log(LOG_ERROR, "%s: Exception loading plugin file: '%s'", __func__, filename.c_str());
						}
					}
				}
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "%s: Error accessing plugins directory %s", __func__, plugin_Dir.c_str());
		}
	}

	CDomoticzHardwareBase* CPluginSystem::RegisterPlugin(const int HwdID, const std::string & Name, const std::string & PluginKey)
	{
		CPlugin*	pPlugin = NULL;
		if (m_bEnabled)
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			pPlugin = new CPlugin(HwdID, Name, PluginKey);
			m_pPlugins.insert(std::pair<int, CPlugin*>(HwdID, pPlugin));
		}
		else
		{
			_log.Log(LOG_STATUS, "PluginSystem: '%s' Registration ignored, Plugins are not enabled.", Name.c_str());
		}
		return (CDomoticzHardwareBase*)pPlugin;
	}

	void CPluginSystem::DeregisterPlugin(const int HwdID)
	{
		if (m_pPlugins.count(HwdID))
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			m_pPlugins.erase(HwdID);
		}
	}

	void CPluginSystem::Do_Work()
	{
		while (!m_bAllPluginsStarted)
		{
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "PluginSystem: Entering work loop.");

		ios.reset();
		boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
		while (!m_stoprequested)
		{
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations if there are any transports that need it
			{
				bool bIos_required = false;
				for (std::map<int, CDomoticzHardwareBase*>::iterator itt = m_pPlugins.begin(); itt != m_pPlugins.end(); itt++)
				{
					CPlugin*	pPlugin = (CPlugin*)itt->second;
					if ((pPlugin->m_pTransport) && (pPlugin->m_pTransport->IsConnected()) && (pPlugin->m_pTransport->ThreadPoolRequired()))
					{
						bIos_required = true;
						break;
					}
				}

				if (bIos_required)
				{
					ios.reset();
					_log.Log(LOG_NORM, "PluginSystem: Restarting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}

			CPluginMessage Message;
			while (!PluginMessageQueue.empty())
			{
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					CPluginMessage FrontMessage = PluginMessageQueue.front();
					Message = FrontMessage;
				}
				if (!m_pPlugins.count(Message.m_HwdID))
				{
					_log.Log(LOG_ERROR, "PluginSystem: Unknown hardware in message: %d.", Message.m_HwdID);
				}
				else
				{
					CPlugin*	pPlugin = (CPlugin*)m_pPlugins[Message.m_HwdID];
					pPlugin->HandleMessage(Message);
				}

				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.pop();
			}
			sleep_milliseconds(50);
		}

		_log.Log(LOG_STATUS, "PluginSystem: Exiting work loop.");
	}

	bool CPluginSystem::StopPluginSystem()
	{
		m_bAllPluginsStarted = false;

		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread = NULL;
		}

		if (Py_LoadLibrary())
		{
			if (Py_IsInitialized()) {
				Py_Finalize();
			}
		}
		// Hardware should already be stopped to just flush the queue (should already be empty)
		boost::lock_guard<boost::mutex> l(PluginMutex);
		while (!PluginMessageQueue.empty())
		{
			PluginMessageQueue.pop();
		}

		m_pPlugins.clear();

		_log.Log(LOG_STATUS, "PluginSystem: Stopped.");
		return true;
	}

	void CPluginSystem::SendNotification(const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority, const std::string &Sound)
	{
		//	Add command to message queue for every plugin
		std::stringstream ssMessage;
		ssMessage << "|Subject=" << Subject << "|Text=" << Text << ExtraData << "Priority=" << Priority << "|Sound=" << Sound << "|";
		boost::lock_guard<boost::mutex> l(PluginMutex);
		for (std::map<int, CDomoticzHardwareBase*>::iterator itt = m_pPlugins.begin(); itt != m_pPlugins.end(); itt++)
		{
			CPluginMessage	Message(PMT_Notification, itt->second->m_HwdID, ssMessage.str());
			PluginMessageQueue.push(Message);
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::PluginList(Json::Value &root)
		{
			int		iPluginCnt = root.size();
			Plugins::CPluginSystem Plugins;
			std::map<std::string, std::string>*	PluginXml = Plugins.GetManifest();
			for (std::map<std::string, std::string>::iterator it_type = PluginXml->begin(); it_type != PluginXml->end(); it_type++)
			{
				TiXmlDocument	XmlDoc;
				XmlDoc.Parse(it_type->second.c_str());
				if (XmlDoc.Error())
				{
					_log.Log(LOG_ERROR, "%s: Error '%s' at line %d column %d in XML '%s'.", __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), it_type->second.c_str());
				}
				else
				{
					TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
					for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
					{
						TiXmlElement* pXmlEle = pXmlNode->ToElement();
						if (pXmlEle)
						{
							root[iPluginCnt]["idx"] = HTYPE_PythonPlugin;
							ATTRIBUTE_VALUE(pXmlEle, "key", root[iPluginCnt]["key"]);
							ATTRIBUTE_VALUE(pXmlEle, "name", root[iPluginCnt]["name"]);
							ATTRIBUTE_VALUE(pXmlEle, "author", root[iPluginCnt]["author"]);
							ATTRIBUTE_VALUE(pXmlEle, "wikilink", root[iPluginCnt]["wikiURL"]);
							ATTRIBUTE_VALUE(pXmlEle, "externallink", root[iPluginCnt]["externalURL"]);

							TiXmlNode* pXmlParamsNode = pXmlEle->FirstChild("params");
							int	iParams = 0;
							if (pXmlParamsNode) pXmlParamsNode = pXmlParamsNode->FirstChild("param");
							for (pXmlParamsNode; pXmlParamsNode; pXmlParamsNode = pXmlParamsNode->NextSiblingElement())
							{
								// <params>
								//		<param field = "Address" label = "IP/Address" width = "100px" required = "true" default = "127.0.0.1" / >
								TiXmlElement* pXmlEle = pXmlParamsNode->ToElement();
								if (pXmlEle)
								{
									ATTRIBUTE_VALUE(pXmlEle, "field", root[iPluginCnt]["parameters"][iParams]["field"]);
									ATTRIBUTE_VALUE(pXmlEle, "label", root[iPluginCnt]["parameters"][iParams]["label"]);
									ATTRIBUTE_VALUE(pXmlEle, "width", root[iPluginCnt]["parameters"][iParams]["width"]);
									ATTRIBUTE_VALUE(pXmlEle, "required", root[iPluginCnt]["parameters"][iParams]["required"]);
									ATTRIBUTE_VALUE(pXmlEle, "default", root[iPluginCnt]["parameters"][iParams]["default"]);

									TiXmlNode* pXmlOptionsNode = pXmlEle->FirstChild("options");
									int	iOptions = 0;
									if (pXmlOptionsNode) pXmlOptionsNode = pXmlOptionsNode->FirstChild("option");
									for (pXmlOptionsNode; pXmlOptionsNode; pXmlOptionsNode = pXmlOptionsNode->NextSiblingElement())
									{
										// <options>
										//		<option label="Hibernate" value="1" default="true" />
										TiXmlElement* pXmlEle = pXmlOptionsNode->ToElement();
										if (pXmlEle)
										{
											std::string sDefault;
											ATTRIBUTE_VALUE(pXmlEle, "label", root[iPluginCnt]["parameters"][iParams]["options"][iOptions]["label"]);
											ATTRIBUTE_VALUE(pXmlEle, "value", root[iPluginCnt]["parameters"][iParams]["options"][iOptions]["value"]);
											ATTRIBUTE_VALUE(pXmlEle, "default", sDefault);
											if (sDefault == "true")
											{
												root[iPluginCnt]["parameters"][iParams]["options"][iOptions]["default"] = sDefault;
											}
											iOptions++;
										}
									}
									iParams++;
								}
							}
							iPluginCnt++;
						}
					}
				}
			}  
		}
	}
}
#endif