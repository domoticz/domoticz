#pragma once

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
		PDT_Disconnect
	};

	class CPluginMessage
	{
	public:
		CPluginMessage() :
			m_Type(PMT_NULL), m_HwdID(-1), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(""), m_Object(NULL) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, int HwdID, const std::string & Message) :
			m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message), m_Object(NULL) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, const std::string & Message) :
			m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Message(Message), m_Object(NULL) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID) :
			m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Object(NULL) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, ePluginDirectiveType dType, int HwdID, int Value) :
			m_Type(Type), m_Directive(dType), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(Value), m_Object(NULL) {
			m_When = time(0);
		};

		void operator=(const CPluginMessage& m)
		{
			m_Type = m.m_Type;
			m_Directive = m.m_Directive;
			m_HwdID = m.m_HwdID;
			m_Unit = m.m_Unit;
			m_When = m.m_When;
			m_Message = m.m_Message;
			m_iValue = m.m_iValue;
			m_iLevel = m.m_iLevel;
			m_fLevel = m.m_fLevel;
			m_iHue = m.m_iHue;
			m_Address = m.m_Address;
			m_Port = m.m_Port;
			m_Operation = m.m_Operation;
			m_Object = m.m_Object;

			m_Subject = m.m_Subject;
			m_Text = m.m_Text;
			m_Name = m.m_Name;
			m_Status = m.m_Status;
			m_Priority = m.m_Priority;
			m_Sound = m.m_Sound;
			m_ImageFile = m.m_ImageFile;
		}

		virtual ~CPluginMessage(void) {};

		ePluginMessageType		m_Type;
		ePluginDirectiveType	m_Directive;
		int						m_HwdID;
		int						m_Unit;
		time_t					m_When;
		std::string				m_Message;
		int						m_iValue;
		int						m_iLevel;
		float					m_fLevel;
		int						m_iHue;
		std::string				m_Address;
		std::string				m_Port;
		std::string				m_Operation;
		void*					m_Object;

		std::string				m_Subject;
		std::string				m_Text;
		std::string				m_Name;
		std::string				m_Status;
		int						m_Priority;
		std::string				m_Sound;
		std::string				m_ImageFile;

	protected:
		CPluginMessage(ePluginMessageType Type, int HwdID) :
			m_Type(Type), m_HwdID(HwdID), m_Unit(-1), m_iLevel(-1), m_iHue(-1), m_iValue(-1), m_Object(NULL) {
			m_When = time(0);
		};
		CPluginMessage(ePluginMessageType Type, int HwdID, int Unit, const std::string & Message, const int level, const int hue) :
			m_Type(Type), m_HwdID(HwdID), m_Unit(Unit), m_iLevel(level), m_iHue(hue), m_iValue(-1), m_Message(Message), m_Object(NULL) {
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
		CommandMessage(int HwdID, int Unit, const std::string & Message, const int level, const int hue) : CPluginMessage(PMT_Command, HwdID, Unit, Message, level, hue)
		{
			m_fLevel = -273.15f;
		};
		CommandMessage(int HwdID, int Unit, const std::string & Message, const float level) : CPluginMessage(PMT_Command, HwdID)
		{
			m_Unit = Unit;
			m_fLevel = level;
			m_Message = Message;
		};
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
	};

	class StopMessage : public CPluginMessage
	{
	public:
		StopMessage(int HwdID) : CPluginMessage(PMT_Stop, HwdID) {};
	};
}

