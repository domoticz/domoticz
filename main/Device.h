#pragma once
#include <boost/shared_ptr.hpp>

#include "RFXNames.h"
#include "mainstructs.h"
#include "../json/json.h"

namespace Devices
{
	class CSensor;
	typedef long long DeviceID;
	typedef boost::shared_ptr<CSensor> CSensorPtr;
	typedef std::map<DeviceID, CSensorPtr> CDeviceMap;
	typedef boost::shared_ptr< CDeviceMap > CDeviceMapPtr;
	typedef CDeviceMap::iterator CDeviceMapIter;

	/*
	**  Switch classes to support click functionality
	*/
	class CSwitchBase
	{
	public:
		CSwitchBase();
		virtual bool	LoadSubDevices(DeviceID DomoticzID, CDeviceMapPtr DeviceList) = 0;
		~CSwitchBase();
	protected:
	};

	class CSwitchBinary : public virtual CSwitchBase
	{
	public:
		CSwitchBinary();
		virtual bool	LoadSubDevices(DeviceID DomoticzID, CDeviceMapPtr DeviceList);
		~CSwitchBinary();
	protected:
		CDeviceMap				m_SubDevices;
	};

	class CSwitchDouble : public virtual CSwitchBinary
	{
	public:
		CSwitchDouble();
		~CSwitchDouble();
	};

	class CSwitchTriple : public virtual CSwitchDouble
	{
	public:
		CSwitchTriple();
		~CSwitchTriple();
	};

	class CSwitchVariable : public virtual CSwitchBinary
	{
	public:
		CSwitchVariable();
		~CSwitchVariable();
	};

	// Category that the devices is in, need to be a mask because there is a Wind device that is in more than one
	enum _eDeviceCategoryMask
	{
		DCAT_UNKNOWN = 0,
		DCAT_SWITCH = 1,
		DCAT_SCENE = 2,
		DCAT_TEMPERATURE = 4,
		DCAT_UTILITY = 8,
		DCAT_WEATHER = 16
	};

	class CSensorBase
	{
	public:
		CSensorBase() {};
		~CSensorBase() {};
	};

	/*
	**	CSensor is the true base class.
	*/
	class CSensor : public CSensorBase
	{
	public:
		CSensor(int DomoticzID);
		virtual bool			Refresh();
		virtual bool			DeepRefresh(CDeviceMapPtr DeviceList);
		virtual Json::Value		toJSON(int UserLevel);
		_eDeviceCategoryMask	Category() { return m_DeviceCategory; };
		~CSensor();
	protected:
		long					m_DomoticzID;
		std::string				m_Name;

		_eDeviceCategoryMask	m_DeviceCategory;

		DeviceID		m_nValue;
		std::string				m_sValue;
		std::string				m_lastUpdate;
		int						m_LastLevel;

		int						m_CustomImage;
		std::string				m_IconURL;
		std::string				m_IconDescription;
		int						m_IconOpacity;

		bool					m_Favourite;
		bool					m_Protected;
		int						m_BatteryLevel;
	};

	class CSensorBinary : public CSensor
	{
	public:
		CSensorBinary(int DomoticzID);
		~CSensorBinary();
	};

	class CSensorVariable : public CSensorBinary
	{
	public:
		CSensorVariable(int DomoticzID);
		~CSensorVariable();
	};

	class CSensorSlider : public CSensorVariable
	{
	public:
		CSensorSlider(int DomoticzID);
		virtual Json::Value		toJSON(int UserLevel);
		~CSensorSlider();
	};

	class CSensorOnOff : public CSensorBinary
	{
	public:
		CSensorOnOff(int DomoticzID);
		virtual bool			Refresh();
		virtual Json::Value		toJSON(int UserLevel);
		~CSensorOnOff();
	};

	class CSensorOpenClose : public CSensorOnOff
	{
	public:
		CSensorOpenClose(int DomoticzID);
		~CSensorOpenClose();
	};

	class CScene : public CSensorOnOff, public virtual CSwitchBinary
	{
	public:
		CScene(int DomoticzID);
		virtual bool	Refresh();
		virtual bool	LoadSubDevices(DeviceID DomoticzID, CDeviceMapPtr DeviceList);
		~CScene();
	};

#ifdef WIN32    
	// Suppress Visual Studio warning telling us the it will implement inheritance properly.
	// Group::LoadSubDevices will come from CScene and not CSwitchBinary (via CSwitchDouble) which is what we want
#pragma warning(push)
#pragma warning(disable : 4250)
#endif // WIN32
	class CGroup : public CScene, public virtual CSwitchDouble
	{
	public:
		CGroup(int DomoticzID);
		~CGroup();
	private:
	};
#ifdef WIN32
#pragma warning(pop)
#endif // WIN32

	class CTemperature : public CSensorVariable
	{
	public:
		CTemperature(int DomoticzID);
		~CTemperature();
	};
}