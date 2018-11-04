#pragma once

#include "../../main/StoppableTask.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//

class CDomoticzHardwareBase;

namespace Plugins {

	class CPluginSystem : public StoppableTask
	{
	private:
		bool	m_bEnabled;
		bool	m_bAllPluginsStarted;
		int		m_iPollInterval;

		void*	m_InitialPythonThread;

		static	std::map<int, CDomoticzHardwareBase*>	m_pPlugins;
		static	std::map<std::string, std::string>		m_PluginXml;

		std::shared_ptr<std::thread> m_thread;
		std::mutex m_mutex;

		void Do_Work();
	public:
		CPluginSystem();
		~CPluginSystem(void);

		bool StartPluginSystem();
		void BuildManifest();
		std::map<std::string, std::string>* GetManifest() { return &m_PluginXml; };
		std::map<int, CDomoticzHardwareBase*>* GetHardware() { return &m_pPlugins; };
		CDomoticzHardwareBase* RegisterPlugin(const int HwdID, const std::string &Name, const std::string &PluginKey);
		void	 DeregisterPlugin(const int HwdID);
		bool	StopPluginSystem();
		void	AllPluginsStarted() { m_bAllPluginsStarted = true; };
		static void LoadSettings();
		void	DeviceModified(uint64_t ID);
		void*	PythonThread() { return m_InitialPythonThread; };
	};
};

