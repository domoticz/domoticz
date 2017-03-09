#pragma once

#include "../DomoticzHardware.h"

#ifndef byte
typedef unsigned char byte;
#endif

namespace Plugins {

	class CPluginMessage;
	class CPluginProtocol;
	class CPluginTransport;

	class CPlugin : public CDomoticzHardwareBase
	{
	private:
		int				m_iPollInterval;

		void*			m_PyInterpreter;
		void*			m_PyModule;

		std::string		m_Username;
		std::string		m_Password;
		std::string		m_Version;
		std::string		m_Author;

		boost::shared_ptr<boost::thread> m_thread;

		bool StartHardware();
		void Do_Work();
		bool StopHardware();
		void LogPythonException();
		void LogPythonException(const std::string &);
		bool HandleInitialise();
		bool HandleStart();
		bool LoadSettings();
		void WriteDebugBuffer(const std::vector<byte>& Buffer, bool Incoming);

	public:
		CPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		~CPlugin(void);

		void HandleMessage(const CPluginMessage* Message);

		bool WriteToHardware(const char *pdata, const unsigned char length);
		void Restart();
		void SendCommand(const int Unit, const std::string &command, const int level, const int hue);
		void SendCommand(const int Unit, const std::string &command, const float level);

		std::string			m_PluginKey;
		CPluginProtocol*	m_pProtocol;
		CPluginTransport*	m_pTransport;
		void*				m_DeviceDict;
		void*				m_ImageDict;
		void*				m_SettingsDict;
		std::string			m_HomeFolder;
		bool				m_bDebug;
		bool				m_stoprequested;
	};

}