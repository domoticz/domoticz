#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include <tinyxml.h>

#include "PluginManager.h"
#include "Plugins.h"
#include "PluginMessages.h"
#include "PluginTransports.h"

#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../main/EventSystem.h"
#include "../json/json.h"
#include "../main/localtime_r.h"
#ifdef WIN32
#	include <direct.h>
#else
#	include <sys/stat.h>
#endif

#include "DelayedLink.h"

#ifdef ENABLE_PYTHON
#include "../../main/EventsPythonModule.h"
#endif

#define MINIMUM_PYTHON_VERSION "3.4.0"

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

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

namespace Plugins {

	PyMODINIT_FUNC PyInit_Domoticz(void);

#ifdef ENABLE_PYTHON
    // Need forward decleration
    // PyMODINIT_FUNC PyInit_DomoticzEvents(void);
#endif // ENABLE_PYTHON

	boost::mutex PluginMutex;	// controls accessto the message queue
	std::queue<CPluginMessageBase*>	PluginMessageQueue;
	boost::asio::io_service ios;

	std::map<int, CDomoticzHardwareBase*>	CPluginSystem::m_pPlugins;
	std::map<std::string, std::string>		CPluginSystem::m_PluginXml;

	CPluginSystem::CPluginSystem() : m_stoprequested(false)
	{
		m_bEnabled = false;
		m_bAllPluginsStarted = false;
		m_iPollInterval = 10;
		m_InitialPythonThread = NULL;
		m_thread = NULL;
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

		if (!Py_LoadLibrary())
		{
			_log.Log(LOG_STATUS, "PluginSystem: Failed dynamic library load, install the latest libpython3.x library that is available for your platform.");
			return false;
		}

		// Pull UI elements from plugins and create manifest map in memory
		BuildManifest();

		m_thread = new boost::thread(boost::bind(&CPluginSystem::Do_Work, this));

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
				_log.Log(LOG_STATUS, "PluginSystem: Invalid Python version '%s' found, '%s' or above required.", sVersion.c_str(), MINIMUM_PYTHON_VERSION);
				return false;
			}

			// Set program name, this prevents it being set to 'python'
			Py_SetProgramName(Py_GetProgramFullPath());

			if (PyImport_AppendInittab("Domoticz", PyInit_Domoticz) == -1)
			{
				_log.Log(LOG_ERROR, "PluginSystem: Failed to append 'Domoticz' to the existing table of built-in modules.");
				return false;
			}

			if (PyImport_AppendInittab("DomoticzEvents", PyInit_DomoticzEvents) == -1)
			{
				_log.Log(LOG_ERROR, "PluginSystem: Failed to append 'DomoticzEvents' to the existing table of built-in modules.");
				return false;
			}

			Py_Initialize();
			m_InitialPythonThread = PyEval_SaveThread();

			m_bEnabled = true;
			_log.Log(LOG_STATUS, "PluginSystem: Started, Python version '%s'.", sVersion.c_str());
		}
		catch (...) {
			_log.Log(LOG_ERROR, "PluginSystem: Failed to start, Python version '%s', Program '%S', Path '%S'.", sVersion.c_str(), Py_GetProgramFullPath(), Py_GetPath());
			return false;
		}

		return true;
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
		std::vector<std::string>::const_iterator itt_Dir, itt_File;
		std::string plugin_Dir, plugin_File;

		DirectoryListing(DirEntries, plugin_BaseDir, true, false);
		for (itt_Dir = DirEntries.begin(); itt_Dir != DirEntries.end(); ++itt_Dir)
		{
			if (*itt_Dir != "examples")
			{
#ifdef WIN32
				plugin_Dir = plugin_BaseDir + *itt_Dir + "\\";
#else
				plugin_Dir = plugin_BaseDir + *itt_Dir + "/";
#endif
				DirectoryListing(FileEntries, plugin_Dir, false, true);
				for (itt_File = FileEntries.begin(); itt_File != FileEntries.end(); ++itt_File)
				{
					if (*itt_File == "plugin.py")
					{
						try
						{
							std::string sXML;
							plugin_File = plugin_Dir + *itt_File;
							std::string line;
							std::ifstream readFile(plugin_File.c_str());
							bool bFound = false;
							while (getline(readFile, line)) {
								if (!bFound && (line.find("<plugin") != std::string::npos))
									bFound = true;
								if (bFound)
									sXML += line + "\n";
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
		CPlugin*	pPlugin = NULL;
		if (m_bEnabled)
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			pPlugin = new CPlugin(HwdID, Name, PluginKey);
			m_pPlugins.insert(std::pair<int, CPlugin*>(HwdID, pPlugin));
		}
		else
		{
			_log.Log(LOG_STATUS, "PluginSystem: '%s' Registration ignored, Plugins are not enabled.", Name.c_str());
		}
		return (CDomoticzHardwareBase*)pPlugin;
	}

	void CPluginSystem::DeregisterPlugin(const int HwdID)
	{
		if (m_pPlugins.count(HwdID))
		{
			boost::lock_guard<boost::mutex> l(PluginMutex);
			m_pPlugins.erase(HwdID);
		}
	}

	void CPluginSystem::Do_Work()
	{
		while (!m_bAllPluginsStarted)
		{
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "PluginSystem: Entering work loop.");

		// Create initial IO Service thread
		ios.reset();
		boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));

		while (!m_stoprequested)
		{
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations if there are any transports that need it
			{
				bool bIos_required = false;
				for (std::map<int, CDomoticzHardwareBase*>::iterator itt = m_pPlugins.begin(); itt != m_pPlugins.end(); itt++)
				{
					CPlugin*	pPlugin = (CPlugin*)itt->second;
					if (pPlugin && pPlugin->IoThreadRequired())
					{
						bIos_required = true;
						break;
					}
				}

				if (bIos_required)
				{
					ios.reset();
					_log.Log(LOG_NORM, "PluginSystem: Restarting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}

			time_t	Now = time(0);
			bool	bProcessed = true;
			while (bProcessed)
			{
				CPluginMessageBase* Message = NULL;
				bProcessed = false;

				// Cycle once through the queue looking for the 1st message that is ready to process
				for (size_t i = 0; i < PluginMessageQueue.size(); i++)
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					CPluginMessageBase* FrontMessage = PluginMessageQueue.front();
					PluginMessageQueue.pop();
					if (FrontMessage->m_When <= Now)
					{
						// Message is ready now or was already ready (this is the case for almost all messages)
						Message = FrontMessage;
						break;
					}
					// Message is for sometime in the future so requeue it (this happens when the 'Delay' parameter is used on a Send)
					PluginMessageQueue.push(FrontMessage);
				}

				if (Message)
				{
					bProcessed = true;
					try
					{
						Message->Process();
					}
					catch(...)
					{
						_log.Log(LOG_ERROR, "PluginSystem: Exception processing message.");
					}
				}
				// Free the memory for the message
				if (Message) delete Message;
			}
			sleep_milliseconds(50);
		}

		_log.Log(LOG_STATUS, "PluginSystem: Exiting work loop.");
	}

	bool CPluginSystem::StopPluginSystem()
	{
		m_bAllPluginsStarted = false;

		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			m_thread = NULL;
		}

		// Hardware should already be stopped to just flush the queue (should already be empty)
		boost::lock_guard<boost::mutex> l(PluginMutex);
		while (!PluginMessageQueue.empty())
		{
			PluginMessageQueue.pop();
		}

		m_pPlugins.clear();

		if (Py_LoadLibrary()  && m_InitialPythonThread)
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
		boost::lock_guard<boost::mutex> l(PluginMutex);
		for (std::map<int, CDomoticzHardwareBase*>::iterator itt = m_pPlugins.begin(); itt != m_pPlugins.end(); itt++)
		{
			if (itt->second)
			{
				SettingsDirective*	Message = new SettingsDirective((CPlugin*)itt->second);
				PluginMessageQueue.push(Message);
			}
			else
			{
				_log.Log(LOG_ERROR, "%s: NULL entry found in Plugins map for Hardware %d.", __func__, itt->first);
			}
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::PluginList(Json::Value &root)
		{
			int		iPluginCnt = root.size();
			Plugins::CPluginSystem Plugins;
			std::map<std::string, std::string>*	PluginXml = Plugins.GetManifest();
			for (std::map<std::string, std::string>::iterator it_type = PluginXml->begin(); it_type != PluginXml->end(); it_type++)
			{
				TiXmlDocument	XmlDoc;
				XmlDoc.Parse(it_type->second.c_str());
				if (XmlDoc.Error())
				{
					_log.Log(LOG_ERROR, "%s: Parsing '%s', '%s' at line %d column %d in XML '%s'.", __func__, it_type->first.c_str(), XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), it_type->second.c_str());
				}
				else
				{
					TiXmlNode* pXmlNode = XmlDoc.FirstChild("plugin");
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
									ATTRIBUTE_VALUE(pXmlEle, "field", root[iPluginCnt]["parameters"][iParams]["field"]);
									ATTRIBUTE_VALUE(pXmlEle, "label", root[iPluginCnt]["parameters"][iParams]["label"]);
									ATTRIBUTE_VALUE(pXmlEle, "width", root[iPluginCnt]["parameters"][iParams]["width"]);
									ATTRIBUTE_VALUE(pXmlEle, "required", root[iPluginCnt]["parameters"][iParams]["required"]);
									ATTRIBUTE_VALUE(pXmlEle, "default", root[iPluginCnt]["parameters"][iParams]["default"]);

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
			Plugins::CPlugin*	pPlugin = NULL;

			// Disabled plugins will not be in plugin hardware map
			if (PluginHwd->count(HwdID))
			{
				pPlugin = (Plugins::CPlugin*)(*PluginHwd)[HwdID];
			}

			if (pPlugin)
			{
				std::string	sKey = "key=\"" + pPlugin->m_PluginKey + "\"";
				std::map<std::string, std::string>*	PluginXml = Plugins.GetManifest();
				for (std::map<std::string, std::string>::iterator it_type = PluginXml->begin(); it_type != PluginXml->end(); it_type++)
				{
					if (it_type->second.find(sKey) != std::string::npos)
					{
						TiXmlDocument	XmlDoc;
						XmlDoc.Parse(it_type->second.c_str());
						if (XmlDoc.Error())
						{
							_log.Log(LOG_ERROR, "%s: Error '%s' at line %d column %d in XML '%s'.", __func__, XmlDoc.ErrorDesc(), XmlDoc.ErrorRow(), XmlDoc.ErrorCol(), it_type->second.c_str());
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
			result = m_sql.safe_query("SELECT HardwareID, Unit FROM DeviceStatus WHERE (ID=='%q') ", sIdx.c_str());
			if (result.size() == 1)
			{
				int HwID = atoi(result[0][0].c_str());
				int Unit = atoi(result[0][1].c_str());
				Plugins::CPluginSystem Plugins;
				std::map<int, CDomoticzHardwareBase*>*	PluginHwd = Plugins.GetHardware();
				Plugins::CPlugin*	pPlugin = (Plugins::CPlugin*)(*PluginHwd)[HwID];
				if (pPlugin)
				{
					pPlugin->SendCommand(Unit, sAction, 0, 0);
				}
			}
		}
	}
}
#endif
