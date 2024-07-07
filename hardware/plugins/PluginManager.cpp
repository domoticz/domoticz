#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "PluginManager.h"
#include "Plugins.h"
#include "PluginMessages.h"
#include "PluginTransports.h"

#include <json/json.h>
#include "../../main/EventSystem.h"
#include "../../main/Helper.h"
#include "../../main/mainworker.h"
#include "../../main/Logger.h"
#include "../../main/SQLHelper.h"
#include "../../main/WebServer.h"
#include "../../tinyxpath/tinyxml.h"
#ifdef WIN32
#	include <direct.h>
#else
#	include <sys/stat.h>
#endif

#include "DelayedLink.h"
#include "../../main/EventsPythonModule.h"

// Python version constants
#define MINIMUM_MAJOR_VERSION 3
#define MINIMUM_MINOR_VERSION 4

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

extern std::string szUserDataFolder;
extern std::string szPyVersion;

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

namespace Plugins {

	PyMODINIT_FUNC PyInit_Domoticz(void);
	PyMODINIT_FUNC PyInit_DomoticzEx(void);

	// Need forward decleration
	// PyMODINIT_FUNC PyInit_DomoticzEvents(void);

	std::mutex PluginMutex;	// controls accessto the message queue and m_pPlugins map
	boost::asio::io_service ios;

	std::map<int, CDomoticzHardwareBase*>	CPluginSystem::m_pPlugins;
	std::map<std::string, std::string>		CPluginSystem::m_PluginXml;
	void *CPluginSystem::m_InitialPythonThread;

	CPluginSystem::CPluginSystem()
	{
		m_bEnabled = false;
		m_bAllPluginsStarted = false;
		m_iPollInterval = 10;
	}

	bool CPluginSystem::StartPluginSystem()
	{
		// Flush the message queue (should already be empty)
		std::lock_guard<std::mutex> l(PluginMutex);

		m_pPlugins.clear();

		if (!Py_LoadLibrary())
		{
			_log.Log(LOG_STATUS, "PluginSystem: Failed dynamic library load, install the latest libpython3.x library that is available for your platform.");
			return false;
		}

		// Pull UI elements from plugins and create manifest map in memory
		BuildManifest();

		m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
		SetThreadName(m_thread->native_handle(), "PluginMgr");

		szPyVersion = Py_GetVersion();

		try
		{
			// Make sure Python is not running
			if (Py_IsInitialized()) {
				Py_Finalize();
			}

			std::string sVersion = szPyVersion.substr(0, szPyVersion.find_first_of(' '));

			std::string sMajorVersion = sVersion.substr(0, sVersion.find_first_of('.'));
			if (std::stoi(sMajorVersion) < MINIMUM_MAJOR_VERSION)
			{
				_log.Log(LOG_STATUS, "PluginSystem: Invalid Python version '%s' found, Major version '%d' or above required.", sVersion.c_str(), MINIMUM_MAJOR_VERSION);
				return false;
			}

			std::string sMinorVersion = sVersion.substr(sMajorVersion.length()+1);
			if (std::stoi(sMinorVersion) < MINIMUM_MINOR_VERSION)
			{
				_log.Log(LOG_STATUS, "PluginSystem: Invalid Python version '%s' found, Minor version '%d.%d' or above required.", sVersion.c_str(), MINIMUM_MAJOR_VERSION, MINIMUM_MINOR_VERSION);
				return false;
			}

			// Set program name, this prevents it being set to 'python'
			Py_SetProgramName(Py_GetProgramFullPath());

			if (PyImport_AppendInittab("Domoticz", PyInit_Domoticz) == -1)
			{
				_log.Log(LOG_ERROR, "PluginSystem: Failed to append 'Domoticz' to the existing table of built-in modules.");
				return false;
			}

			if (PyImport_AppendInittab("DomoticzEx", PyInit_DomoticzEx) == -1)
			{
				_log.Log(LOG_ERROR, "PluginSystem: Failed to append 'DomoticzEx' to the existing table of built-in modules.");
				return false;
			}

			if (PyImport_AppendInittab("DomoticzEvents", PyInit_DomoticzEvents) == -1)
			{
				_log.Log(LOG_ERROR, "PluginSystem: Failed to append 'DomoticzEvents' to the existing table of built-in modules.");
				return false;
			}

			Py_Initialize();

			// Initialise threads. Python 3.7+ does this inside Py_Initialize so done here for compatibility
			if (!PyEval_ThreadsInitialized())
			{
				PyEval_InitThreads();
			}

			m_InitialPythonThread = PyEval_SaveThread();

			m_bEnabled = true;
			_log.Log(LOG_STATUS, "PluginSystem: Started, Python version '%s', %d plugin definitions loaded.", sVersion.c_str(), (int)m_PluginXml.size());
		}
		catch (...) {
			_log.Log(LOG_ERROR, "PluginSystem: Failed to start, Python version '%s', Program '%S', Path '%S'.", szPyVersion.c_str(), Py_GetProgramFullPath(), Py_GetPath());
			return false;
		}

		return true;
	}

	bool CPluginSystem::StopPluginSystem()
	{
		m_bAllPluginsStarted = false;

		if (m_thread)
		{
			RequestStop();
			m_thread->join();
			m_thread.reset();
		}

		m_pPlugins.clear();

		if (Py_LoadLibrary() && m_InitialPythonThread)
		{
			if (Py_IsInitialized()) {
				PyEval_RestoreThread((PyThreadState*)m_InitialPythonThread);
				Py_Finalize();
			}
		}

		_log.Log(LOG_STATUS, "PluginSystem: Stopped.");
		return true;
	}

	void CPluginSystem::LoadSettings()
	{
		//	Add command to message queue for every plugin
		for (const auto &plugin : m_pPlugins)
		{
			if (plugin.second)
			{
				auto pPlugin = reinterpret_cast<CPlugin *>(plugin.second);
				pPlugin->MessagePlugin(new SettingsDirective());
			}
			else
			{
				_log.Log(LOG_ERROR, "%s: NULL entry found in Plugins map for Hardware %d.", __func__, plugin.first);
			}
		}
	}

	void CPluginSystem::BuildManifest()
	{
		//
		//	Scan plugins folder and load XML plugin manifests
		//
		m_PluginXml.clear();
		std::string plugin_BaseDir;
#ifdef WIN32
		plugin_BaseDir = szUserDataFolder + "plugins\\";
#else
		plugin_BaseDir = szUserDataFolder + "plugins/";
#endif
		if (!createdir(plugin_BaseDir.c_str(), 0755))
		{
			_log.Log(LOG_NORM, "%s: Created directory %s", __func__, plugin_BaseDir.c_str());
		}

		std::vector<std::string> DirEntries, FileEntries;
		std::string plugin_Dir, plugin_File;

		DirectoryListing(DirEntries, plugin_BaseDir, true, false);
		for (const auto &dir : DirEntries)
		{
			if (dir != "examples")
			{
#ifdef WIN32
				plugin_Dir = plugin_BaseDir + dir + "\\";
#else
				plugin_Dir = plugin_BaseDir + dir + "/";
#endif
				DirectoryListing(FileEntries, plugin_Dir, false, true);
				for (const auto &file : FileEntries)
				{
					if (file == "plugin.py")
					{
						try
						{
							std::string sXML;
							plugin_File = plugin_Dir + file;
							std::string line;
							std::ifstream readFile(plugin_File.c_str());
							bool bFound = false;
							while (getline(readFile, line)) {
								if (!bFound && (line.find("<plugin") != std::string::npos))
									bFound = true;
								if (bFound)
									sXML += line + '\n';
								if (line.find("</plugin>") != std::string::npos)
									break;
							}
							readFile.close();
							m_PluginXml.insert(std::pair<std::string, std::string>(plugin_Dir, sXML));
						}
						catch (...)
						{
							_log.Log(LOG_ERROR, "%s: Exception loading plugin file: '%s'", __func__, plugin_File.c_str());
						}
					}
				}
				FileEntries.clear();
			}
		}
	}

	CDomoticzHardwareBase* CPluginSystem::RegisterPlugin(const int HwdID, const std::string & Name, const std::string & PluginKey)
	{
		CPlugin *pPlugin = nullptr;
		if (m_bEnabled)
		{
			std::lock_guard<std::mutex> l(PluginMutex);
			pPlugin = new CPlugin(HwdID, Name, PluginKey);
			m_pPlugins.insert(std::pair<int, CPlugin*>(HwdID, pPlugin));
		}
		else
		{
			_log.Log(LOG_STATUS, "PluginSystem: '%s' Registration ignored, Plugins are not enabled.", Name.c_str());
		}
		return dynamic_cast<CDomoticzHardwareBase*>(pPlugin);
	}

	void CPluginSystem::DeregisterPlugin(const int HwdID)
	{
		if (m_pPlugins.count(HwdID))
		{
			std::lock_guard<std::mutex> l(PluginMutex);
			m_pPlugins.erase(HwdID);
		}
	}

	void BoostWorkers()
	{
		
		ios.run();
	}

	void CPluginSystem::Do_Work()
	{
		while (!m_bAllPluginsStarted && !IsStopRequested(500))
		{
			continue;
		}

		if (m_pPlugins.size())
		{
			_log.Log(LOG_STATUS, "PluginSystem: %d plugins started.", (int)m_pPlugins.size());
		}

		// Create initial IO Service thread
		ios.restart();
		// Create some work to keep IO Service alive
		auto work = boost::asio::io_service::work(ios);
		boost::thread_group BoostThreads;
		for (int i = 0; i < 1; i++)
		{
			boost::thread*	bt = BoostThreads.create_thread(BoostWorkers);
			SetThreadName(bt->native_handle(), "Plugin_ASIO");
		}

		while (!IsStopRequested(500))
		{
		}

		// Shutdown IO workers
		ios.stop();
		BoostThreads.join_all();

		_log.Log(LOG_STATUS, "PluginSystem: Exited work loop.");
	}

	void CPluginSystem::DeviceModified(uint64_t DevIdx)
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit FROM DeviceStatus WHERE (ID == %" PRIu64 ")", DevIdx);
		if (result.empty())
			return;
		std::vector<std::string> sd = result[0];
		std::string sHwdID = sd[0];
		std::string Unit = sd[2];
		CDomoticzHardwareBase *pHardware = m_mainworker.GetHardwareByIDType(sHwdID, HTYPE_PythonPlugin);
		if (pHardware == nullptr)
			return;
		//std::vector<std::string> sd = result[0];
		//GizMoCuz: Why does this work with UNIT ? Why not use the device idx which is always unique ?
		_log.Debug(DEBUG_NORM, "CPluginSystem::DeviceModified: Notifying plugin %s about modification of device %s", sHwdID.c_str(), Unit.c_str());
		Plugins::CPlugin *pPlugin = (Plugins::CPlugin*)pHardware;
		pPlugin->DeviceModified(sd[1], atoi(Unit.c_str()));
	}
} // namespace Plugins

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::PluginList(Json::Value &root)
		{
			int		iPluginCnt = root.size();
			Plugins::CPluginSystem Plugins;
			std::map<std::string, std::string>*	PluginXml = Plugins.GetManifest();
			for (const auto &type : *PluginXml)
			{
				TiXmlDocument	XmlDoc;
				XmlDoc.Parse(type.second.c_str());
				if (XmlDoc.Error())
				{
					_log.Log(LOG_ERROR,
						 "%s: Parsing '%s', '%s' at line %d column %d in "
						 "XML '%s'.",
						 __func__, type.first.c_str(), XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(),
						 type.second.c_str());
				}
				else
				{
					TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
					TiXmlPrinter Xmlprinter;
					Xmlprinter.SetStreamPrinting();
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

							TiXmlElement* pXmlDescNode = (TiXmlElement*)pXmlEle->FirstChild("description");
							std::string		sDescription;
							if (pXmlDescNode)
							{
								pXmlDescNode->Accept(&Xmlprinter);
								sDescription = Xmlprinter.CStr();
							}
							root[iPluginCnt]["description"] = sDescription;

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
									ATTRIBUTE_VALUE(pXmlEle, "password", root[iPluginCnt]["parameters"][iParams]["password"]);
									ATTRIBUTE_VALUE(pXmlEle, "rows", root[iPluginCnt]["parameters"][iParams]["rows"]);

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

									TiXmlNode* pXmlDescNode = pXmlEle->FirstChild("description");
									if (pXmlDescNode)
									{
										TiXmlPrinter Xmlprinter;
										Xmlprinter.SetStreamPrinting();
										pXmlDescNode->Accept(&Xmlprinter);
										root[iPluginCnt]["parameters"][iParams]["description"] = Xmlprinter.CStr();
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

		void CWebServer::PluginLoadConfig()
		{
			Plugins::CPluginSystem Plugins;
			Plugins.LoadSettings();
		}

		std::string CWebServer::PluginHardwareDesc(int HwdID)
		{
			Plugins::CPluginSystem Plugins;
			std::map<int, CDomoticzHardwareBase*>*	PluginHwd = Plugins.GetHardware();
			std::string		sRetVal = Hardware_Type_Desc(HTYPE_PythonPlugin);
			Plugins::CPlugin *pPlugin = nullptr;

			// Disabled plugins will not be in plugin hardware map
			if (PluginHwd->count(HwdID))
			{
				pPlugin = (Plugins::CPlugin*)(*PluginHwd)[HwdID];
			}

			if (pPlugin)
			{
				std::string	sKey = "key=\"" + pPlugin->m_PluginKey + "\"";
				std::map<std::string, std::string>*	PluginXml = Plugins.GetManifest();
				for (const auto &type : *PluginXml)
				{
					if (type.second.find(sKey) != std::string::npos)
					{
						TiXmlDocument	XmlDoc;
						XmlDoc.Parse(type.second.c_str());
						if (XmlDoc.Error())
						{
							_log.Log(LOG_ERROR,
								 "%s: Error '%s' at line %d column "
								 "%d in XML '%s'.",
								 __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(),
								 type.second.c_str());
						}
						else
						{
							TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
							for (pXmlNode; pXmlNode; pXmlNode = pXmlNode->NextSiblingElement())
							{
								TiXmlElement* pXmlEle = pXmlNode->ToElement();
								if (pXmlEle)
								{
									const char*	pAttributeValue = pXmlEle->Attribute("name");
									if (pAttributeValue) sRetVal = pAttributeValue;
								}
							}
						}
						break;
					}
				}
			}

			return sRetVal;
		}

		void CWebServer::Cmd_PluginCommand(WebEmSession & session, const request& req, Json::Value &root)
		{
			std::string sIdx = request::findValue(&req, "idx");
			std::string sAction = request::findValue(&req, "action");
			if (sIdx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit FROM DeviceStatus WHERE (ID=='%q') ", sIdx.c_str());
			if (result.size() == 1)
			{
				int HwID = atoi(result[0][0].c_str());
				int Unit = atoi(result[0][2].c_str());
				Plugins::CPluginSystem Plugins;
				std::map<int, CDomoticzHardwareBase*>*	PluginHwd = Plugins.GetHardware();
				Plugins::CPlugin*	pPlugin = (Plugins::CPlugin*)(*PluginHwd)[HwID];
				if (pPlugin)
				{
					pPlugin->SendCommand(result[0][1], Unit, sAction, 0, NoColor);
				}
			}
		}
	} // namespace server
} // namespace http
#endif
