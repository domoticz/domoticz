#pragma once

#include "../DomoticzHardware.h"
#include "../hardwaretypes.h"
#include "../../notifications/NotificationBase.h"
#include "PythonObjects.h"
#include "PythonObjectEx.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	// forward declarations
	class CDirectiveBase;
	class CEventBase;
	class CPluginMessageBase;
	class CPluginNotifier;
	class CPluginTransport;
	class PyNewRef;
	class PyBorrowedRef;

	enum PluginDebugMask
	{
		PDM_NONE = 0,
		// 1 is mapped to PDM_ALL in code for backwards compatibility
		PDM_PYTHON = 2,
		PDM_PLUGIN = 4,
		PDM_DEVICE = 8,
		PDM_CONNECTION = 16,
		PDM_IMAGE = 32,
		PDM_MESSAGE = 64,
		PDM_QUEUE = 128,
		PDM_ALL = 65535
	};

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		int				m_iPollInterval;

		PyThreadState*	m_PyInterpreter;
		PyObject*		m_PyModule;

		std::string		m_Version;
		std::string		m_Author;

		CPluginNotifier*	m_Notifier;

		std::mutex	m_TransportsMutex;
		std::vector<CPluginTransport*>	m_Transports;
		std::mutex m_QueueMutex; // controls access to the message queue
		std::deque<CPluginMessageBase *> m_MessageQueue;

		std::shared_ptr<std::thread> m_thread;

		bool m_bIsStarting;
		bool m_bIsStopped;

		void Do_Work();

		void LogPythonException();
		void LogPythonException(const std::string &);

	public:
	  CPlugin(int HwdID, const std::string &Name, const std::string &PluginKey);
	  ~CPlugin() override;

	  bool StartHardware() override;
	  bool StopHardware() override;

	  int PollInterval(int Interval = -1);
	  PyObject*	PythonModule() { return m_PyModule; };
	  void Notifier(const std::string &Notifier = "");
	  void AddConnection(CPluginTransport *);
	  void RemoveConnection(CPluginTransport *);

	  bool Initialise();
	  bool LoadSettings();
	  bool Start();
	  void ConnectionProtocol(CDirectiveBase *);
	  void ConnectionConnect(CDirectiveBase *);
	  void ConnectionListen(CDirectiveBase *);
	  void ConnectionRead(CPluginMessageBase *);
	  void ConnectionWrite(CDirectiveBase *);
	  void ConnectionDisconnect(CDirectiveBase *);
	  void DisconnectEvent(CEventBase *);
	  void Callback(PyObject* pTarget, const std::string &sHandler, PyObject *pParams);
	  void RestoreThread();
	  void ReleaseThread();
	  void Stop();

	  void WriteDebugBuffer(const std::vector<byte> &Buffer, bool Incoming);

	  bool WriteToHardware(const char *pdata, unsigned char length) override;
	  void SendCommand(const std::string &DeviceID, int Unit, const std::string &command, int level, _tColor color);
	  void SendCommand(const std::string &DeviceID, int Unit, const std::string &command, float level);

	  void onDeviceAdded(const std::string DeviceID, int Unit);
	  void onDeviceModified(const std::string DeviceID, int Unit);
	  void onDeviceRemoved(const std::string DeviceID, int Unit);
	  void MessagePlugin(CPluginMessageBase *pMessage);
	  void DeviceAdded(const std::string DeviceID, int Unit);
	  void DeviceModified(const std::string DeviceID, int Unit);
	  void DeviceRemoved(const std::string DeviceID, int Unit);

	  bool HasNodeFailed(const std::string DeviceID, int Unit);

	  PyBorrowedRef FindDevice(const std::string &Key);
	  PyBorrowedRef	FindUnitInDevice(const std::string &deviceKey, const int unitKey);

	  std::string m_PluginKey;
	  PyDictObject*	m_DeviceDict;
	  PyDictObject* m_ImageDict;
	  PyDictObject* m_SettingsDict;
	  std::string m_HomeFolder;
	  PluginDebugMask m_bDebug;
	  bool m_bTracing;
	};

	class CPluginNotifier : public CNotificationBase
	{
	private:
		CPlugin*	m_pPlugin;
	public:
		CPluginNotifier(CPlugin* pPlugin, const std::string & );
		~CPluginNotifier() override;
		bool IsConfigured() override;
		std::string	 GetIconFile(const std::string &ExtraData);
		std::string GetCustomIcon(std::string & szCustom);
	protected:
	  bool SendMessageImplementation(uint64_t Idx, const std::string &Name, const std::string &Subject, const std::string &Text, const std::string &ExtraData, int Priority,
					 const std::string &Sound, bool bFromNotification) override;
	};

	//
	//	Holds per plugin state details, specifically plugin object, read using PyModule_GetState(PyObject *module)
	//
	struct module_state {
		CPlugin* pPlugin;
		PyObject* error;
		PyTypeObject *pDeviceClass;
		PyTypeObject *pUnitClass;
	};

	//
	//	Controls lifetime of Python Objects to ensure they always release
	//
	class PyBorrowedRef
	{
	      protected:
		PyObject *m_pObject;

	      public:
		PyBorrowedRef()
			: m_pObject(NULL){};
		PyBorrowedRef(PyObject *pObject)
		{
			m_pObject = pObject;
		};
		operator PyObject *() const
		{
			return m_pObject;
		}
		operator PyTypeObject *() const
		{
			return (PyTypeObject *)m_pObject;
		}
		operator PyBytesObject *() const
		{
			return (PyBytesObject *)m_pObject;
		}
		operator bool() const
		{
			return (m_pObject != NULL);
		}
		operator CDevice *() const
		{
			return (CDevice *)m_pObject;
		}
		operator CDeviceEx *() const
		{
			return (CDeviceEx *)m_pObject;
		}
		operator CUnitEx *() const
		{
			return (CUnitEx *)m_pObject;
		}
		operator std::string() const
		{
			if (!m_pObject)
				return std::string("");
			PyObject* pString = PyObject_Str(m_pObject);
			if (!pString)
				return std::string("");
			std::string sUTF8 = PyUnicode_AsUTF8(pString);
			Py_DECREF(pString);
			return sUTF8;
		}
		PyObject **operator&()
		{
			return &m_pObject;
		};
		PyObject *operator->()
		{
			return m_pObject;
		};
		void operator=(PyObject *pObject)
		{
			m_pObject = pObject;
		}
		void operator++()
		{
			if (m_pObject)
			{
				Py_INCREF(m_pObject);
			}
		}
		void operator--()
		{
			if (m_pObject)
			{
				Py_DECREF(m_pObject);
			}
		}
		~PyBorrowedRef()
		{
			m_pObject = NULL;
		};
	};

	class PyNewRef : public PyBorrowedRef
	{
	      public:
		PyNewRef()
			: PyBorrowedRef(){};
		PyNewRef(PyObject *pObject)
			: PyBorrowedRef(pObject){};
		void operator=(PyObject *pObject)
		{
			if (m_pObject)
			{
				Py_XDECREF(m_pObject);
			}
			m_pObject = pObject;
		}
		void operator+=(PyObject *pObject)
		{
			if (m_pObject)
			{
				Py_XDECREF(m_pObject);
			}
			m_pObject = pObject;
			if (m_pObject)
			{
				Py_INCREF(m_pObject);
			}
		}
		~PyNewRef()
		{
			if (m_pObject)
			{
				// Py_CLEAR(m_pObject);  // Need to look into using clear more broadly
				Py_XDECREF(m_pObject);
			}
		};
	};
} // namespace Plugins
