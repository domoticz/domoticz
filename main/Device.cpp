#include "stdafx.h"
#include "Logger.h"
#include "Device.h"
#include "SQLHelper.h"
#include "Helper.h"

extern std::string szUserDataFolder;
extern std::string szWWWFolder;

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

namespace Devices
{
	CSwitchBase::CSwitchBase()
	{
	}
	CSwitchBase::~CSwitchBase()
	{
	}

	CSwitchBinary::CSwitchBinary()
	{
	}

	bool CSwitchBinary::LoadSubDevices(DeviceID DomoticzID, CDeviceMapPtr DeviceList)
	{
		//
		//	Switches can have sub devices so check if this one does
		//
		m_SubDevices.clear();
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT DeviceRowID FROM LightSubDevices WHERE ParentID = %d ORDER BY [DeviceRowID]", DomoticzID);
		if (result.size() > 0)
		{
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;
				CDeviceMapIter	DMi = DeviceList->find(atoi(sd["DeviceRowID"].c_str()));
				if (DMi != DeviceList->end())
				{
					m_SubDevices[atoi(sd["DeviceRowID"].c_str())] = DMi->second;
				}
			}
		}
		return (result.size() > 0);
	}
	CSwitchBinary::~CSwitchBinary()
	{
	}

	CSwitchDouble::CSwitchDouble()
	{
	}
	CSwitchDouble::~CSwitchDouble()
	{
	}

	CSwitchTriple::CSwitchTriple()
	{
	}
	CSwitchTriple::~CSwitchTriple()
	{
	}

	CSwitchVariable::CSwitchVariable()
	{
	}
	CSwitchVariable::~CSwitchVariable()
	{
	}

	CSensor::CSensor(int DomoticzID)
	{
		m_DomoticzID = DomoticzID;
		m_Name = "";
		m_DeviceCategory = DCAT_UNKNOWN;
		m_nValue = 0;
		m_sValue = "";
		m_lastUpdate = "";
		m_LastLevel = 0;
		m_Favourite = false;
		m_Protected = false;
		m_CustomImage = 0;
		m_IconURL = szWWWFolder + "/domoticz.png";
		m_IconDescription = "";
		m_IconOpacity = 100;
	}
	bool CSensor::Refresh()
	{
		bool bRetVal = false;
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT Name, nValue, sValue, LastUpdate, LastLevel, Favorite, Protected, BatteryLevel, CustomImage FROM DeviceStatus WHERE ID = %d", m_DomoticzID);
		if (result.size() > 0)
		{
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;

				// Notify the caller if the device has been changed
				bRetVal = (m_lastUpdate != sd["LastUpdate"]);

				m_Name = sd["Name"];
				m_nValue = atoi(sd["nValue"].c_str());
				m_sValue = sd["sValue"];
				m_lastUpdate = sd["LastUpdate"];
				m_LastLevel = atoi(sd["LastLevel"].c_str());
				m_Favourite = (sd["Favorite"] == "1");
				m_Protected = (sd["Protected"] == "1");
				m_BatteryLevel = atoi(sd["BatteryLevel"].c_str());
				if (m_CustomImage != atoi(sd["CustomImage"].c_str()))
				{
					m_IconURL = "";
				}
				m_CustomImage = atoi(sd["CustomImage"].c_str());
			}
		}
		return bRetVal;
	}
	bool CSensor::DeepRefresh(CDeviceMapPtr DeviceList)
	{
		bool bRetVal = Refresh();
		CSwitchBase* pSwitch = dynamic_cast<CSwitchBase*> (this);
		if (pSwitch)
		{
			pSwitch->LoadSubDevices(m_DomoticzID, DeviceList);
		}
		return bRetVal;
	}
	Json::Value CSensor::toJSON(int UserLevel)
	{
		Json::Value json;
		json["Index"] = m_DomoticzID;
		json["Title"] = m_Name;
		if (m_Protected)
		{
			json["TitleBarClass"] = "protected";
		}
//		if (hasTimeout)  // need to get Preference SensorTimeOut and compare to LastSeen 
//		{
//			json["Title Bar Class"] = "protected";
//		}
		if (m_BatteryLevel <= 10)
		{
			json["TitleBarClass"] = "lowbattery";
		}

		json["Icon"] = m_IconURL;
		json["IconTitle"] = m_IconDescription;
		json["IconOpacity"] = m_IconOpacity;

		json["FavouriteTitle"] = (m_Favourite ? "Remove from Dashboard " : "Add to Dashboard");
		json["FavouriteIcon"] = szWWWFolder + "/images/" + (m_Favourite ? "" : "no") + "favorite.png";
		json["FavouriteOnClick"] = "";
		if (UserLevel == 2) json["Favourite OnClick"] = "MakeFavorite(" + SSTR(m_DomoticzID) + "," + (m_Favourite ? "0" : "1") + ");";

//		json["WebcamIcon"] = szWWWFolder + "/images/webcam.png";
//		json["WebcamOnClick"] = szWWWFolder + "/images/webcam.png";

		json["LastUpdate"] = m_lastUpdate;

		return json;
	}
	CSensor::~CSensor()
	{
	}

	CSensorBinary::CSensorBinary(int DomoticzID) : CSensor(DomoticzID)
	{
	}
	CSensorBinary::~CSensorBinary()
	{
	}

	CSensorOnOff::CSensorOnOff(int DomoticzID) : CSensorBinary(DomoticzID)
	{
	}
	bool CSensorOnOff::Refresh()
	{
		if (CSensorBinary::Refresh())
		{
			// Icon URLs should be set be leaf classes with the execption of custom images for On/Off
			if ((m_CustomImage > 0) && (!m_IconURL.length()))
			{
				m_IconURL = szWWWFolder + "/domoticz.png";
				m_IconDescription = "";
				m_IconOpacity = 100;

				// File based custom icons
				if (m_CustomImage < 100)
				{
					std::ifstream infile;
					std::string switchlightsfile = szWWWFolder + "/switch_icons.txt";
					infile.open(switchlightsfile.c_str());
					if (infile.is_open())
					{
						int index = 0;
						std::string sLine = "";
						for (int index = 0; index++; !infile.eof() && (index++ < m_CustomImage))
						{
							getline(infile, sLine);
						}
						if (!infile.eof() && (sLine.size() != 0))
						{
							std::vector<std::string> results;
							StringSplit(sLine, ";", results);
							m_IconURL = szWWWFolder + "/images/" + results[0] + (m_nValue ? "48_On.png" : "48_Off.png");
							m_IconDescription = results[2];
						}
						infile.close();
					}
				}
				else
				{
					// Database based custom icons
					std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT Base, Description FROM CustomImages WHERE ID = %d", m_CustomImage - 100);
					if (result.size() > 0)
					{
						for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
						{
							std::map<std::string, std::string> sd = *itt;
							m_IconURL = szWWWFolder + "/images/" + sd["Base"] + (m_nValue ? "48_On.png" : "48_Off.png");
							m_IconDescription = sd["Description"];
						}
					}
				}
			}
			return true;
		}
		else return false;
	}
	Json::Value CSensorOnOff::toJSON(int UserLevel)
	{
		Json::Value json = CSensorBinary::toJSON(UserLevel);
		json["Status"] = (m_nValue ? "On " : "Off");
		json["ExtendedStatus"] = "";
		return json;
	}
	CSensorOnOff::~CSensorOnOff()
	{
	}

	CSensorOpenClose::CSensorOpenClose(int DomoticzID) : CSensorOnOff(DomoticzID)
	{
	}
	CSensorOpenClose::~CSensorOpenClose()
	{
	}

	CSensorVariable::CSensorVariable(int DomoticzID) : CSensorBinary(DomoticzID)
	{
	}
	CSensorVariable::~CSensorVariable()
	{
	}

	CSensorSlider::CSensorSlider(int DomoticzID) : CSensorVariable(DomoticzID)
	{
	}
	Json::Value CSensorSlider::toJSON(int UserLevel)
	{
		Json::Value json = CSensorVariable::toJSON(UserLevel);
		json["SliderLevel"] = m_nValue;
		json["MaxSliderLevel"] = 100;
		return json;
	}
	CSensorSlider::~CSensorSlider()
	{
	}

	/*
	**  Base classes complete.  Below here actual device definitions should be listed alphabetically
	*/
	CGroup::CGroup(int DomoticzID) : CScene(DomoticzID)
	{
	}
	CGroup::~CGroup()
	{
	}

	CScene::CScene(int DomoticzID) : CSensorOnOff(DomoticzID)
	{
		m_DeviceCategory = DCAT_SCENE;
	}
	bool CScene::Refresh()
	{
		bool bRetVal = false;
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT Name, nValue, LastUpdate, Favorite, Protected FROM Scenes WHERE ID = %d", m_DomoticzID);
		if (result.size() > 0)
		{
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;

				// Notify the caller if the device has been changed
				bRetVal = (m_lastUpdate != sd["LastUpdate"]);

				m_Name = sd["Name"];
				m_nValue = atoi(sd["nValue"].c_str());
				m_lastUpdate = sd["LastUpdate"];
				m_Favourite = (sd["Favorite"] == "1");
				m_Protected = (sd["Protected"] == "1");
			}
		}
		return bRetVal;
	}
	bool CScene::LoadSubDevices(DeviceID DomoticzID, CDeviceMapPtr DeviceList)
	{
		//
		//	Groups have 'sub devices' so associate this with them
		//
		m_SubDevices.clear();
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT DeviceRowID FROM SceneDevices WHERE SceneRowID = %d ORDER BY [Order]", DomoticzID);
		if (result.size() > 0)
		{
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;
				CDeviceMapIter	DMi = DeviceList->find(atoi(sd["DeviceRowID"].c_str()));
				if (DMi != DeviceList->end())
				{
					m_SubDevices[atoi(sd["DeviceRowID"].c_str())] = DMi->second;
				}
			}
		}
		return (result.size() > 0);
	}
	CScene::~CScene()
	{
	}

	CTemperature::CTemperature(int DomoticzID) : CSensorVariable(DomoticzID)
	{
		m_DeviceCategory = DCAT_TEMPERATURE;
	}
	CTemperature::~CTemperature()
	{
	}

}