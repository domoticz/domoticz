#include "stdafx.h"
#include "Logger.h"
#include "DeviceHelper.h"
#include "SQLHelper.h"
#include "RFXtrx.h"

namespace Devices
{
	CDeviceHelper::CDeviceHelper()
	{
		CDeviceMapPtr pDeviceMap(new CDeviceMap());
		m_devicelist = pDeviceMap;
	}
	CDeviceHelper::~CDeviceHelper()
	{
		Unload();
		m_devicelist.reset();
	}

	size_t CDeviceHelper::Load()
	{
		Unload();
		LoadDevices();
		LoadScenes();
		return m_devicelist->size();
	}
	void CDeviceHelper::Unload()
	{
		boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
		m_devicelist->clear();
	}

	CDeviceMapPtr CDeviceHelper::Find(_eDeviceCategoryMask Category)
	{
		CDeviceMapPtr pDeviceMap(new CDeviceMap());
		boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
		for (CDeviceMapIter itt = m_devicelist->begin(); itt != m_devicelist->end(); ++itt)
		{
			CSensorPtr pSensor = itt->second;
			if (pSensor->Category() & Category)
			{
				(*pDeviceMap)[itt->first] = pSensor;
			}
		}
		return pDeviceMap;
	}
	CDeviceMapPtr CDeviceHelper::Find(int Roomplan)
	{
		CDeviceMapPtr pDeviceMap(new CDeviceMap());
		boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT DeviceRowID, DevSceneType FROM DeviceToPlansMap WHERE ID = %d", Roomplan);
		if (result.size() > 0)
		{
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;
				int DeviceRowID = atoi(sd["DeviceRowID"].c_str());
				if (sd["DevSceneType"] == "0")
				{
					(*pDeviceMap)[DeviceRowID] = FindDevice(DeviceRowID);
				}
				else
				{
					(*pDeviceMap)[DeviceRowID*-1] = FindScene(DeviceRowID);
				}
			}
		}
		return pDeviceMap;
	}
	CDeviceMapPtr CDeviceHelper::Find(int Roomplan, _eDeviceCategoryMask Category)
	{
		CDeviceMapPtr pDeviceMap = Find(Roomplan);
		for (CDeviceMapIter itt = pDeviceMap->begin(); itt != pDeviceMap->end(); ++itt)
		{
			CSensorPtr pSensor = itt->second;
			if (!(pSensor->Category() & Category))
			{
				pDeviceMap->erase(itt->first);
			}
		}
		return pDeviceMap;
	}
	CDeviceMapPtr CDeviceHelper::Find(std::vector<DeviceID> Devices)
	{
		CDeviceMapPtr pDeviceMap(new CDeviceMap());
		boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
		for (std::vector<DeviceID>::iterator itt = Devices.begin(); itt != Devices.end(); ++itt)
		{
			if (*itt > 0)
			{
				(*pDeviceMap)[*itt] = FindDevice(*itt);
			}
			else
			{
				(*pDeviceMap)[*itt*-1] = FindScene(*itt);
			}
		}
		return pDeviceMap;
	}

	size_t CDeviceHelper::LoadDevices()
	{
		// Select used devices for enabled hardware only
		std::vector<std::map<std::string,std::string> > result = m_sql.safe_query_map("SELECT D.ID, D.SwitchType, D.Type, D.SubType FROM DeviceStatus D, Hardware H WHERE (D.Used = '1') and (D.HardwareID = H.ID) and (H.Enabled = 1)");
		if (result.size()>0)
		{
			boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;
				LoadDevice(atoi(sd["ID"].c_str()), (_eSwitchType)atoi(sd["SwitchType"].c_str()), atoi(sd["Type"].c_str()), atoi(sd["SubType"].c_str()));
			}
		}
		return result.size();
	}
	size_t CDeviceHelper::LoadScenes()
	{
		// Select used devices for enabled hardware only
		std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT ID, SceneType from Scenes");
		if (result.size()>0)
		{
			boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
			for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::map<std::string, std::string> sd = *itt;
				LoadScene(atoi(sd["ID"].c_str()), (_eSceneGroupType)atoi(sd["SceneType"].c_str()));
			}
		}
		return result.size();
	}

	void CDeviceHelper::LoadDevice(DeviceID DomoticzID, _eSwitchType sType, int dType, int subType)
	{
		// Macro to handle object creation for a given type
#define Case(X, Y) \
	case X: \
		pDevice = new Y(DomoticzID); \
		break;

		CSensor* pDevice = NULL;
		//
		//	Use SwitchType if specified
		//
		if (sType > STYPE_OnOff) {
			switch (sType)
			{
				Case(STYPE_Blinds, CSensorBinary);
				Case(STYPE_BlindsInverted, CSensorBinary);
				Case(STYPE_BlindsPercentage, CSensorBinary);
				Case(STYPE_BlindsPercentageInverted, CSensorBinary);
				Case(STYPE_Contact, CSensorBinary);
				Case(STYPE_Dimmer, CSensorBinary);
				Case(STYPE_Doorbell, CSensorBinary);
				Case(STYPE_DoorLock, CSensorBinary);
				Case(STYPE_Dusk, CSensorBinary);
				Case(STYPE_Media, CSensorBinary);
				Case(STYPE_Motion, CSensorBinary);
				Case(STYPE_PushOff, CSensorBinary);
				Case(STYPE_PushOn, CSensorBinary);
				Case(STYPE_Selector, CSensorBinary);
				Case(STYPE_SMOKEDETECTOR, CSensorBinary);
				Case(STYPE_VenetianBlindsEU, CSensorBinary);
				Case(STYPE_VenetianBlindsUS, CSensorBinary);
				Case(STYPE_X10Siren, CSensorBinary);
			default:
				pDevice = new CSensor(DomoticzID);
			}
		}
		else
		{
			switch (dType)
			{
				Case(pTypeCamera, CSensor);
				Case(pTypeRemote, CSensor);
				Case(pTypeLux, CSensor);
				Case(pTypeBBQ, CSensor);
				Case(pTypeHUM, CSensor);
				Case(pTypeBARO, CSensor);
				Case(pTypeTEMP, CSensor);
				Case(pTypeTEMP_RAIN, CSensor);
				Case(pTypeTEMP_BARO, CSensor);
				Case(pTypeTEMP_HUM, CSensor);
				Case(pTypeTEMP_HUM_BARO, CSensor);
				Case(pTypeRAIN, CSensor);
				Case(pTypeWIND, CSensor);
				Case(pTypeUV, CSensor);
				Case(pTypeUsage, CSensor);
				Case(pTypeAirQuality, CSensor);
				Case(pTypeP1Power, CSensor);
				Case(pTypeP1Gas, CSensor);
				Case(pTypeYouLess, CSensor);
				Case(pTypeDT, CSensor);
				Case(pTypeCURRENT, CSensor);
				Case(pTypeENERGY, CSensor);
				Case(pTypeCURRENTENERGY, CSensor);
				Case(pTypePOWER, CSensor);
				Case(pTypeWEIGHT, CSensor);
				Case(pTypeGAS, CSensor);
				Case(pTypeWATER, CSensor);
				Case(pTypeRFXSensor, CSensor);
				Case(pTypeRFXMeter, CSensor);
				Case(pTypeFS20, CSensor);
				Case(pTypeLighting1, CSensorBinary);
				Case(pTypeLighting2, CSensorBinary);
				Case(pTypeLighting3, CSensorBinary);
				Case(pTypeLighting4, CSensorBinary);
				Case(pTypeLighting5, CSensorBinary);
				Case(pTypeLighting6, CSensorBinary);
				Case(pTypeChime, CSensorBinary);
				Case(pTypeFan, CSensorBinary);
				Case(pTypeCurtain, CSensorBinary);
				Case(pTypeBlinds, CSensorBinary);
				Case(pTypeHomeConfort, CSensorBinary);
				Case(pTypeThermostat, CSensorBinary);
				Case(pTypeThermostat1, CSensorBinary);
				Case(pTypeThermostat2, CSensorBinary);
				Case(pTypeThermostat3, CSensorBinary);
				Case(pTypeRadiator1, CSensorBinary);
				Case(pTypeSecurity1, CSensorBinary);
				Case(pTypeSecurity2, CSensorBinary);
			case pTypeGeneral:
				switch (subType)
				{
					Case(sTypeVisibility, CSensor);
					Case(sTypeSolarRadiation, CSensor);
					Case(sTypeSoilMoisture, CSensor);
					Case(sTypeLeafWetness, CSensor);
					Case(sTypeSystemTemp, CSensor);
					Case(sTypePercentage, CSensor);
					Case(sTypeFan, CSensor);
					Case(sTypeVoltage, CSensor);
					Case(sTypePressure, CSensor);
					Case(sTypeSetPoint, CSensor);
					Case(sTypeTemperature, CSensor);
					Case(sTypeZWaveClock, CSensor);
					Case(sTypeTextStatus, CSensor);
					Case(sTypeZWaveThermostatMode, CSensor);
					Case(sTypeZWaveThermostatFanMode, CSensor);
					Case(sTypeAlert, CSensor);
					Case(sTypeCurrent, CSensor);
					Case(sTypeSoundLevel, CSensor);
					Case(sTypeBaro, CSensor);
					Case(sTypeDistance, CSensor);
					Case(sTypeCounterIncremental, CSensor);
					Case(sTypeKwh, CSensor);
					Case(sTypeWaterflow, CSensor);
				default:
					pDevice = new CSensor(DomoticzID);
				}
				break;
				Case(pTypeGeneralSwitch, CSensorBinary);
			default:
				pDevice = new CSensorBinary(DomoticzID);
			}
		}

		if (pDevice)
		{
			pDevice->DeepRefresh(m_devicelist);
			(*m_devicelist)[DomoticzID] = (CSensorPtr)pDevice;
		}
	}
	CSensorPtr CDeviceHelper::FindDevice(DeviceID DomoticzID)
	{
		return (*m_devicelist)[DomoticzID];
	}
	void CDeviceHelper::AddDevice(DeviceID DomoticzID)
	{
		if (FindDevice(DomoticzID))
		{
			UpdateDevice(DomoticzID);
		}
		else
		{
			std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT ID, SwitchType, Type, SubType FROM DeviceStatus WHERE ID = %d", DomoticzID);
			if (result.size()>0)
			{
				boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
				for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::map<std::string, std::string> sd = *itt;
					LoadDevice(atoi(sd["ID"].c_str()), (_eSwitchType)atoi(sd["SwitchType"].c_str()), atoi(sd["Type"].c_str()), atoi(sd["SubType"].c_str()));
				}
			}
		}
	}
	void CDeviceHelper::UpdateDevice(DeviceID DomoticzID)
	{
		CSensorPtr pDevice = FindDevice(DomoticzID);
		if (!pDevice)
		{
			AddDevice(DomoticzID);
		}
		else
		{
			pDevice->Refresh();
		}
	}
	void CDeviceHelper::DeepUpdateDevice(DeviceID DomoticzID)
	{
		CSensorPtr pDevice = FindDevice(DomoticzID);
		if (!pDevice)
		{
			AddDevice(DomoticzID);
		}
		else
		{
			pDevice->DeepRefresh(m_devicelist);
		}
	}
	void CDeviceHelper::DeleteDevice(DeviceID DomoticzID)
	{
		boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
		m_devicelist->erase(DomoticzID);
		// Get scenes & groups to deep refresh in case the deleted device was a 'SceneDevice' member
		for (CDeviceMapIter itt = m_devicelist->begin(); itt != m_devicelist->end(); ++itt)
		{
			boost::shared_ptr<CScene>	pScene = boost::dynamic_pointer_cast<CScene>((*itt).second);
			if (pScene)
			{
				pScene->DeepRefresh(m_devicelist);
			}
		}
	}

	void CDeviceHelper::LoadScene(DeviceID	DomoticzID, _eSceneGroupType sType)
	{
		CScene* pDevice = NULL;
		switch (sType)
		{
		case SGTYPE_SCENE:
			pDevice = new CScene(DomoticzID);
			break;
		case SGTYPE_GROUP:
			pDevice = new CGroup(DomoticzID);
			break;
		}

		if (pDevice)
		{
			pDevice->DeepRefresh(m_devicelist);
			// Device and scene ID ranges are the same so make scene IDs negative
			(*m_devicelist)[DomoticzID*-1] = (CSensorPtr)pDevice;
		}
	}
	CSensorPtr CDeviceHelper::FindScene(DeviceID DomoticzID)
	{
		return FindDevice(DomoticzID*-1);
	}
	void CDeviceHelper::AddScene(DeviceID DomoticzID)
	{
		if (FindScene(DomoticzID))
		{
			UpdateScene(DomoticzID);
		}
		else
		{
			std::vector<std::map<std::string, std::string> > result = m_sql.safe_query_map("SELECT ID, SceneType FROM Scenes WHERE ID = %d", DomoticzID);
			if (result.size() > 0)
			{
				boost::unique_lock<boost::shared_mutex> devicelistMutexLock(m_devicelistMutex);
				for (std::vector<std::map<std::string, std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::map<std::string, std::string> sd = *itt;
					LoadScene(atoi(sd["ID"].c_str()), (_eSceneGroupType)atoi(sd["SceneType"].c_str()));
				}
			}
		}
	}
	void CDeviceHelper::UpdateScene(DeviceID DomoticzID)
	{
		CSensorPtr pDevice = FindScene(DomoticzID);
		if (!pDevice)
		{
			AddScene(DomoticzID);
		}
		else
		{
			pDevice->Refresh();
		}
	}
	void CDeviceHelper::DeepUpdateScene(DeviceID DomoticzID)
	{
		CSensorPtr pDevice = FindScene(DomoticzID);
		if (!pDevice)
		{
			AddScene(DomoticzID);
		}
		else
		{
			pDevice->DeepRefresh(m_devicelist);
		}
	}
	void CDeviceHelper::DeleteScene(DeviceID DomoticzID)
	{
		DeleteDevice(DomoticzID*-1);
	}

}