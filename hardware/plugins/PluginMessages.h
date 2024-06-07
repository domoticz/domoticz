#pragma once

#include "DelayedLink.h"
#include "Plugins.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	extern struct PyModuleDef DomoticzExModuleDef;

	class CPluginMessageBase
	{
	public:
	  virtual ~CPluginMessageBase() = default;
	  ;

	  std::string m_Name;
	  std::string m_DeviceID;
	  int m_Unit;
	  bool m_Delay;
	  time_t m_When;

	protected:
		CPluginMessageBase() : m_Unit(-1), m_Delay(false)
		{
			m_Name = __func__;
			m_When = time(nullptr);
		};
		virtual void ProcessLocked(CPlugin* pPlugin) = 0;
	public:
		virtual const char* Name() { return m_Name.c_str(); };
		virtual void Process(CPlugin*	pPlugin)
		{
			AccessPython	Guard(pPlugin, m_Name.c_str());
			ProcessLocked(pPlugin);
		};
	};

	// Handles lifecycle management of the Python Connection object
	class CHasConnection
	{
	public:
		CHasConnection(CConnection *Connection) : m_pConnection(Connection)
		{
			Py_XINCREF(m_pConnection);
		};
		~CHasConnection()
		{
			Py_XDECREF(m_pConnection);
		}
		CConnection *m_pConnection;
	};

	class InitializeMessage : public CPluginMessageBase
	{
	public:
		InitializeMessage() : CPluginMessageBase() { m_Name = __func__; };
		void Process(CPlugin* pPlugin) override
		{
			pPlugin->Initialise();
		};
		void ProcessLocked(CPlugin* pPlugin) override{};
	};

	// Base callback message class
	class CCallbackBase : public CPluginMessageBase
	{
	protected:
		PyNewRef	m_Target;
		std::string m_Callback;
		void ProcessLocked(CPlugin* pPlugin) override = 0;

	      public:
		CCallbackBase(const std::string &Callback) : CPluginMessageBase(), m_Callback(Callback)
		{
		};
		virtual PyNewRef Callback(CPlugin* pPlugin, PyObject* pParams)
		{
			if (!m_Target)
			{
				m_Target = pPlugin->PythonModule();
			}

			if (m_Target && m_Callback.length())
			{
				Py_INCREF(m_Target);
				return pPlugin->Callback(m_Target, m_Callback, pParams);
			}
			return PyNewRef();
		};
		virtual const char* PythonName() { return m_Callback.c_str(); };

	      protected:
		bool UpdateEventTarget(const std::string DeviceID, int Unit)
		{
			// Used by some events in DomoticzEx.  Looks for callback existance on Unit, Device and Plugin in that order
			PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
			if (pModule)
			{
				module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
				if (pModState)
				{
					PyBorrowedRef	pUnit = pModState->pPlugin->FindUnitInDevice(m_DeviceID, m_Unit);
					if ((pUnit) && (PyObject_HasAttrString(pUnit, m_Callback.c_str())))
					{
						PyNewRef pFunc = PyObject_GetAttrString(pUnit, m_Callback.c_str());
						if (pFunc && PyCallable_Check(pFunc))
						{
							m_Target += pUnit;	// Increment ref count because pFunc will decrement it on function exit
							return true;
						}
					}

					PyBorrowedRef	pDevice = pModState->pPlugin->FindDevice(m_DeviceID);
					if ((pDevice) && (PyObject_HasAttrString(pDevice, m_Callback.c_str())))
					{
						PyNewRef pFunc = PyObject_GetAttrString(pDevice, m_Callback.c_str());
						if (pFunc && PyCallable_Check(pFunc))
						{
							m_Target += pDevice;	// Increment ref count because pFunc will decrement it on function exit
							return true;
						}
					}
				}
				// True signals that DomoticzEx is loaded so both DeviceID and Unit should be used in the callback
				return true;
			}
			return false;
		}
	};

	class onStartCallback : public CCallbackBase
	{
	public:
		onStartCallback() : CCallbackBase("onStart") { m_Name = __func__; };
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  pPlugin->Start();
		  Callback(pPlugin, nullptr);
	  };
	};

	class onHeartbeatCallback : public CCallbackBase
	{
	public:
		onHeartbeatCallback() : CCallbackBase("onHeartbeat")
		{
			m_Name = __func__;
		};
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  Callback(pPlugin, nullptr);
	  };
	};

#ifdef _WIN32
static std::wstring string_to_wstring(const std::string &str, int codepage)
{
	if (str.empty()) return std::wstring();
	int sz = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), 0, 0);
	std::wstring res(sz, 0);
	MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &res[0], sz);
	return res;
}

static std::string wstring_to_string(const std::wstring &wstr, int codepage)
{
	if (wstr.empty()) return std::string();
	int sz = WideCharToMultiByte(codepage, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
	std::string res(sz, 0);
	WideCharToMultiByte(codepage, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
	return res;
}

static std::string get_utf8_from_ansi(const std::string &utf8, int codepage)
{
	std::wstring utf16 = string_to_wstring(utf8, codepage);
	std::string ansi = wstring_to_string(utf16, CP_UTF8);
	return ansi;
}
#endif

	class onConnectCallback : public CCallbackBase, public CHasConnection
	{
	public:
		onConnectCallback(CConnection* Connection, const int Code, const std::string &Text) : CCallbackBase("onConnect"), CHasConnection(Connection), m_Status(Code), m_Text(Text)
		{
			m_Name = __func__;
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
		int						m_Status;
		std::string				m_Text;
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
#ifdef _WIN32
			std::string textUTF8 = get_utf8_from_ansi(m_Text, GetACP());
#else
			std::string textUTF8 = m_Text; // TODO: Is it safe to assume non-Windows will always be UTF-8?
#endif
			PyNewRef pParams = Py_BuildValue("Ois", m_pConnection, m_Status, textUTF8.c_str());
			Callback(pPlugin, pParams);
	  };
	};

	class onTimeoutCallback : public CCallbackBase, public CHasConnection
	{
    public:
		onTimeoutCallback(CConnection *Connection): CCallbackBase("onTimeout"), CHasConnection(Connection) 
		{
			m_Name = __func__;
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
    protected:
		virtual void ProcessLocked(CPlugin* pPlugin) override
		{
			PyNewRef pParams = Py_BuildValue("(O)", m_pConnection);
			Callback(pPlugin, pParams);
		};
	};

	class onDisconnectCallback : public CCallbackBase, public CHasConnection
	{
	public:
		onDisconnectCallback(CConnection* Connection) : CCallbackBase("onDisconnect"), CHasConnection(Connection)
		{
			m_Name = __func__;
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  PyNewRef pParams = Py_BuildValue("(O)", m_pConnection);
		  Callback(pPlugin, pParams);
	  };
	};

	class onDeviceAddedCallback : public CCallbackBase
	{
	public:
		onDeviceAddedCallback(const std::string DeviceID, int Unit) : CCallbackBase("onDeviceAdded")
		{
			m_DeviceID = DeviceID;
			m_Unit = Unit;
		};
	protected:
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->onDeviceAdded(m_DeviceID, m_Unit);

			if (UpdateEventTarget(m_DeviceID, m_Unit))
			{
				if (CUnitEx::isInstance(m_Target))
				{
					PyNewRef pParams = Py_BuildValue("()");
					Callback(pPlugin, pParams);
				}
				else if (CDeviceEx::isInstance(m_Target))
				{
					PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
					Callback(pPlugin, pParams);
				}
				else
				{
					PyNewRef pParams = Py_BuildValue("(si)", m_DeviceID.c_str(), m_Unit);
					Callback(pPlugin, pParams);
				}
			}
			else
			{
				PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
				Callback(pPlugin, pParams);
			}
		};
	};

	class onDeviceModifiedCallback : public CCallbackBase
	{
	public:
		onDeviceModifiedCallback(const std::string DeviceID, int Unit) : CCallbackBase("onDeviceModified")
		{
			m_DeviceID = DeviceID;
			m_Unit = Unit;
		};
	protected:
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->onDeviceModified(m_DeviceID, m_Unit);

			if (UpdateEventTarget(m_DeviceID, m_Unit))
			{
				if (CUnitEx::isInstance(m_Target))
				{
					PyNewRef pParams = Py_BuildValue("()");
					Callback(pPlugin, pParams);
				}
				else if (CDeviceEx::isInstance(m_Target))
				{
					PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
					Callback(pPlugin, pParams);
				}
				else
				{
					PyNewRef pParams = Py_BuildValue("(si)", m_DeviceID.c_str(), m_Unit);
					Callback(pPlugin, pParams);
				}
			}
			else
			{
				PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
				Callback(pPlugin, pParams);
			}
		};
	};

	class onDeviceRemovedCallback : public CCallbackBase
	{
	public:
		onDeviceRemovedCallback(const std::string DeviceID, int Unit) : CCallbackBase("onDeviceRemoved")
		{
			m_DeviceID = DeviceID;
			m_Unit = Unit;
		};
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  if (UpdateEventTarget(m_DeviceID, m_Unit))
		  {
			  if (CUnitEx::isInstance(m_Target))
			  {
				  PyNewRef pParams = Py_BuildValue("()");
				  Callback(pPlugin, pParams);
			  }
			  else if (CDeviceEx::isInstance(m_Target))
			  {
				  PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
				  Callback(pPlugin, pParams);
			  }
			  else
			  {
				  PyNewRef pParams = Py_BuildValue("(si)", m_DeviceID.c_str(), m_Unit);
				  Callback(pPlugin, pParams);
			  }
		  }
		  else
		  {
			  PyNewRef pParams = Py_BuildValue("(i)", m_Unit);
			  Callback(pPlugin, pParams);
		  }

		  pPlugin->onDeviceRemoved(m_DeviceID, m_Unit);
	  };
	};

	class onCommandCallback : public CCallbackBase
	{
	public:
		onCommandCallback(const std::string DeviceID, int Unit, const std::string &Command, const int level, const std::string &color)
			: CCallbackBase("onCommand")
		{
			m_Name = __func__;
			m_DeviceID = DeviceID;
			m_Unit = Unit;
			m_fLevel = -273.15F;
			m_Command = Command;
			m_iLevel = level;
			m_iColor = color;
		};
		onCommandCallback(const std::string DeviceID, int Unit, const std::string &Command, const float level)
			: CCallbackBase("onCommand")
		{
			m_Name = __func__;
			m_DeviceID = DeviceID;
			m_Unit = Unit;
			m_fLevel = level;
			m_Command = Command;
			m_iLevel = -1;
			m_iColor = "";
		};
		std::string				m_Command;
		std::string				m_iColor;
		int						m_iLevel;
		float					m_fLevel;

	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  PyNewRef pParams;
		  if (UpdateEventTarget(m_DeviceID, m_Unit))
		  {
			  if (CUnitEx::isInstance(m_Target))
			  {
				  if (m_fLevel != -273.15F)
				  {
					  pParams = Py_BuildValue("sfs", m_Command.c_str(), m_fLevel, "");
				  }
				  else
				  {
					  pParams = Py_BuildValue("sis", m_Command.c_str(), m_iLevel, m_iColor.c_str());
				  }
			  }
			  else if (CDeviceEx::isInstance(m_Target))
			  {
				  if (m_fLevel != -273.15F)
				  {
					  pParams = Py_BuildValue("isfs", m_Unit, m_Command.c_str(), m_fLevel, "");
				  }
				  else
				  {
					  pParams = Py_BuildValue("isis", m_Unit, m_Command.c_str(), m_iLevel, m_iColor.c_str());
				  }
			  }
			  else
			  {
				  if (m_fLevel != -273.15F)
				  {
					  pParams = Py_BuildValue("sisfs", m_DeviceID.c_str(), m_Unit, m_Command.c_str(), m_fLevel, "");
				  }
				  else
				  {
					  pParams = Py_BuildValue("sisis", m_DeviceID.c_str(), m_Unit, m_Command.c_str(), m_iLevel, m_iColor.c_str());
				  }
			  }
		  }
		  else
		  {
			  if (m_fLevel != -273.15F)
			  {
				  pParams = Py_BuildValue("isfs", m_Unit, m_Command.c_str(), m_fLevel, "");
			  }
			  else
			  {
				  pParams = Py_BuildValue("isis", m_Unit, m_Command.c_str(), m_iLevel, m_iColor.c_str());
			  }
		  }
		  Callback(pPlugin, pParams);
	  };
	};

	class onSecurityEventCallback : public CCallbackBase
	{
	public:
		onSecurityEventCallback(const std::string DeviceID, int Unit, const int level, const std::string &Description)
			: CCallbackBase("onSecurityEvent")
		{
			m_Name = __func__;
			m_DeviceID = DeviceID;
			m_Unit = Unit;
			m_iLevel = level;
			m_Description = Description;
		};
		int				m_iLevel;
		std::string		m_Description;

	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  if (UpdateEventTarget(m_DeviceID, m_Unit))
		  {
			  if (CUnitEx::isInstance(m_Target))
			  {
				  PyNewRef pParams = Py_BuildValue("is", m_iLevel, m_Description.c_str());
				  Callback(pPlugin, pParams);
			  }
			  else if (CDeviceEx::isInstance(m_Target))
			  {
				  PyNewRef pParams = Py_BuildValue("iis", m_Unit, m_iLevel, m_Description.c_str());
				  Callback(pPlugin, pParams);
			  }
			  else
			  {
				  PyNewRef pParams = Py_BuildValue("siis", m_DeviceID.c_str(), m_Unit, m_iLevel, m_Description.c_str());
				  Callback(pPlugin, pParams);
			  }
		  }
		  else
		  {
			  PyNewRef pParams = Py_BuildValue("iis", m_Unit, m_iLevel, m_Description.c_str());
			  Callback(pPlugin, pParams);
		  }
	  };
	};

	class onMessageCallback : public CCallbackBase, public CHasConnection
	{
	public:
		onMessageCallback(CConnection *Connection, const std::string &Buffer) : CCallbackBase("onMessage"), CHasConnection(Connection), m_Data(nullptr)
		{
			m_Name = __func__;
			m_Buffer.reserve(Buffer.length());
			m_Buffer.assign((const byte *)Buffer.c_str(), (const byte *)Buffer.c_str() + Buffer.length());
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
		onMessageCallback(CConnection *Connection, const std::vector<byte> &Buffer): CCallbackBase("onMessage"), CHasConnection(Connection), m_Data(nullptr)
		{
			m_Name = __func__;
			m_Buffer = Buffer;
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
		onMessageCallback(CConnection* Connection, PyObject*	pData) : CCallbackBase("onMessage"), CHasConnection(Connection)
		{
			m_Name = __func__;
			m_Data = pData;
			if (Connection->Target)
			{
				m_Target += Connection->Target;
			}
		};
		std::vector<byte>		m_Buffer;
		PyObject*				m_Data;

	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  PyNewRef pParams;

		  // Data is stored in a single vector of bytes
		  if (!m_Buffer.empty())
		  {
			  PyNewRef	Bytes(m_Buffer);
			  pParams = Py_BuildValue("OO", m_pConnection, (PyObject*)Bytes);
			  Callback(pPlugin, pParams);
		  }

		  // Data is in a dictionary
		  if (m_Data)
		  {
			  pParams = Py_BuildValue("OO", m_pConnection, m_Data);
			  Callback(pPlugin, pParams);
			  Py_XDECREF(m_Data);
		  }
	  }
	};

	class onNotificationCallback : public CCallbackBase
	{
	public:
		onNotificationCallback(CPlugin* pPlugin,
							const std::string& Subject,
							const std::string& Text,
							const std::string& Name,
							const std::string& Status,
							int Priority,
							const std::string& Sound,
							const std::string& ImageFile) : CCallbackBase("onNotification")
		{
			m_Name = __func__;
			m_Subject = Subject;
			m_Text = Text;
			m_SuppliedName = Name;
			m_Status = Status;
			m_Priority = Priority;
			m_Sound = Sound;
			m_ImageFile = ImageFile;
		};

		std::string				m_Subject;
		std::string				m_Text;
		std::string				m_SuppliedName;
		std::string				m_Status;
		int						m_Priority;
		std::string				m_Sound;
		std::string				m_ImageFile;

	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  PyNewRef pParams = Py_BuildValue("ssssiss", m_SuppliedName.c_str(), m_Subject.c_str(), m_Text.c_str(), m_Status.c_str(), m_Priority, m_Sound.c_str(), m_ImageFile.c_str());
		  Callback(pPlugin, pParams);
	  };
	};

	class onStopCallback : public CCallbackBase
	{
	public:
		onStopCallback() : CCallbackBase("onStop") { m_Name = __func__; };
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override
	  {
		  Callback(pPlugin, nullptr);
		  m_Target = Py_None; // Make sure object does not hold any Python memory
		  pPlugin->Stop();
	  };
	};

	// Base directive message class
	class CDirectiveBase : public CPluginMessageBase
	{
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override = 0;

	public:
		CDirectiveBase() : CPluginMessageBase() {};
		void Process(CPlugin* pPlugin) override
		{
			AccessPython	Guard(pPlugin, m_Name.c_str());
			ProcessLocked(pPlugin);
		};
	};

	class ProtocolDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ProtocolDirective(CConnection* Connection) : CDirectiveBase(), CHasConnection(Connection) { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->ConnectionProtocol(this);
		};
	};

	class ConnectDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ConnectDirective(CConnection* Connection) : CDirectiveBase(), CHasConnection(Connection) { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->ConnectionConnect(this);
		};
	};

	class ListenDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ListenDirective(CConnection* Connection) : CDirectiveBase(), CHasConnection(Connection) { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->ConnectionListen(this);
		};
	};

	class DisconnectDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		DisconnectDirective(CConnection* Connection) : CDirectiveBase(), CHasConnection(Connection) { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->ConnectionDisconnect(this);
		};
	};

	class WriteDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		PyObject*		m_Object;
		WriteDirective(CConnection* Connection, PyObject* pData, const int Delay) : CDirectiveBase(), CHasConnection(Connection)
		{
			m_Name = __func__;
			m_Object = pData;
			if (m_Object)
				Py_INCREF(m_Object);
			if (Delay)
			{
				m_When += Delay;
				m_Delay=true;
			}
		};
		~WriteDirective() override
		{
			if (m_Object)
				Py_DECREF(m_Object);
		}

		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->ConnectionWrite(this);
		};
	};

	class SettingsDirective : public CDirectiveBase
	{
	public:
		SettingsDirective() : CDirectiveBase() { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->LoadSettings();
		};
	};

	class PollIntervalDirective : public CDirectiveBase
	{
	public:
		PollIntervalDirective(const int PollInterval) : CDirectiveBase(), m_Interval(PollInterval) { m_Name = __func__; };
		int						m_Interval;
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->PollInterval(m_Interval);
		};
	};

	class NotifierDirective : public CDirectiveBase
	{
	public:
		NotifierDirective(const char* Name) : CDirectiveBase(), m_NotifierName(Name) { m_Name = __func__; };
		std::string		m_NotifierName;
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->Notifier(m_NotifierName);
		};
	};

	// Base event message class
	class CEventBase : public CPluginMessageBase
	{
	protected:
	  void ProcessLocked(CPlugin* pPlugin) override = 0;

	public:
		CEventBase() : CPluginMessageBase() {};
	};

	class ReadEvent : public CEventBase, public CHasConnection
	{
	public:
		ReadEvent(CConnection* Connection, const size_t ByteCount, const unsigned char* Data, const int ElapsedMs = -1) : CEventBase(), CHasConnection(Connection)
		{
			m_Name = __func__;
			m_ElapsedMs = ElapsedMs;
			m_Buffer.reserve(ByteCount);
			m_Buffer.assign(Data, Data + ByteCount);
		};
		std::vector<byte>		m_Buffer;
		int						m_ElapsedMs;
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->WriteDebugBuffer(m_Buffer, true);
			pPlugin->ConnectionRead(this);
		};
	};

	class DisconnectedEvent : public CEventBase, public CHasConnection
	{
	public:
		DisconnectedEvent(CConnection* Connection) : CEventBase(), CHasConnection(Connection), bNotifyPlugin(true) { m_Name = __func__; };
		DisconnectedEvent(CConnection* Connection, bool NotifyPlugin) : CEventBase(), CHasConnection(Connection), bNotifyPlugin(NotifyPlugin) { m_Name = __func__; };
		void ProcessLocked(CPlugin* pPlugin) override
		{
			pPlugin->DisconnectEvent(this);
		};
		bool	bNotifyPlugin;
	};
} // namespace Plugins
