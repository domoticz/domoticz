#include "stdafx.h"


#include "PluginManager.h"
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
#include "Include/structmember.h"

#define MINIMUM_PYTHON_VERSION "3.5.1"

#ifdef WIN32
#include "../../../domoticz/main/dirent_windows.h"
#else
#include <dirent.h>
#endif

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
				_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to add key '%s', value '%s' to dictionary.", key, value);	\
			Py_DECREF(pObj); \
		}

extern std::string szUserDataFolder;

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

namespace Plugins {

	boost::mutex PythonMutex;	// only used during startup when multiple threads could use Python
	boost::mutex PluginMutex;	// controls accessto the message queue
	std::queue<CPluginMessage>	PluginMessageQueue;

	struct module_state {
		PyObject *error;
	};

	static PyObject*	PyDomoticz_Log(PyObject *self, PyObject *args)
	{
		char* msg;
		int type;
		if (!PyArg_ParseTuple(args, "is", &type, &msg))
			return NULL;
		_log.Log((_eLogLevel)type, msg);
		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Debug(PyObject *self, PyObject *args)
	{
		int		HwdID;
		int		type;
		if (!PyArg_ParseTuple(args, "ii", &HwdID, &type))
			return NULL;
		CPluginMessage	Message(PMT_Directive, PMT_Debug, HwdID, std::string(type ? "true" : "false"));
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Transport(PyObject *self, PyObject *args)
	{
		int		HwdID;
		char*	szTransport;
		if (!PyArg_ParseTuple(args, "is", &HwdID, &szTransport))
			return NULL;
		//	Add start command to message queue
		std::string	sTransport = szTransport;
		CPluginMessage	Message(PMT_Directive, PMT_Transport, HwdID, sTransport);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyObject*	PyDomoticz_Protocol(PyObject *self, PyObject *args)
	{
		int		HwdID;
		char*	szProtocol;
		if (!PyArg_ParseTuple(args, "is", &HwdID, &szProtocol))
			return NULL;
		//	Add start command to message queue
		std::string	sProtocol = szProtocol;
		CPluginMessage	Message(PMT_Directive, PMT_Protocol, HwdID, sProtocol);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	static PyMethodDef DomoticzMethods[] = {
		{ "Log", PyDomoticz_Log, METH_VARARGS, "Write message to Domoticz log." },
		{ "Debug", PyDomoticz_Debug, METH_VARARGS, "Set logging level. 1 set verbose logging, all other values use default level" },
		{ "Transport", PyDomoticz_Transport, METH_VARARGS, "Set the communication transport: TCP/IP, Serial." },
		{ "Protocol", PyDomoticz_Protocol, METH_VARARGS, "Set the protocol the messages will use: None, HTTP." },
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

	static struct PyModuleDef moduledef = {
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

	PyMODINIT_FUNC PyInit_Domoticz(void)
	{
		return PyModule_Create(&moduledef);
	}

	typedef struct {
		PyObject_HEAD
		int			ID;
		PyObject*	Name;
		int			nValue;
		PyObject*	sValue;
	} CDevice;

	static void CDevice_dealloc(CDevice* self)
	{
		Py_XDECREF(self->Name);
		Py_XDECREF(self->sValue);
		Py_TYPE(self)->tp_free((PyObject*)self);
	}

	static PyObject* CDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		// Domoticz.Device objects should not be created in Python
		return NULL;
	}

	static int CDevice_init(CDevice *self, PyObject *args, PyObject *kwds)
	{
		// Domoticz.Device objects should not be initialised in Python
		return -1;
	}

	static PyMemberDef CDevice_members[] = {
		{ "ID",		T_INT,		offsetof(CDevice, ID),		READONLY, "Domoticz internal ID" },
		{ "Name",	T_OBJECT,	offsetof(CDevice, Name),	READONLY, "Name" },
		{ "nValue", T_INT,		offsetof(CDevice, nValue), READONLY, "Numeric device value" },
		{ "sValue", T_OBJECT,	offsetof(CDevice, sValue), READONLY, "String device value" },
		{ NULL }  /* Sentinel */
	};

	static PyObject* CDevice_refresh(CDevice* self)
	{
		return NULL;
	}

	static PyObject* CDevice_str(CDevice* self)
	{
		PyObject*	pNameBytes = PyUnicode_AsASCIIString(self->Name);
		PyObject*	pValueBytes = PyUnicode_AsASCIIString(self->sValue);
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Name: '%s', nValue: %d, sValue: '%s'", self->ID, PyBytes_AsString(pNameBytes), self->nValue, PyBytes_AsString(pValueBytes));
		Py_DECREF(pNameBytes);
		Py_DECREF(pValueBytes);
		return pRetVal;
	}

	static PyMethodDef CDevice_methods[] = {
		{ "Refresh", (PyCFunction)CDevice_refresh, METH_NOARGS, "Refresh details"},
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
		"Domoticz Internal Device instance",           /* tp_doc */
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
		CDevice_new,                 /* tp_new */
	};

	static PyModuleDef CDevice_module = {
		PyModuleDef_HEAD_INIT,
		"Device",
		"Domoticz Device",
		-1,
		NULL, NULL, NULL, NULL, NULL
	};

	PyMODINIT_FUNC PyInit_CDevice(void)
	{
		PyObject* m;

		if (PyType_Ready(&CDeviceType) < 0)
			return NULL;

		m = PyModule_Create(&CDevice_module);
		if (m == NULL)
			return NULL;

		Py_INCREF(&CDeviceType);
		PyModule_AddObject(m, "CDevice", (PyObject *)&CDeviceType);
		return m;
	}

	CPluginMessage::CPluginMessage(ePluginMessageType Type, int HwdID, std::string & Message)
	{
		m_Type = Type;
		m_HwdID = HwdID;
		m_Message = Message;
	}

	CPluginMessage::CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, std::string & Message)
	{
		m_Type = Type;
		m_Directive = dType;
		m_HwdID = HwdID;
		m_Message = Message;
	}

	CPluginMessage::CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID)
	{
		m_Type = Type;
		m_Directive = dType;
		m_HwdID = HwdID;
	}

	CPluginMessage::CPluginMessage(ePluginMessageType Type, int HwdID)
	{
		m_Type = Type;
		m_HwdID = HwdID;
	}

	CPlugin::CPlugin(const int HwdID, const std::string &sName, const std::string &sPluginKey) : m_stoprequested(false)
	{
		m_HwdID = HwdID;
		Name = sName;
		m_PluginKey = sPluginKey;

		m_iPollInterval = 10;

		m_bIsStarted = false;
		m_bDebug = false;

		m_PyInterpreter = NULL;
		m_PyModule = NULL;
	}

	CPlugin::~CPlugin(void)
	{
		m_bIsStarted = false;
	}

	bool CPlugin::StartHardware()
	{
		if (m_bIsStarted) StopHardware();
		m_bIsStarted = true;

		boost::lock_guard<boost::mutex> l(PythonMutex);
		m_PyInterpreter = Py_NewInterpreter();
		if (!m_PyInterpreter)
		{
			_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to create interpreter.", m_PluginKey.c_str());
			return false;
		}

		m_PyModule = PyImport_ImportModule(m_PluginKey.c_str());
		if (!m_PyModule)
		{
			_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to load, Path '%S'.", m_PluginKey.c_str(), Py_GetPath());
			return false;
		}

		//Start worker thread
		m_stoprequested = false;
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPlugin::Do_Work, this)));

		if (!m_thread)
		{
			_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed start worker thread.", m_PluginKey.c_str());
			return false;
		}

		_log.Log(LOG_STATUS, "Plugin: '%s' Started", Name.c_str());

		return true;
	}

	bool CPlugin::StopHardware()
	{
		try
		{
			// Tell transport to disconnect (which will put a PMT_Disconnect message in the queue)

			//	Add stop message to message queue after the diconnect
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

		_log.Log(LOG_STATUS, "Plugin: '%s' Stopped.", Name.c_str());

		return true;
	}

	void CPlugin::Do_Work()
	{
		if (m_PyModule)
		{
			boost::lock_guard<boost::mutex> l(PythonMutex);

			PyObject* pModuleDict = PyModule_GetDict(m_PyModule);  // returns the __dict__ object for the module
			PyObject *pParamsDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Parameters", pParamsDict) == -1)
			{
				_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to add Parameters dictionary.", m_PluginKey.c_str());
			}
			Py_DECREF(pParamsDict);

			PyObject*	pObj = Py_BuildValue("i", m_HwdID);
			if (PyDict_SetItemString(pParamsDict, "HardwareID", pObj) == -1)
			{
				_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to add key 'HardwareID', value '%s' to dictionary.", m_HwdID);
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

			PyObject *pDevicesDict = PyDict_New();
			if (PyDict_SetItemString(pModuleDict, "Devices", pDevicesDict) == -1)
			{
				_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to add Device dictionary.", m_PluginKey.c_str());
			}

			// load associated devices to make them available to python
			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Used==1) ORDER BY Unit ASC", m_HwdID);
			if (result2.size() > 0)
			{
				PyType_Ready(&CDeviceType);
				// Add device objects into the device dictionary with Unit as the key
				for (std::vector<std::vector<std::string> >::const_iterator itt = result2.begin(); itt != result2.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					CDevice* pDevice = (CDevice*)PyObject_New(CDevice, &CDeviceType);

					PyObject*	pKey = PyLong_FromLong(atoi(sd[0].c_str()));
					if (PyDict_SetItem(pDevicesDict, pKey, (PyObject*)pDevice) == -1)
					{
							_log.Log(LOG_ERROR, "CPlugin: Plugin: '%s' failed to add unit '%s' to device dictionary.", sd[0].c_str());
					}
					pDevice->ID = atoi(sd[1].c_str());
					pDevice->Name = PyUnicode_FromString(sd[2].c_str());
					pDevice->nValue = atoi(sd[3].c_str());
					pDevice->sValue = PyUnicode_FromString(sd[4].c_str());
					Py_DECREF(pDevice);
				}
			}

			Py_DECREF(pDevicesDict);
		}

		//	Add start command to message queue
		CPluginMessage	Message(PMT_Start, m_HwdID);
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}

		m_LastHeartbeat = mytime(NULL);
		int scounter = 0;
		while (!m_stoprequested)
		{
			if (scounter++ >= (m_iPollInterval * 2))
			{
				//	Add heartbeat to message queue
				CPluginMessage	Message(PMT_Heartbeat, m_HwdID);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
				scounter = 0;

				m_LastHeartbeat = mytime(NULL);
			}
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "Plugin: '%s' Exiting work loop...", Name.c_str());
	}

	void CPlugin::Restart()
	{
		StopHardware();
		StartHardware();
	}

	void CPlugin::HandleMessage(const CPluginMessage & Message)
	{
		PyEval_RestoreThread(m_PyInterpreter);

		std::string sHandler = "";
		switch (Message.m_Type)
		{
		case PMT_Start:
			sHandler = "onStart";
			break;
		case PMT_Directive:
			switch (Message.m_Directive)
			{
			case PMT_Debug:
				(Message.m_Message == "true") ? m_bDebug = true : m_bDebug = false;
				_log.Log(LOG_NORM, "Plugin: '%s': Debug log level set to: '%s'.", m_PluginKey.c_str(), Message.m_Message.c_str());
				return;
			case PMT_Transport:
				if (m_bDebug) _log.Log(LOG_NORM, "Plugin: '%s': Transport set to: '%s'.", m_PluginKey.c_str(), Message.m_Message.c_str());
				return;
			case PMT_Protocol:
				if (m_bDebug) _log.Log(LOG_NORM, "Plugin: '%s': Protocol set to: '%s'.", m_PluginKey.c_str(), Message.m_Message.c_str());
				return;
			default:
				_log.Log(LOG_ERROR, "Plugin: '%s': Unknown directive type in message: %d.", m_PluginKey.c_str(), Message.m_Directive);
				return;
			}
		case PMT_Connected:
			sHandler = "onConnected";
			break;
		case PMT_Read:
			sHandler = "onRead";
			break;
		case PMT_Heartbeat:
			sHandler = "onHeartbeat";
			break;
		case PMT_Disconnect:
			sHandler = "onDisconnect";
			break;
		case PMT_Command:
			sHandler = "onCommand";
			break;
		case PMT_Stop:
			sHandler = "onStop";
			break;
		default:
			_log.Log(LOG_ERROR, "Plugin: '%s': Unknown message type in message: %d.", m_PluginKey.c_str(), Message.m_Type);
			return;
		}

		PyObject*	pFunc = PyObject_GetAttrString(m_PyModule, sHandler.c_str());
		if (pFunc && PyCallable_Check(pFunc))
		{
			if (m_bDebug) _log.Log(LOG_NORM, "Plugin: '%s': Calling message handler '%s'.", m_PluginKey.c_str(), sHandler.c_str());

			PyErr_Clear();
			PyObject*	pValue = PyObject_CallFunction(pFunc, NULL);
			if (!pValue)
			{
//				PyObject *ptype, *pvalue, *ptraceback;
//				PyErr_Fetch(&ptype, &pvalue, &ptraceback);
//				PyTracebackObject* traceback = ptraceback;
//				PyFrame_GetLineNumber();  // need to iclude frameobject.h to get this
//				if (pvalue)
//				{
//					Py_ssize_t size = PyBytes_Size(pvalue);
//					char* msg = PyBytes_AsString(pvalue);
//					_log.Log(LOG_ERROR, "Plugin: '%s': Call to message handler '%s' failed with error '%s'.", m_PluginKey.c_str(), sHandler.c_str(), msg);
//				}
//				else
//				{
					_log.Log(LOG_ERROR, "Plugin: '%s': Call to message handler '%s' failed.", m_PluginKey.c_str(), sHandler.c_str());
//				}
			}
			else
			{
				Py_XDECREF(pValue);
			}
		}

		if (Message.m_Type == PMT_Stop)
		{
			// Stop Python
			Py_EndInterpreter(m_PyInterpreter);
			Py_XDECREF(m_PyModule);
			m_PyModule = NULL;
			m_PyInterpreter = NULL;
			m_bIsStarted = false;
		}
	}

	bool CPlugin::WriteToHardware(const char *pdata, const unsigned char length)
	{
		const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

		unsigned char packettype = pSen->ICMND.packettype;

		if (packettype != pTypeLighting2)
			return false;

		long	DevID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;
		/*
			std::vector<boost::shared_ptr<CPluginBase> >::iterator itt;
			for (itt = m_pPluginDevices.begin(); itt != m_pPluginDevices.end(); ++itt)
			{
				if ((*itt)->m_DevID == DevID)
				{
					if ((*itt)->IsOn()) {
						int iParam = pSen->LIGHTING2.level;
						switch (pSen->LIGHTING2.cmnd)
						{
						case light2_sOff:
						case light2_sGroupOff:
							(*itt)->SendCommand("off");
						case gswitch_sStop:
							(*itt)->SendCommand("stop");
							return true;
						case gswitch_sPlay:
							(*itt)->SendCommand("play");
							return true;
						case gswitch_sPause:
							(*itt)->SendCommand("pause");
							return true;
						case gswitch_sSetVolume:
							(*itt)->SendCommand("setvolume", iParam);
							return true;
						case gswitch_sPlayPlaylist:
							(*itt)->SendCommand("playlist", iParam);
							return true;
						case gswitch_sPlayFavorites:
							(*itt)->SendCommand("favorites", iParam);
							return true;
						case gswitch_sExecute:
							(*itt)->SendCommand("execute", iParam);
							return true;
						default:
							return true;
						}
					}
					else
						_log.Log(LOG_NORM, "CPluginSystem: (%s) Command not sent, Device is 'Off'.", (*itt)->Name().c_str());
				}
			}
		*/
		_log.Log(LOG_ERROR, "CPluginSystem: (%d) Shutdown. Device not found.", DevID);
		return false;
	}

	void CPlugin::SendCommand(const int ID, const std::string &command)
	{
		/*	std::vector<boost::shared_ptr<CPluginBase> >::iterator itt;
			for (itt = m_pPluginDevices.begin(); itt != m_pPluginDevices.end(); ++itt)
			{
				if ((*itt)->m_ID == ID)
				{
					(*itt)->SendCommand(command);
					return;
				}
			}
		*/
		_log.Log(LOG_ERROR, "Plugin: (%d) Command: '%s'. Device not found.", ID, command.c_str());
	}

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

		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPluginSystem::Do_Work, this)));

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
				_log.Log(LOG_ERROR, "PluginSystem: Invalid Python version '%s' found, '%s' or above required.", sVersion.c_str(), MINIMUM_PYTHON_VERSION);
				return false;
			}

			// Set program name, this prevents it being set to 'python'
			Py_SetProgramName(Py_GetProgramFullPath());

			// Append 'plugins' directory to path so that our plugin files are found
			std::wstring	sPath(Py_GetProgramFullPath());
			sPath = sPath.substr(0, sPath.find(L"omoticz")+7);
#ifdef WIN32
			sPath += L"\\plugins\\";
			sPath = L";" + sPath;
#else
			sPath += L"/plugins/";
			sPath = L":" + sPath;
#endif
			sPath = Py_GetPath() + sPath;
			Py_SetPath(sPath.c_str());

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

	CPlugin * CPluginSystem::RegisterPlugin(const int HwdID, const std::string & Name, const std::string & PluginKey)
	{
		CPlugin*	pPlugin = NULL;
		if (m_bEnabled)
		{
			pPlugin = new CPlugin(HwdID, Name, PluginKey);
			m_pPlugins.insert(std::pair<int, CPlugin*>(HwdID, pPlugin));
		}
		else
		{
			_log.Log(LOG_STATUS, "PluginSystem: '%s' Registration ignored, Plugins are not enabled.", Name.c_str());
		}
		return pPlugin;
	}

	void CPluginSystem::Do_Work()
	{
		while (!m_bAllPluginsStarted)
		{
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "PluginSystem: Entering work loop.");

		while (!m_stoprequested)
		{
			while (!PluginMessageQueue.empty())
			{
				CPluginMessage Message = PluginMessageQueue.front();
				if (!m_pPlugins.count(Message.m_HwdID))
				{
					_log.Log(LOG_ERROR, "PluginSystem: Unknown hardware in message: %d.", Message.m_HwdID);
				}
				else
				{
					CPlugin*	pPlugin = m_pPlugins[Message.m_HwdID];
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
		if (Py_IsInitialized()) {
			Py_Finalize();
		}

		m_bAllPluginsStarted = false;

		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread.reset();
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
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::PluginList(Json::Value &root)
		{
			//
			//	Scan plugins folder and load XML plugin manifests
			//
			int		iPluginCnt = root.size();
			std::stringstream plugin_DirT;
#ifdef WIN32
			plugin_DirT << szUserDataFolder << "plugins\\";
#else
			plugin_DirT << szUserDataFolder << "plugins/";
#endif
			std::string plugin_Dir = plugin_DirT.str();
			DIR *lDir;
			struct dirent *ent;
			if ((lDir = opendir(plugin_Dir.c_str())) != NULL)
			{
				while ((ent = readdir(lDir)) != NULL)
				{
					std::string filename = ent->d_name;
					if (dirent_is_file(plugin_Dir, ent))
					{
						if ((filename.length() < 4) || (filename.compare(filename.length() - 4, 4, ".xml") != 0))
						{
							//_log.Log(LOG_STATUS,"Cmd_PluginList: ignore file not .xml: %s",filename.c_str());
						}
						else if (filename.find("_demo.xml") == std::string::npos) //skip demo xml files
						{
							try
							{
								std::stringstream	XmlDocName;
								XmlDocName << plugin_DirT.str() << filename;
								_log.Log(LOG_NORM, "Cmd_PluginList: Loading plugin manifest: '%s'", XmlDocName.str().c_str());
								TiXmlDocument	XmlDoc = TiXmlDocument(XmlDocName.str().c_str());
								if (!XmlDoc.LoadFile(TIXML_ENCODING_UNKNOWN))
								{
									_log.Log(LOG_ERROR, "Cmd_PluginList: Plugin manifest: '%s'. Error '%s' at line %d column %d.", XmlDocName.str().c_str(), XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol());
								}
								else
								{
									TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugins");
									if (pXmlNode) pXmlNode = pXmlNode->FirstChild("plugin");
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
													ATTRIBUTE_VALUE(pXmlEle, "field",    root[iPluginCnt]["parameters"][iParams]["field"]);
													ATTRIBUTE_VALUE(pXmlEle, "label",    root[iPluginCnt]["parameters"][iParams]["label"]);
													ATTRIBUTE_VALUE(pXmlEle, "width",    root[iPluginCnt]["parameters"][iParams]["width"]);
													ATTRIBUTE_VALUE(pXmlEle, "required", root[iPluginCnt]["parameters"][iParams]["required"]);
													ATTRIBUTE_VALUE(pXmlEle, "default",  root[iPluginCnt]["parameters"][iParams]["default"]);

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
							catch (...)
							{
								_log.Log(LOG_ERROR, "Cmd_PluginList: Exeception loading plugin manifest: '%s'", filename.c_str());
							}
						}
					}
				}
				closedir(lDir);
			}
			else {
				_log.Log(LOG_ERROR, "Cmd_PluginList: Error accessing plugins directory %s", plugin_Dir.c_str());
			}
		}

		void CWebServer::Cmd_PluginAdd(int HwdID, std::string& Name, std::string& PluginKey)
		{
			//
			//	Scan plugins folder for pugin definition and create devices specified in manifest
			//
			std::stringstream plugin_DirT;
#ifdef WIN32
			plugin_DirT << szUserDataFolder << "plugins\\";
#else
			plugin_DirT << szUserDataFolder << "plugins/";
#endif
			std::string plugin_Dir = plugin_DirT.str();
			DIR *lDir;
			struct dirent *ent;
			if ((lDir = opendir(plugin_Dir.c_str())) != NULL)
			{
				while ((ent = readdir(lDir)) != NULL)
				{
					std::string filename = ent->d_name;
					if (dirent_is_file(plugin_Dir, ent))
					{
						if ((filename.length() < 4) || (filename.compare(filename.length() - 4, 4, ".xml") != 0))
						{
							//_log.Log(LOG_STATUS,"Cmd_PluginAdd: ignore file not .xml: %s",filename.c_str());
						}
						else if (filename.find("_demo.xml") == std::string::npos) //skip demo xml files
						{
							try
							{
								std::stringstream	XmlDocName;
								XmlDocName << plugin_DirT.str() << filename;
								_log.Log(LOG_NORM, "Cmd_PluginAdd: Loading plugin manifest: '%s'", XmlDocName.str().c_str());
								TiXmlDocument	XmlDoc = TiXmlDocument(XmlDocName.str().c_str());
								if (!XmlDoc.LoadFile(TIXML_ENCODING_UNKNOWN))
								{
									_log.Log(LOG_ERROR, "Cmd_PluginAdd: Plugin manifest: '%s'. Error '%s' at line %d column %d.", XmlDocName.str().c_str(), XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol());
								}
								else
								{
									TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugins");
									if (pXmlNode) pXmlNode = pXmlNode->FirstChild("plugin");
									for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
									{
										TiXmlElement* pXmlEle = pXmlNode->ToElement();
										if (pXmlEle)
										{
											std::string	sNodeKey;
											ATTRIBUTE_VALUE(pXmlEle, "key", sNodeKey);
											if (sNodeKey == PluginKey)
											{
												char szID[40];
												sprintf(szID, "%X%02X%02X%02X", 0, 0, (HwdID & 0xFF00) >> 8, HwdID & 0xFF);

												TiXmlNode* pXmlDevicesNode = pXmlEle->FirstChild("devices");
												if (pXmlDevicesNode) pXmlDevicesNode = pXmlDevicesNode->FirstChild("device");
												for (pXmlDevicesNode; pXmlDevicesNode; pXmlDevicesNode = pXmlDevicesNode->NextSiblingElement())
												{
													// <devices>
													//	  <device name = "Status" type = "17" subtype = "0" switchtype = "17" customicon = "" / >
													TiXmlElement* pXmlEle = pXmlDevicesNode->ToElement();
													if (pXmlEle)
													{
														std::string sDeviceName;
														int			iUnit;
														int			iType;
														int			iSubType;
														int			iSwitchType;
														int			iIcon;

														ATTRIBUTE_VALUE(pXmlEle, "name", sDeviceName);
														sDeviceName = Name + " - " + sDeviceName;

														ATTRIBUTE_NUMBER(pXmlEle, "unit", iUnit);
														ATTRIBUTE_NUMBER(pXmlEle, "type", iType);
														ATTRIBUTE_NUMBER(pXmlEle, "subtype", iSubType);
														ATTRIBUTE_NUMBER(pXmlEle, "switchtype", iSwitchType);
														ATTRIBUTE_NUMBER(pXmlEle, "icon", iIcon);

														//Also add a light (push) device
														m_sql.safe_query(
															"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage) "
															"VALUES (%d, '%q', %d, %d, %d, %d, 1, 12, 255, '%q', 0, '', %d)",
															HwdID, szID, iUnit, iType, iSubType, iSwitchType, sDeviceName.c_str(), iIcon);

													}
												}
												return;
											}
										}
									}
								}
							}
							catch (...)
							{
								_log.Log(LOG_ERROR, "Cmd_PluginAdd: Exeception loading plugin manifest: '%s'", filename.c_str());
							}
						}
					}
				}
				closedir(lDir);
			}
			else {
				_log.Log(LOG_ERROR, "Cmd_PluginAdd: Error accessing plugins directory %s", plugin_Dir.c_str());
			}
		}
	}
}

