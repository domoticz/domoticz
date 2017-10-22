#pragma once

#include "DelayedLink.h"
#include "Plugins.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	class CPluginMessageBase
	{
	public:
		virtual ~CPluginMessageBase(void) {};

		CPlugin*	m_pPlugin;
		int			m_HwdID;
		int			m_Unit;
		time_t		m_When;

	protected:
		CPluginMessageBase(CPlugin* pPlugin) : m_pPlugin(pPlugin), m_HwdID(pPlugin->m_HwdID), m_Unit(-1)
		{
			m_When = time(0);
		};
	public:
		virtual void Process() = 0;
	};

	// Handles lifecycle management of the Python Connection object
	class CHasConnection
	{
	public:
		CHasConnection(PyObject* Connection) : m_pConnection(Connection)
		{
			Py_XINCREF(m_pConnection);
		};
		~CHasConnection()
		{
			Py_XDECREF(m_pConnection);
		}
		PyObject*	m_pConnection;
	};

	class InitializeMessage : public CPluginMessageBase
	{
	public:
		InitializeMessage(CPlugin* pPlugin) : CPluginMessageBase(pPlugin) {};
		virtual void Process()
		{
			m_pPlugin->Initialise();
		};
	};

	// Base callback message class
	class CCallbackBase : public CPluginMessageBase
	{
	protected:
		std::string	m_Callback;
	public:
		CCallbackBase(CPlugin* pPlugin, std::string Callback) : CPluginMessageBase(pPlugin), m_Callback(Callback) {};
		virtual void Callback(PyObject* pParams) { if (m_Callback.length()) m_pPlugin->Callback(m_Callback, pParams); };
		virtual void Process() { throw "Base callback class Handle called."; };
	};

	class StartCallback : public CCallbackBase
	{
	public:
		StartCallback(CPlugin* pPlugin) : CCallbackBase(pPlugin, "onStart") {};
		virtual void Process()
		{
			m_pPlugin->Start();
			Callback(NULL);
		};
	};

	class HeartbeatCallback : public CCallbackBase
	{
	public:
		HeartbeatCallback(CPlugin* pPlugin) : CCallbackBase(pPlugin, "onHeartbeat") {};
		virtual void Process()
		{
			Callback(NULL);
		};
	};

	class ConnectedMessage : public CCallbackBase, public CHasConnection
	{
	public:
		ConnectedMessage(CPlugin* pPlugin, PyObject* Connection) : CCallbackBase(pPlugin, "onConnect"), CHasConnection(Connection) {};
		ConnectedMessage(CPlugin* pPlugin, PyObject* Connection, const int Code, const std::string Text) : CCallbackBase(pPlugin, "onConnect"), CHasConnection(Connection), m_Status(Code), m_Text(Text) {};
		int						m_Status;
		std::string				m_Text;
		virtual void Process()
		{
			Callback(Py_BuildValue("Ois", m_pConnection, m_Status, m_Text.c_str()));  // 0 is success else socket failure code
		};
	};

	class ReadMessage : public CPluginMessageBase, public CHasConnection
	{
	public:
		ReadMessage(CPlugin* pPlugin, PyObject* Connection, const int ByteCount, const unsigned char* Data) : CPluginMessageBase(pPlugin), CHasConnection(Connection)
		{
			m_Buffer.reserve(ByteCount);
			m_Buffer.assign(Data, Data + ByteCount);
		};
		std::vector<byte>		m_Buffer;
		virtual void Process()
		{
			m_pPlugin->WriteDebugBuffer(m_Buffer, true);
			m_pPlugin->ConnectionRead(this);
		};
	};

	class DisconnectMessage : public CCallbackBase, public CHasConnection
	{
	public:
		DisconnectMessage(CPlugin* pPlugin, PyObject* Connection) : CCallbackBase(pPlugin, "onDisconnect"), CHasConnection(Connection) {};
		virtual void Process()
		{
			Callback(Py_BuildValue("(O)", m_pConnection));  // 0 is success else socket failure code
		};
	};

	class CommandMessage : public CCallbackBase
	{
	public:
		CommandMessage(CPlugin* pPlugin, int Unit, const std::string& Command, const int level, const int hue) : CCallbackBase(pPlugin, "onCommand")
		{
			m_Unit = Unit;
			m_fLevel = -273.15f;
			m_Command = Command;
			m_iLevel = level;
			m_iHue = hue;
		};
		CommandMessage(CPlugin* pPlugin, int Unit, const std::string& Command, const float level) : CCallbackBase(pPlugin, "onCommand")
		{
			m_Unit = Unit;
			m_fLevel = level;
			m_Command = Command;
			m_iLevel = -1;
			m_iHue = -1;
		};
		std::string				m_Command;
		int						m_iHue;
		int						m_iLevel;
		float					m_fLevel;

		virtual void Process()
		{
			PyObject*	pParams;
			if (m_fLevel != -273.15f)
			{
				pParams = Py_BuildValue("isfi", m_Unit, m_Command.c_str(), m_fLevel, 0);
			}
			else
			{
				pParams = Py_BuildValue("isii", m_Unit, m_Command.c_str(), m_iLevel, m_iHue);
			}
			Callback(pParams);
		};
	};

	class ReceivedMessage : public CCallbackBase, public CHasConnection
	{
	public:
		ReceivedMessage(CPlugin* pPlugin, PyObject* Connection, const std::string& Buffer) : CCallbackBase(pPlugin, "onMessage"), CHasConnection(Connection), m_Data(NULL)
		{
			m_Buffer.reserve(Buffer.length());
			m_Buffer.assign((const byte*)Buffer.c_str(), (const byte*)Buffer.c_str()+Buffer.length());
		};
		ReceivedMessage(CPlugin* pPlugin, PyObject* Connection, const std::vector<byte>& Buffer) : CCallbackBase(pPlugin, "onMessage"), CHasConnection(Connection), m_Data(NULL)
		{
			m_Buffer = Buffer;
		};
		ReceivedMessage(CPlugin* pPlugin, PyObject* Connection, PyObject*	pData) : CCallbackBase(pPlugin, "onMessage"), CHasConnection(Connection)
		{
			m_Data = pData;
		};
		std::vector<byte>		m_Buffer;
		PyObject*				m_Data;

		virtual void Process()
		{
			PyObject*	pParams = NULL;

			// Data is stored in a single vector of bytes
			if (m_Buffer.size())
			{
				pParams = Py_BuildValue("Oy#", m_pConnection, &m_Buffer[0], m_Buffer.size());
				Callback(pParams);
			}

			// Data is in a dictionary
			if (m_Data)
			{
				pParams = Py_BuildValue("OO", m_pConnection, m_Data);
				Callback(pParams);
				Py_XDECREF(m_Data);
			}
		}
	};

	class NotificationMessage : public CCallbackBase
	{
	public:
		NotificationMessage(CPlugin* pPlugin,
							const std::string& Subject,
							const std::string& Text,
							const std::string& Name,
							const std::string& Status,
							int Priority,
							const std::string& Sound,
							const std::string& ImageFile) : CCallbackBase(pPlugin, "onNotification")
		{
			m_Subject = Subject;
			m_Text = Text;
			m_Name = Name;
			m_Status = Status;
			m_Priority = Priority;
			m_Sound = Sound;
			m_ImageFile = ImageFile;
		};

		std::string				m_Subject;
		std::string				m_Text;
		std::string				m_Name;
		std::string				m_Status;
		int						m_Priority;
		std::string				m_Sound;
		std::string				m_ImageFile;

		virtual void Process()
		{
			PyObject*	pParams = Py_BuildValue("ssssiss", m_Name.c_str(), m_Subject.c_str(), m_Text.c_str(), m_Status.c_str(), m_Priority, m_Sound.c_str(), m_ImageFile.c_str());
			Callback(pParams);
		};
	};

	class StopMessage : public CCallbackBase
	{
	public:
		StopMessage(CPlugin* pPlugin) : CCallbackBase(pPlugin, "onStop") {};
		virtual void Process()
		{
			Callback(NULL);
			m_pPlugin->Stop();
		};
	};

	// Base directive message class
	class CDirectiveBase : public CPluginMessageBase
	{
	public:
		CDirectiveBase(CPlugin* pPlugin) : CPluginMessageBase(pPlugin) {};
		virtual void Process() { throw "Base directive class Handle called"; };
	};

	class ProtocolDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ProtocolDirective(CPlugin* pPlugin, PyObject* Connection) : CDirectiveBase(pPlugin), CHasConnection(Connection) {};
		virtual void Process() { m_pPlugin->ConnectionProtocol(this); };
	};

	class ConnectDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ConnectDirective(CPlugin* pPlugin, PyObject* Connection) : CDirectiveBase(pPlugin), CHasConnection(Connection) {};
		virtual void Process() { m_pPlugin->ConnectionConnect(this); };
	};

	class ListenDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		ListenDirective(CPlugin* pPlugin, PyObject* Connection) : CDirectiveBase(pPlugin), CHasConnection(Connection) {};
		virtual void Process() { m_pPlugin->ConnectionListen(this); };
	};

	class DisconnectDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		DisconnectDirective(CPlugin* pPlugin, PyObject* Connection) : CDirectiveBase(pPlugin), CHasConnection(Connection) {};
		virtual void Process() { m_pPlugin->ConnectionDisconnect(this); };
	};

	class WriteDirective : public CDirectiveBase, public CHasConnection
	{
	public:
		PyObject*		m_Object;
		WriteDirective(CPlugin* pPlugin, PyObject* Connection, PyObject* pData, const int Delay) : CDirectiveBase(pPlugin), CHasConnection(Connection)
		{
			m_Object = pData;
			if (m_Object)
				Py_INCREF(m_Object);
			if (Delay) m_When += Delay;
		};
		~WriteDirective()
		{
			if (m_Object)
				Py_DECREF(m_Object);
		}

		virtual void Process() { m_pPlugin->ConnectionWrite(this); };
	};

	class SettingsDirective : public CDirectiveBase
	{
	public:
		SettingsDirective(CPlugin* pPlugin) : CDirectiveBase(pPlugin) {};
		virtual void Process() { m_pPlugin->LoadSettings(); };
	};

	class PollIntervalDirective : public CDirectiveBase
	{
	public:
		PollIntervalDirective(CPlugin* pPlugin, const int PollInterval) : CDirectiveBase(pPlugin), m_Interval(PollInterval) {};
		int						m_Interval;
		virtual void Process() {m_pPlugin->PollInterval(m_Interval); };
	};

	class NotifierDirective : public CDirectiveBase
	{
	public:
		NotifierDirective(CPlugin* pPlugin, const char* Name) : CDirectiveBase(pPlugin), m_Name(Name) {};
		std::string		m_Name;
		virtual void Process() { m_pPlugin->Notifier(m_Name); };
	};

	// Base event message class
	class CEventBase : public CPluginMessageBase
	{
	public:
		CEventBase(CPlugin* pPlugin) : CPluginMessageBase(pPlugin) {};
		virtual void Process() { throw "Base event class Handle called"; };
	};

	class DisconnectedEvent : public CEventBase, public CHasConnection
	{
	public:
		DisconnectedEvent(CPlugin* pPlugin, PyObject* Connection) : CEventBase(pPlugin), CHasConnection(Connection) {};
		virtual void Process() { m_pPlugin->DisconnectEvent(this); };
	};
}
