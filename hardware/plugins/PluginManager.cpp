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

extern std::string szUserDataFolder;

namespace Plugins {

	boost::mutex PluginMessageListMutex;
	std::queue<CPluginMessage>	PluginMessageList;

	CPluginMessage::CPluginMessage(ePluginMessageType Type, int HwdID, std::string & Message)
	{
		m_Type = Type;
		m_HwdID = HwdID;
		m_Message = Message;
	}

	CPluginMessage::CPluginMessage(ePluginMessageType Type, int HwdID)
	{
		m_Type = Type;
		m_HwdID = HwdID;
	}

	CPluginBase::CPluginBase(const int HwdID)
	{
		m_stoprequested = false;
		m_Busy = false;
		m_Stoppable = false;

		m_HwdID = HwdID;

		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == 1)", m_HwdID, m_szDevID);
		if (result2.size() == 1)
		{
			m_ID = atoi(result2[0][0].c_str());
		}
	}

	CPluginBase::~CPluginBase(void)
	{
		handleDisconnect();
	}

	CPluginManager::CPluginManager(const int HwdID, const std::string &sName, const std::string &sPluginKey) : m_stoprequested(false)
	{
		m_HwdID = HwdID;
		Name = sName;
		m_PluginKey = sPluginKey;

		m_pPluginDevice = NULL;
		m_bIsStarted = false;
	}

	CPluginManager::~CPluginManager(void)
	{
		m_bIsStarted = false;
	}

	bool CPluginManager::StartHardware()
	{
		if (m_bIsStarted) StopHardware();
		m_bIsStarted = true;
//		sOnConnected(this);

		//	Add start command to message queue
		CPluginMessage	StartMessage(PMT_Start, m_HwdID);
		{
			boost::lock_guard<boost::mutex> l(PluginMessageListMutex);
			PluginMessageList.push(StartMessage);
		}

		//Start worker thread
		m_stoprequested = false;
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CPluginManager::Do_Work, this)));
		_log.Log(LOG_STATUS, "PluginManager: '%s' Started", Name.c_str());

		return true;
	}

	bool CPluginManager::StopHardware()
	{
		try
		{
			//	Add stop command to message queue
			CPluginMessage	StopMessage(PMT_Stop, m_HwdID);
			{
				boost::lock_guard<boost::mutex> l(PluginMessageListMutex);
				PluginMessageList.push(StopMessage);
			}

			if (m_thread)
			{
				m_stoprequested = true;
				m_thread->join();
				m_thread.reset();
			}

			if (m_pPluginDevice)
			{
				delete m_pPluginDevice;
				m_pPluginDevice = NULL;
			}
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}

		if (m_bIsStarted)
		{
			_log.Log(LOG_STATUS, "PluginManager: '%s' Stopped", Name.c_str());
			m_bIsStarted = false;
		}

		return true;
	}

	void CPluginManager::Do_Work()
	{
		int scounter = 0;

		//	These exist in the base class and need to be set when heartbeat response recieved
		//	void SetHeartbeatReceived();
		//	time_t m_LastHeartbeat;
		//	time_t m_LastHeartbeatReceive;

		while (!m_stoprequested)
		{
			if (scounter++ >= (m_iPollInterval * 2))
			{
				boost::lock_guard<boost::mutex> l(m_mutex);

				scounter = 0;

				time_t now;
				mytime(&now);
				m_LastHeartbeat = now;
				SetHeartbeatReceived(); // need to do something with this

//				if (m_ios.stopped())  // make sure that there is a boost thread to service i/o operations
//				{
//					m_ios.reset();
//					_log.Log(LOG_NORM, "PluginManager: Restarting I/O service thread.");
//					boost::thread bt(boost::bind(&boost::asio::io_service::run, &m_ios));
//				}
			}
			sleep_milliseconds(500);
		}

		_log.Log(LOG_STATUS, "PluginManager: Worker stopped...");
	}

	void CPluginManager::Restart()
	{
		StopHardware();
		StartHardware();
	}

	bool CPluginManager::WriteToHardware(const char *pdata, const unsigned char length)
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
						_log.Log(LOG_NORM, "CPluginManager: (%s) Command not sent, Device is 'Off'.", (*itt)->Name().c_str());
				}
			}
		*/
		_log.Log(LOG_ERROR, "CPluginManager: (%d) Shutdown. Device not found.", DevID);
		return false;
	}

	void CPluginManager::SendCommand(const int ID, const std::string &command)
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

