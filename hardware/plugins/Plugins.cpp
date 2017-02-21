#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "Plugins.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"

#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../hardware/hardwaretypes.h"
#include "../tinyxpath/tinyxml.h"
#include "../main/localtime_r.h"

#include "DelayedLink.h"

#define ADD_STRING_TO_DICT(pDict, key, value) \
		{	\
			PyObject*	pObj = Py_BuildValue("s", value.c_str());	\
			if (PyDict_SetItemString(pDict, key, pObj) == -1)	\
				_log.Log(LOG_ERROR, "(%s) failed to add key '%s', value '%s' to dictionary.", m_PluginKey.c_str(), key, value.c_str());	\
			Py_DECREF(pObj); \
		}

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessage>	PluginMessageQueue;
	extern boost::asio::io_service ios;

	boost::mutex PythonMutex;	// only used during startup when multiple threads could use Python

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
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);
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
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
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
					LogPythonException(pModState->pPlugin, std::string(__func__));
				}
				else
				{
					std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
					_log.Log((_eLogLevel)LOG_NORM, message.c_str());
				}
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
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_NORM, message.c_str());
			}
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
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				std::string	message = "(" + pModState->pPlugin->Name + ") " + msg;
				_log.Log((_eLogLevel)LOG_ERROR, message.c_str());
			}
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
				LogPythonException(pModState->pPlugin, std::string(__func__));
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

	static PyObject*	PyDomoticz_Transport(PyObject *self, PyObject *args, PyObject *keywds)
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
		else if (pModState->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, Transport set request from '%s' ignored. Plugin is stopping.", __func__, pModState->pPlugin->Name.c_str());
		}
		else
		{
			char*	szTransport;
			char*	szAddress;
			char*	szPort = NULL;
			int		iBaud = 115200;
			static char *kwlist[] = { "Transport", "Address", "Port", "Baud", NULL };
			if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|si", kwlist, &szTransport, &szAddress, &szPort, &iBaud))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters. Expected: Transport, Address, Port or Transport, Address, Baud.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
				Py_INCREF(Py_None);
				return Py_None;
			}

			//	Add start command to message queue
			std::string	sTransport = szTransport;
			CPluginMessage	Message(PMT_Directive, PDT_Transport, pModState->pPlugin->m_HwdID, sTransport);
			{
				Message.m_Address = szAddress;
				if (szPort) Message.m_Port = szPort;
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
		else if (pModState->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, protocol set request from '%s' ignored. Plugin is stopping.", __func__, pModState->pPlugin->Name.c_str());
		}
		else
		{
			char*	szProtocol;
			if (!PyArg_ParseTuple(args, "s", &szProtocol))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, string expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
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
				LogPythonException(pModState->pPlugin, std::string(__func__));
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
			if (pModState->pPlugin->m_stoprequested)
			{
				_log.Log(LOG_NORM, "%s, connection request from '%s' ignored. Plugin is stopping.", __func__, pModState->pPlugin->Name.c_str());
			}
			else if ((pModState->pPlugin->m_pTransport) && (pModState->pPlugin->m_pTransport->IsConnected()))
			{
				_log.Log(LOG_ERROR, "%s, connection request from '%s' ignored. Transport is already connected.", __func__, pModState->pPlugin->Name.c_str());
			}
			else
			{
				CPluginMessage	Message(PMT_Directive, PDT_Connect, pModState->pPlugin->m_HwdID);
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Send(PyObject *self, PyObject *args, PyObject *keywds)
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
		else if (pModState->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, send request from '%s' ignored. Plugin is stopping.", __func__, pModState->pPlugin->Name.c_str());
		}
		else
		{
			char*		szMessage = NULL;
			char*		szVerb = NULL;
			char*		szURL = NULL;
			PyObject*	pHeaders = NULL;
			int			iDelay = 0;
			static char *kwlist[] = { "Message", "Verb", "URL", "Headers", "Delay", NULL };
			if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ssOi", kwlist, &szMessage, &szVerb, &szURL, &pHeaders, &iDelay))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, Message or Message,Verb,URL,Headers,Delay expected.", pModState->pPlugin->Name.c_str());
				LogPythonException(pModState->pPlugin, std::string(__func__));
			}
			else
			{
				//	Add start command to message queue
				std::string	sMessage = szMessage;
				CPluginMessage	Message(PMT_Directive, PDT_Write, pModState->pPlugin->m_HwdID, sMessage);
				{
					if (szURL) Message.m_Address = szURL;
					if (szVerb) Message.m_Operation = szVerb;
					if (pHeaders)
					{
						Message.m_Object = pHeaders;
						Py_INCREF(pHeaders);
					}
					if (iDelay) Message.m_When += iDelay;
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
		else if (pModState->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, disconnection request from '%s' ignored. Plugin is stopping.", __func__, pModState->pPlugin->Name.c_str());
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
		{ "Transport", (PyCFunction)PyDomoticz_Transport, METH_VARARGS | METH_KEYWORDS, "Set the communication transport: TCP/IP, Serial." },
		{ "Protocol", PyDomoticz_Protocol, METH_VARARGS, "Set the protocol the messages will use: None, line, JSON, XML, HTTP." },
		{ "Heartbeat", PyDomoticz_Heartbeat, METH_VARARGS, "Set the heartbeat interval, default 10 seconds." },
		{ "Connect", PyDomoticz_Connect, METH_NOARGS, "Connect to remote device using transport details." },
		{ "Send", (PyCFunction)PyDomoticz_Send, METH_VARARGS | METH_KEYWORDS, "Send the specified message to the remote device." },
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
		PyObject*	LastUpdate;
		int			nValue;
		int			SignalLevel;
		int			BatteryLevel;
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
				self->PluginKey = PyUnicode_FromString("");
				if (self->PluginKey == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->HwdID = -1;
				self->DeviceID = PyUnicode_FromString("");
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
				self->Name = PyUnicode_FromString("");
				if (self->Name == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->nValue = 0;
				self->sValue = PyUnicode_FromString("");
				if (self->sValue == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Options = PyUnicode_FromString("");
				if (self->Options == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Image = 0;
				self->SignalLevel = 100;
				self->BatteryLevel = 255;
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
		char*		TypeName = NULL;
		int			Type = -1;
		int			SubType = -1;
		int			SwitchType = -1;
		int			Image = -1;
		char*		Options = NULL;
		static char *kwlist[] = { "Name", "Unit", "TypeName", "Type", "Subtype", "Switchtype", "Image", "Options", NULL };

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

			if (PyArg_ParseTupleAndKeywords(args, kwds, "si|siiiis", kwlist, &Name, &Unit, &TypeName, &Type, &SubType, &SwitchType, &Image, &Options))
			{
				self->pPlugin = pModState->pPlugin;
				self->PluginKey = PyUnicode_FromString(pModState->pPlugin->m_PluginKey.c_str());
				self->HwdID = pModState->pPlugin->m_HwdID;
				if (Name) {
					Py_DECREF(self->Name);
					self->Name = PyUnicode_FromString(Name);
				}
				if ((Unit > 0) && (Unit < 256))
				{
					self->Unit = Unit;
				}
				else
				{
					_log.Log(LOG_ERROR, "CPlugin:%s, illegal Unit number (%d), valid values range from 1 to 255.", __func__, Unit);
					return 0;
				}
				if (TypeName) {
					std::string	sTypeName = TypeName;

					self->Type = pTypeGeneral;

					if (sTypeName == "Pressure")					self->SubType = sTypePressure;
					else if (sTypeName == "Percentage")				self->SubType = sTypePercentage;
					else if (sTypeName == "Gas")
					{
						self->Type = pTypeP1Gas;
						self->SubType = sTypeP1Gas;
					}
					else if (sTypeName == "Voltage")				self->SubType = sTypeVoltage;
					else if (sTypeName == "Text")					self->SubType = sTypeTextStatus;
					else if (sTypeName == "Switch")
					{
						self->Type = pTypeGeneralSwitch;
						self->SubType = sSwitchGeneralSwitch;
					}
					else if (sTypeName == "Alert")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("No Alert!");
						self->SubType = sTypeAlert;
					}
					else if (sTypeName == "Current/Ampere")	
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;0.0;0.0");
						self->Type = pTypeCURRENT;
						self->SubType = sTypeELEC1;
					}
					else if (sTypeName == "Sound Level")			self->SubType = sTypeSoundLevel;
					else if (sTypeName == "Barometer")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("1021.34;0");
						self->SubType = sTypeBaro;
					}
					else if (sTypeName == "Visibility")				self->SubType = sTypeVisibility;
					else if (sTypeName == "Distance")				self->SubType = sTypeDistance;
					else if (sTypeName == "Counter Incremental")	self->SubType = sTypeCounterIncremental;
					else if (sTypeName == "Soil Moisture")			self->SubType = sTypeSoilMoisture;
					else if (sTypeName == "Leaf Wetness")			self->SubType = sTypeLeafWetness;
					else if (sTypeName == "kWh")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0; 0.0");
						self->SubType = sTypeKwh;
					}
					else if (sTypeName == "Current (Single)")		self->SubType = sTypeCurrent;
					else if (sTypeName == "Solar Radiation")		self->SubType = sTypeSolarRadiation;
					else if (sTypeName == "Temperature")
					{
						self->Type = pTypeTEMP;
						self->SubType = sTypeTEMP5;
					}
					else if (sTypeName == "Humidity")
					{
						self->Type = pTypeHUM;
						self->SubType = sTypeHUM1;
					}
					else if (sTypeName == "Temp+Hum")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;50;1");
						self->Type = pTypeTEMP_HUM;
						self->SubType = sTypeTH1;
					}
					else if (sTypeName == "Temp+Hum+Baro")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;50;1;1010;1");
						self->Type = pTypeTEMP_HUM_BARO;
						self->SubType = sTypeTHB1;
					}
					else if (sTypeName == "Wind")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;N;0;0;0;0");
						self->Type = pTypeWIND;
						self->SubType = sTypeWIND1;
					}
					else if (sTypeName == "Rain")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;0");
						self->Type = pTypeRAIN;
						self->SubType = sTypeRAIN3;
					}
					else if (sTypeName == "UV")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;0");
						self->Type = pTypeUV;
						self->SubType = sTypeUV1;
					}
					else if (sTypeName == "Air Quality")
					{
						self->Type = pTypeAirQuality;
						self->SubType = sTypeVoltcraft;
					}
					else if (sTypeName == "Usage")
					{
						self->Type = pTypeUsage;
						self->SubType = sTypeElectric;
					}
					else if (sTypeName == "Illumination")
					{
						self->Type = pTypeLux;
						self->SubType = sTypeLux;
					}
					else if (sTypeName == "Waterflow")				self->SubType = sTypeWaterflow;
					else if (sTypeName == "Wind+Temp+Chill")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;N;0;0;0;0");
						self->Type = pTypeWIND;
						self->SubType = sTypeWIND4;
					}
					else if (sTypeName == "Selector Switch")
					{
						Py_DECREF(self->Options);
						self->Options = PyUnicode_FromString("LevelActions:fHx8;LevelNames:T2ZmfExldmVsMXxMZXZlbDJ8TGV2ZWwz;LevelOffHidden:ZmFsc2U=;SelectorStyle:MA==");
						self->Type = pTypeGeneralSwitch;
						self->SubType = sSwitchTypeSelector;
						self->SwitchType = 18;
					}
					else if (sTypeName == "Custom")					self->SubType = sTypeCustom;
				}
				if ((Type != -1) && Type) self->Type = Type;
				if ((SubType != -1) && SubType) self->SubType = SubType;
				if (SwitchType != -1) self->SwitchType = SwitchType;
				if (Image != -1) self->Image = Image;
				if (Options) {
					Py_DECREF(self->Options);
					self->Options = PyUnicode_FromString(Options);
				}
			}
			else
			{
				CPlugin* pPlugin = NULL;
				if (pModState) pPlugin = pModState->pPlugin;
				_log.Log(LOG_ERROR, "Expected: myVar = Domoticz.Device(Name=\"myDevice\", Unit=0, TypeName=\"\", Type=0, Subtype=0, Switchtype=0, Image=0, Options=\"\")");
				LogPythonException(pPlugin, __func__);
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
		{ "SignalLevel", T_INT, offsetof(CDevice, SignalLevel), READONLY, "Numeric signal level" },
		{ "BatteryLevel", T_INT, offsetof(CDevice, BatteryLevel), READONLY, "Numeric battery level" },
		{ "Image", T_INT, offsetof(CDevice, Image), READONLY, "Numeric image number" },
		{ "LastLevel", T_INT, offsetof(CDevice, LastLevel), READONLY, "Previous device level" },
		{ "LastUpdate", T_OBJECT, offsetof(CDevice, LastUpdate), READONLY, "Last update timestamp" },
		{ NULL }  /* Sentinel */
	};

	static PyObject* CDevice_refresh(CDevice* self)
	{
		if ((self->pPlugin) && (self->HwdID != -1) && (self->Unit != -1))
		{
			// load associated devices to make them available to python
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, DeviceID, Type, SubType, SwitchType, LastLevel, CustomImage, SignalLevel, BatteryLevel, LastUpdate FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) ORDER BY Unit ASC", self->HwdID, self->Unit);
			if (result.size() > 0)
			{
				for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					self->Unit = atoi(sd[0].c_str());
					self->ID = atoi(sd[1].c_str());
					Py_XDECREF(self->Name);
					self->Name = PyUnicode_FromString(sd[2].c_str());
					self->nValue = atoi(sd[3].c_str());
					Py_XDECREF(self->sValue);
					self->sValue = PyUnicode_FromString(sd[4].c_str());
					Py_XDECREF(self->DeviceID);
					self->DeviceID = PyUnicode_FromString(sd[5].c_str());
					self->Type = atoi(sd[6].c_str());
					self->SubType = atoi(sd[7].c_str());
					self->SwitchType = atoi(sd[8].c_str());
					self->LastLevel = atoi(sd[9].c_str());
					self->Image = atoi(sd[10].c_str());
					self->SignalLevel = atoi(sd[11].c_str());
					self->BatteryLevel = atoi(sd[12].c_str());
					Py_XDECREF(self->LastUpdate);
					self->LastUpdate = PyUnicode_FromString(sd[13].c_str());
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device refresh failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_insert(CDevice* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			PyObject*	pNameBytes = PyUnicode_AsASCIIString(self->Name);
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
					PyObject*	pOptionBytes = PyUnicode_AsASCIIString(self->Options);
					PyObject*	pSValueBytes = PyUnicode_AsASCIIString(self->sValue);
					std::string	sLongName = self->pPlugin->Name + " - " + PyBytes_AsString(pNameBytes);
					m_sql.safe_query(
						"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
						"VALUES (%d, '%q', %d, %d, %d, %d, 1, 12, 255, '%q', 0, '%q', %d, '%q')",
						self->HwdID, szID, self->Unit, self->Type, self->SubType, self->SwitchType, sLongName.c_str(), std::string(PyBytes_AsString(pSValueBytes)).c_str(), self->Image, std::string(PyBytes_AsString(pOptionBytes)).c_str());
					Py_DECREF(pOptionBytes);
					Py_DECREF(pSValueBytes);

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

						// Refresh device data to ensure it is usable straight away
						PyObject* pRetVal = CDevice_refresh(self);
						Py_DECREF(pRetVal);
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

	static PyObject* CDevice_update(CDevice *self, PyObject *args, PyObject *kwds)
	{
		if (self->pPlugin)
		{
			int			nValue = self->nValue;
			char*		sValue = NULL;
			int			iSignalLevel = self->SignalLevel;
			int			iBatteryLevel = self->BatteryLevel;
			int			iImage = self->Image;
			PyObject*	pNameBytes = PyUnicode_AsASCIIString(self->Name);
			static char *kwlist[] = { "nValue", "sValue", "Image", "SignalLevel", "BatteryLevel", NULL };

			if (!PyArg_ParseTupleAndKeywords(args, kwds, "is|iii", kwlist, &nValue, &sValue, &iImage, &iSignalLevel, &iBatteryLevel))
			{
				_log.Log(LOG_ERROR, "(%s) %s: Failed to parse parameters: 'nValue', 'sValue', 'SignalLevel' or 'BatteryLevel' expected.", __func__, PyBytes_AsString(pNameBytes));
				LogPythonException(self->pPlugin, __func__);
				Py_DECREF(pNameBytes);
				return NULL;
			}

			if (self->pPlugin->m_bDebug)
			{
				wchar_t*	pWideValue = PyUnicode_AsWideCharString(self->sValue, NULL);
				_log.Log(LOG_NORM, "(%s) Updating device from %d:'%S' to have values %d:'%s'.",
					std::string(PyBytes_AsString(pNameBytes)).c_str(), self->nValue, pWideValue, nValue, sValue);
				PyMem_Free(pWideValue);
			}
			PyObject*	pDeviceBytes = PyUnicode_AsASCIIString(self->DeviceID);
			std::string	sName = PyBytes_AsString(pNameBytes);
			m_sql.UpdateValue(self->HwdID, std::string(PyBytes_AsString(pDeviceBytes)).c_str(), (const unsigned char)self->Unit, (const unsigned char)self->Type, (const unsigned char)self->SubType, iSignalLevel, iBatteryLevel, nValue, std::string(sValue).c_str(), sName, true);
			Py_DECREF(pDeviceBytes);
			Py_DECREF(pNameBytes);

			// Image change
			if (iImage != self->Image)
			{
				time_t now = time(0);
				struct tm ltime;
				localtime_r(&now, &ltime);
				m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=%d, LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (HardwareID==%d) and (Unit==%d)",
					iImage, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec, self->HwdID, self->Unit);
			}

			CDevice_refresh(self);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device update failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_delete(CDevice* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			PyObject*	pNameBytes = PyUnicode_AsASCIIString(self->Name);
			if (self->ID != -1)
			{
				if (self->pPlugin->m_bDebug)
				{
					_log.Log(LOG_NORM, "(%s) Deleting device '%s'.", self->pPlugin->Name.c_str(), std::string(PyBytes_AsString(pNameBytes)).c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
				if (result.size() != 0)
				{
					result = m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);

					PyObject*	pKey = PyLong_FromLong(self->Unit);
					if (PyDict_DelItem((PyObject*)self->pPlugin->m_DeviceDict, pKey) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to delete unit '%d' from device dictionary.", self->pPlugin->Name.c_str(), self->Unit);
						Py_INCREF(Py_None);
						return Py_None;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Device deletion failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Device deletion failed, '%s' does not represent a device in Domoticz.", self->pPlugin->Name.c_str(), std::string(PyBytes_AsString(pNameBytes)).c_str());
			}
			Py_DECREF(pNameBytes);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device deletion failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject* CDevice_str(CDevice* self)
	{
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Name: '%U', nValue: %d, sValue: '%U'", self->ID, self->Name, self->nValue, self->sValue);
		return pRetVal;
	}

	static PyMethodDef CDevice_methods[] = {
		{ "Refresh", (PyCFunction)CDevice_refresh, METH_NOARGS, "Refresh device details"},
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


	CPlugin::CPlugin(const int HwdID, const std::string &sName, const std::string &sPluginKey) : 
		m_stoprequested(false),
		m_pProtocol(NULL),
		m_pTransport(NULL),
		m_PluginKey(sPluginKey),
		m_iPollInterval(10),
		m_bDebug(false),
		m_PyInterpreter(NULL),
		m_PyModule(NULL),
		m_DeviceDict(NULL)
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
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);	// Won't normally return text for Import related errors
			if (!pErrBytes)
			{
				// ImportError has name and path attributes
				if (PyObject_HasAttrString(pValue, "path"))
				{
					PyObject*		pString = PyObject_GetAttrString(pValue, "path");
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
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
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
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
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
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
					PyBytesObject*	pBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pString);
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
			pErrBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pValue);
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
				PyBytesObject*	pFileBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_filename);
				PyBytesObject*	pFuncBytes = (PyBytesObject*)PyUnicode_AsASCIIString(pCode->co_name);
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
		InitializeMessage	Message(m_HwdID);
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
			m_stoprequested = true;

			// Tell transport to disconnect if required
			if ((m_pTransport) && (m_pTransport->IsConnected()))
			{
				CPluginMessage	DisconnectMessage(PMT_Directive, PDT_Disconnect, m_HwdID);
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}
			else
			{
				// otherwise just signal stop
				StopMessage	StopMessage(m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(StopMessage);
				}
			}

			// loop on stop to be processed
			int scounter = 0;
			while (m_bIsStarted && (scounter++ < 50))
			{
				sleep_milliseconds(100);
			}

			if (m_thread)
			{
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
				HeartbeatMessage	Message(m_HwdID);
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
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Transport set to: '%s', '%s', %d.", Name.c_str(), Message.m_Message.c_str(), Message.m_Address.c_str(), Message.m_iValue);
					m_pTransport = (CPluginTransport*) new CPluginTransportSerial(m_HwdID, Message.m_Address, Message.m_iValue);
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
				else if (Message.m_Message == "HTTP")
				{
					CPluginProtocolHTTP*	pProtocol = new CPluginProtocolHTTP();
					pProtocol->AuthenticationDetails(m_Username, m_Password);
					m_pProtocol = (CPluginProtocol*)pProtocol;
				}
				else m_pProtocol = new CPluginProtocol();
				break;
			case PDT_PollInterval:
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Heartbeat interval set to: %d.", Name.c_str(), Message.m_iValue);
				this->m_iPollInterval = Message.m_iValue;
				break;
			case PDT_Connect:
				if (!m_pTransport)
				{
					_log.Log(LOG_ERROR, "(%s) No transport specified, connect directive ignored.", Name.c_str());
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
				if (!m_pTransport || !m_pTransport->IsConnected())
				{
					_log.Log(LOG_ERROR, "(%s) Transport is not connected, write directive ignored.", Name.c_str());
					return;
				}
				else
				{
					if (!m_pProtocol)
					{
						if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol not specified, 'None' assumed.", Name.c_str());
						m_pProtocol = new CPluginProtocol();
					}
					std::string	sWriteData = m_pProtocol->ProcessOutbound(Message);
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Sending data: '%s'.", Name.c_str(), sWriteData.c_str());
					m_pTransport->handleWrite(sWriteData);
					if (Message.m_Object)
					{
						PyObject*	pHeaders = (PyObject*)Message.m_Object;
						Py_XDECREF(pHeaders);
					}
				}
				break;
			case PDT_Disconnect:
				if (m_pTransport && (m_pTransport->IsConnected()))
				{
					if (m_bDebug) _log.Log(LOG_NORM, "(%s) Disconnect directive received.", Name.c_str());
					m_pTransport->handleDisconnect();
					if (m_pProtocol)
					{
						m_pProtocol->Flush(m_HwdID);
					}
					// inform the plugin
					DisconnectMessage	DisconnectMessage(m_HwdID);
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(DisconnectMessage);
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
				if (m_bDebug) _log.Log(LOG_NORM, "(%s) Protocol not specified, 'None' assumed.", Name.c_str());
				m_pProtocol = new CPluginProtocol();
			}
			m_pProtocol->ProcessInbound(Message.m_HwdID, (std::string&)Message.m_Message);
			break;
		case PMT_Message:
			if (Message.m_Message.length())
			{
				sHandler = "onMessage";
				if (Message.m_Object)
				{
					PyObject*	pHeaders = (PyObject*)Message.m_Object;
					pParams = Py_BuildValue("yiO", (unsigned char*)(Message.m_Message.c_str()), Message.m_iLevel, pHeaders);
					if (!pParams)
					{
						_log.Log(LOG_ERROR, "(%s) Failed to create parameters for inbound message: (%d) %s.", Name.c_str(), Message.m_Message.length(), Message.m_Message.c_str());
						LogPythonException(sHandler);
					}
					Py_XDECREF(pHeaders);
				}
				else
				{
					Py_INCREF(Py_None);
					pParams = Py_BuildValue("yiO", Message.m_Message.c_str(), Message.m_iLevel, Py_None);
				}
			}
			break;
		case PMT_Notification:
			sHandler = "onNotification";
			pParams = Py_BuildValue("ssssiss", Message.m_Name.c_str(), Message.m_Subject.c_str(), Message.m_Text.c_str(), Message.m_Status.c_str(), Message.m_Priority, Message.m_Sound.c_str(), Message.m_ImageFile.c_str());
			break;
		case PMT_Heartbeat:
			sHandler = "onHeartbeat";
			break;
		case PMT_Disconnect:
			sHandler = "onDisconnect";
			if (m_stoprequested) // Plugin exiting, forced stop
			{
				StopMessage	StopMessage(m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(StopMessage);
				}
			}
			break;
		case PMT_Command:
			sHandler = "onCommand";
			if (Message.m_fLevel != -273.15)
			{
				pParams = Py_BuildValue("isfi", Message.m_Unit, Message.m_Message.c_str(), Message.m_fLevel, 0);
			}
			else
			{
				pParams = Py_BuildValue("isii", Message.m_Unit, Message.m_Message.c_str(), Message.m_iLevel, Message.m_iHue);
			}
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
				else if (m_bDebug) _log.Log(LOG_NORM, "(%s) Message handler '%s' not callable, ignored.", Name.c_str(), sHandler.c_str());
			}

			if (pParams) Py_XDECREF(pParams);
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
			try
			{
				// Stop Python
				if (m_DeviceDict) Py_XDECREF(m_DeviceDict);
				if (m_PyInterpreter) Py_EndInterpreter((PyThreadState*)m_PyInterpreter);
				Py_XDECREF(m_PyModule);
			}
			catch (std::exception e)
			{
				_log.Log(LOG_ERROR, "%s: Execption thrown releasing Interpreter: %s", __func__, e.what());
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "%s: Unknown execption thrown releasing Interpreter", __func__);
			}
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

		// Prepend plugin directory to path so that python will search it early when importing
#ifdef WIN32
		std::wstring	sSeparator = L";";
#else
		std::wstring	sSeparator = L":";
#endif
		std::wstringstream ssPath;
		std::string		sFind = "key=\"" + m_PluginKey + "\"";
		CPluginSystem Plugins;
		std::map<std::string, std::string>*	mPluginXml = Plugins.GetManifest();
		std::string		sPluginXML;
		for (std::map<std::string, std::string>::iterator it_type = mPluginXml->begin(); it_type != mPluginXml->end(); it_type++)
		{
			if (it_type->second.find(sFind) != std::string::npos)
			{
				m_HomeFolder = it_type->first;
				ssPath << m_HomeFolder.c_str();
				sPluginXML = it_type->second;
				break;
			}
		}
		std::wstring	sPath = ssPath.str() + sSeparator;
		sPath += Py_GetPath();
		PySys_SetPath((wchar_t*)sPath.c_str());

		m_PyModule = PyImport_ImportModule("plugin");
		if (!m_PyModule)
		{
			_log.Log(LOG_ERROR, "(%s) failed to load 'plugin.py', Python Path used was '%S'.", m_PluginKey.c_str(), sPath.c_str());
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
		StartMessage	Message(m_HwdID);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		std::string		sExtraDetail;
		TiXmlDocument	XmlDoc;
		XmlDoc.Parse(sPluginXML.c_str());
		if (XmlDoc.Error())
		{
			_log.Log(LOG_ERROR, "%s: Error '%s' at line %d column %d in XML '%s'.", __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), sPluginXML.c_str());
		}
		else
		{
			TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
			for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
			{
				TiXmlElement* pXmlEle = pXmlNode->ToElement();
				if (pXmlEle)
				{
					const char*	pAttributeValue = pXmlEle->Attribute("version");
					if (pAttributeValue)
					{
						m_Version = pAttributeValue;
						sExtraDetail += "version ";
						sExtraDetail += pAttributeValue;
					}
					pAttributeValue = pXmlEle->Attribute("author");
					if (pAttributeValue)
					{
						m_Author = pAttributeValue;
						if (sExtraDetail.length()) sExtraDetail += ", ";
						sExtraDetail += "author '";
						sExtraDetail += pAttributeValue;
						sExtraDetail += "'";
					}
				}
			}
		}
		_log.Log(LOG_STATUS, "(%s) Initialized %s", Name.c_str(), sExtraDetail.c_str());

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
				ADD_STRING_TO_DICT(pParamsDict, "HomeFolder", m_HomeFolder);
				ADD_STRING_TO_DICT(pParamsDict, "Version", m_Version);
				ADD_STRING_TO_DICT(pParamsDict, "Author", m_Author);
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

				// Remember these for use with some protocols
				m_Username = sd[4];
				m_Password = sd[5];
			}
		}

		m_DeviceDict = PyDict_New();
		if (PyDict_SetItemString(pModuleDict, "Devices", (PyObject*)m_DeviceDict) == -1)
		{
			_log.Log(LOG_ERROR, "(%s) failed to add Device dictionary.", m_PluginKey.c_str());
			return false;
		}

		// load associated devices to make them available to python
		result = m_sql.safe_query("SELECT Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (Used==1) ORDER BY Unit ASC", m_HwdID);
		if (result.size() > 0)
		{
			PyType_Ready(&CDeviceType);
			// Add device objects into the device dictionary with Unit as the key
			for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;
				CDevice* pDevice = (CDevice*)CDevice_new(&CDeviceType, (PyObject*)NULL, (PyObject*)NULL);

				PyObject*	pKey = PyLong_FromLong(atoi(sd[0].c_str()));
				if (PyDict_SetItem((PyObject*)m_DeviceDict, pKey, (PyObject*)pDevice) == -1)
				{
					_log.Log(LOG_ERROR, "(%s) failed to add unit '%s' to device dictionary.", m_PluginKey.c_str(), sd[0].c_str());
					return false;
				}
				pDevice->pPlugin = this;
				pDevice->PluginKey = PyUnicode_FromString(m_PluginKey.c_str());
				pDevice->HwdID = m_HwdID;
				pDevice->Unit = atoi(sd[0].c_str());
				CDevice_refresh(pDevice);
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
		CommandMessage	Message(m_HwdID, Unit, command, level, hue);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}
	void CPlugin::SendCommand(const int Unit, const std::string & command, const float level)
	{
		//	Add command to message queue
		CommandMessage	Message(m_HwdID, Unit, command, level);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}
}
#endif
