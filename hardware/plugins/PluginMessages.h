#pragma once

#include "DelayedLink.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	enum ePluginMessageType {
		PMT_NULL = 0,
		PMT_Initialise,
		PMT_Start,
		PMT_Directive,
		PMT_Connected,
		PMT_Read,
		PMT_Message,
		PMT_Heartbeat,
		PMT_Disconnect,
		PMT_Command,
		PMT_Notification,
		PMT_Stop
	};

	enum ePluginDirectiveType {
		PDT_PollInterval = 0,
		PDT_Transport,
		PDT_Protocol,
		PDT_Connect,
		PDT_Write,
		PDT_Disconnect,
		PDT_Settings
	};

	class CPluginMessage
	{
	public:
		virtual ~CPluginMessage(void) {};

		ePluginMessageType		m_Type;
		ePluginDirectiveType	m_Directive;
		int						m_HwdID;
		int						m_Unit;
		time_t					m_When;

	protected:
		CPluginMessage(ePluginMessageType Type, int HwdID) :
			m_Type(Type), m_HwdID(HwdID), m_Unit(-1) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID) :
			m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1) {
			m_When = time(0);
		};
	};

	class InitializeMessage : public CPluginMessage
	{
	public:
		InitializeMessage(int HwdID) : CPluginMessage(PMT_Initialise, HwdID) {};
	};

	class StartMessage : public CPluginMessage
	{
	public:
		StartMessage(int HwdID) : CPluginMessage(PMT_Start, HwdID) {};
	};

	class ConnectedMessage : public CPluginMessage
	{
	public:
		ConnectedMessage(int HwdID) : CPluginMessage(PMT_Connected, HwdID) {};
		ConnectedMessage(int HwdID, const int Code, const std::string Text) : CPluginMessage(PMT_Connected, HwdID)
		{
			m_Status = Code;
			m_Text = Text;
		};
		int						m_Status;
		std::string				m_Text;
	};

	class ReadMessage : public CPluginMessage
	{
	public:
		ReadMessage(int HwdID, const int ByteCount, const unsigned char* Data) : CPluginMessage(PMT_Read, HwdID)
		{
			m_Buffer.reserve(ByteCount);
			m_Buffer.assign(Data, Data + ByteCount);
		};
		std::vector<byte>		m_Buffer;
	};

	class HeartbeatMessage : public CPluginMessage
	{
	public:
		HeartbeatMessage(int HwdID) : CPluginMessage(PMT_Heartbeat, HwdID) {};
	};

	class DisconnectMessage : public CPluginMessage
	{
	public:
		DisconnectMessage(int HwdID) : CPluginMessage(PMT_Disconnect, HwdID) {};
	};

	class CommandMessage : public CPluginMessage
	{
	public:
		CommandMessage(int HwdID, int Unit, const std::string& Command, const int level, const int hue) : CPluginMessage(PMT_Command, HwdID)
		{
			m_Unit = Unit;
			m_fLevel = -273.15f;
			m_Command = Command;
			m_iLevel = level;
			m_iHue = hue;
		};
		CommandMessage(int HwdID, int Unit, const std::string& Command, const float level) : CPluginMessage(PMT_Command, HwdID)
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
	};

	class ReceivedMessage : public CPluginMessage
	{
	public:
		ReceivedMessage(int HwdID, const std::string& Buffer) : CPluginMessage(PMT_Message, HwdID), m_Status(-1), m_Object(NULL)
		{
			m_Buffer.reserve(Buffer.length());
			m_Buffer.assign((const byte*)Buffer.c_str(), (const byte*)Buffer.c_str()+Buffer.length());
		};
		ReceivedMessage(int HwdID, const std::vector<byte>& Buffer) : CPluginMessage(PMT_Message, HwdID), m_Status(-1), m_Object(NULL)
		{
			m_Buffer = Buffer;
		};
		ReceivedMessage(int HwdID, const std::vector<byte>& Buffer, const int Status, PyObject*	Object) : CPluginMessage(PMT_Message, HwdID)
		{
			m_Buffer = Buffer;
			m_Status = Status;
			m_Object = Object;
		};
		std::vector<byte>		m_Buffer;
		int						m_Status;
		PyObject*				m_Object;
	};

	class NotificationMessage : public CPluginMessage
	{
	public:
		NotificationMessage(int HwdID,
							const std::string& Subject,
							const std::string& Text,
							const std::string& Name,
							const std::string& Status,
							int Priority,
							const std::string& Sound,
							const std::string& ImageFile) : CPluginMessage(PMT_Notification, HwdID)
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
	};

	class StopMessage : public CPluginMessage
	{
	public:
		StopMessage(int HwdID) : CPluginMessage(PMT_Stop, HwdID) {};
	};

	// Base directive message class
	class CDirectiveMessage : public CPluginMessage
	{
	public:
		CDirectiveMessage(ePluginDirectiveType dType, int HwdID) : CPluginMessage(PMT_Directive, dType, HwdID) {};
	};

	class SettingsDirective : public CDirectiveMessage
	{
	public:
		SettingsDirective(int HwdID) : CDirectiveMessage(PDT_Settings, HwdID) {};
	};

	class ConnectDirective : public CDirectiveMessage
	{
	public:
		ConnectDirective(int HwdID) : CDirectiveMessage(PDT_Connect, HwdID) {};
	};

	class DisconnectDirective : public CDirectiveMessage
	{
	public:
		DisconnectDirective(int HwdID) : CDirectiveMessage(PDT_Disconnect, HwdID) {};
	};

	class WriteDirective : public CDirectiveMessage
	{
	public:
		WriteDirective(int HwdID, const Py_buffer* Buffer, const char* URL, const char* Verb, PyObject*	pHeaders, const int Delay) : CDirectiveMessage(PDT_Write, HwdID)
		{
			if (Buffer)
			{
				m_Buffer.reserve((size_t)Buffer->len);
				m_Buffer.assign((const byte*)Buffer->buf, (const byte*)Buffer->buf + Buffer->len);
			}
			
			if (URL) m_URL = URL;
			if (Verb) m_Operation = Verb;
			m_Object = NULL;
			if (pHeaders)
			{
				m_Object = pHeaders;
				Py_INCREF(pHeaders);
			}
			if (Delay) m_When += Delay;
		};
		std::vector<byte>		m_Buffer;
		std::string				m_URL;
		std::string				m_Operation;
		PyObject*				m_Object;
	};

	class ProtocolDirective : public CDirectiveMessage
	{
	public:
		ProtocolDirective(int HwdID, const char* Protocol) : CDirectiveMessage(PDT_Protocol, HwdID)
		{
			m_Protocol = Protocol;
		};
		std::string		m_Protocol;
	};

	class PollIntervalDirective : public CDirectiveMessage
	{
	public:
		PollIntervalDirective(int HwdID, const int PollInterval) : CDirectiveMessage(PDT_PollInterval, HwdID)
		{
			m_Interval = PollInterval;
		};
		int						m_Interval;
	};

	class TransportDirective : public CDirectiveMessage
	{
	public:
		TransportDirective(int HwdID, const char* Transport, const char* Address, const char* Port, int Baud) : CDirectiveMessage(PDT_Transport, HwdID)
		{
			m_Transport = Transport;
			m_Address = Address;
			if (Port) m_Port = Port;
			m_Baud = Baud;
		};
		std::string				m_Transport;
		std::string				m_Address;
		std::string				m_Port;
		int						m_Baud;
	};
}

