#pragma once

enum _eSecurityStatus
{
	SECSTATUS_DISARMED = 0,
	SECSTATUS_ARMEDHOME,
	SECSTATUS_ARMEDAWAY,
};

enum _eSceneGroupType
{
	SGTYPE_SCENE = 0,
	SGTYPE_GROUP
};

//Keep this consistant with the SQL DeviceStatus table
struct _tDeviceSensor
{
	unsigned long long	Idx;	//Idx from database

	int				HardwareID;
	_eHardwareTypes	HdwType;

	std::string		Name;
	std::string		Description;

	std::string		ID;
	int				Unit;
	int				DevType;
	int				SubType;

	int				nValue;
	std::string		sValue;

	int				Used;
	_eSwitchType	SwitchType;
	int				Favorite;
	int				SignalLevel;
	int				BatteryLevel;

	int				Order;
	float			AddjValue;
	float			AddjValue2;
	float			AddjMulti;
	float			AddjMulti2;

	int				LastLevel;
	int				CustomImage;

	std::string		StrParam1;
	std::string		StrParam2;

	int				Protected;
	time_t			LastUpdate;
};
