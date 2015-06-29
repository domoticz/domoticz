#pragma once

#include "DomoticzHardware.h"

class MySensorsBase : public CDomoticzHardwareBase
{
	friend class MySensorsSerial;
	friend class MySensorsTCP;
	friend class MQTT;
public:
	enum _eMessageType
	{
		MT_Presentation = 0,	// Sent by a node when they present attached sensors.This is usually done in setup() at startup.
		MT_Set = 1,			// This message is sent from or to a sensor when a sensor value should be updated
		MT_Req = 2,			// Requests a variable value(usually from an actuator destined for controller).
		MT_Internal = 3,		// This is a special internal message.See table "_eInternalType" below for the details
		MT_Stream = 4			// Used for OTA firmware updates
	};

	enum _ePresentationType
	{
		S_DOOR=0,				// Door and window sensors
		S_MOTION=1,				// Motion sensors
		S_SMOKE=2,				// Smoke sensor
		S_LIGHT=3,				// Light Actuator(on / off)
		S_DIMMER=4,				// Dimmable device of some kind
		S_COVER=5,				// Window covers or shades
		S_TEMP=6,				// Temperature sensor
		S_HUM=7,				// Humidity sensor
		S_BARO=8,				// Barometer sensor(Pressure)
		S_WIND=9,				// Wind sensor
		S_RAIN=10,				// Rain sensor
		S_UV=11,				// UV sensor
		S_WEIGHT=12,			// Weight sensor for scales etc.
		S_POWER=13,				// Power measuring device, like power meters
		S_HEATER=14,			// Heater device
		S_DISTANCE=15,			// Distance sensor
		S_LIGHT_LEVEL=16,		// Light sensor
		S_ARDUINO_NODE=17,		// Arduino node device
		S_ARDUINO_RELAY=18,		// Arduino repeating node device
		S_LOCK=19,				// Lock device
		S_IR=20,				// Ir sender / receiver device
		S_WATER=21,				// Water meter
		S_AIR_QUALITY=22,		// Air quality sensor e.g.MQ - 2
		S_CUSTOM=23,			// Use this for custom sensors where no other fits.
		S_DUST=24,				// Dust level sensor
		S_SCENE_CONTROLLER=25,	// Scene controller device
		S_RGB_LIGHT=26,			// Send data using V_RGB or V_RGBW 
		S_COLOR_SENSOR=27,		// Send data using V_RGB
		S_MULTIMETER=28,		// V_VOLTAGE, V_CURRENT, V_IMPEDANCE 
		S_SPRINKLER=29,			// V_TRIPPED, V_ARMED
		S_WATER_LEAK=30,		// V_TRIPPED, V_ARMED
		S_SOUND=31,				// V_TRIPPED, V_ARMED, V_SOUND_DB
		S_VIBRATION=32,			// V_TRIPPED, V_ARMED, V_VIBRATION_HZ 
		S_ROTARY_ENCODER=33,	// V_ENCODER_VALUE
	};

	enum _eSetType
	{
		V_TEMP = 0,			//	Temperature
		V_HUM = 1,			//	Humidity
		V_LIGHT = 2,			//	Light status. 0 = off 1 = on
		V_DIMMER = 3,			//	Dimmer value. 0 - 100 %
		V_PRESSURE = 4,		//	Atmospheric Pressure
		V_FORECAST = 5,		//	Whether forecast.One of "stable", "sunny", "cloudy", "unstable", "thunderstorm" or "unknown"
		V_RAIN = 6,			//	Amount of rain
		V_RAINRATE = 7,		//	Rate of rain
		V_WIND = 8,			//	Windspeed
		V_GUST = 9,			//	Gust
		V_DIRECTION = 10,		//	Wind direction
		V_UV = 11,			//	UV light level
		V_WEIGHT = 12,		//	Weight(for scales etc)
		V_DISTANCE = 13,		//	Distance
		V_IMPEDANCE = 14,		//	Impedance value
		V_ARMED = 15,			//	Armed status of a security sensor. 1 = Armed, 0 = Bypassed
		V_TRIPPED = 16,		//	Tripped status of a security sensor. 1 = Tripped, 0 = Untripped
		V_WATT = 17,			//	Watt value for power meters
		V_KWH = 18,			//	Accumulated number of KWH for a power meter
		V_SCENE_ON = 19,		//	Turn on a scene
		V_SCENE_OFF = 20,		//	Turn of a scene
		V_HEATER = 21,		//	Mode of header.One of "Off", "HeatOn", "CoolOn", or "AutoChangeOver"
		V_HEATER_SW = 22,		//	Heater switch power. 1 = On, 0 = Off
		V_LIGHT_LEVEL = 23,	//	Light level. 0 - 100 %
		V_VAR1 = 24,			//	Custom value
		V_VAR2 = 25,			//	Custom value
		V_VAR3 = 26,			//	Custom value
		V_VAR4 = 27,			//	Custom value
		V_VAR5 = 28,			//	Custom value
		V_UP = 29,			//	Window covering.Up.
		V_DOWN = 30,			//	Window covering.Down.
		V_STOP = 31,			//	Window covering.Stop.
		V_IR_SEND = 32,		//	Send out an IR - command
		V_IR_RECEIVE = 33,	//	This message contains a received IR - command
		V_FLOW = 34,			//	Flow of water(in meter)
		V_VOLUME = 35,		//	Water volume
		V_LOCK_STATUS = 36,	//	Set or get lock status. 1 = Locked, 0 = Unlocked
		V_DUST_LEVEL = 37,	//	Dust level
		V_VOLTAGE = 38,		//	Voltage level
		V_CURRENT = 39,		//	Current level
		V_RGB = 40, 		// S_RGB_LIGHT, S_COLOR_SENSOR.  (RRGGBB)
		V_RGBW = 41,		// S_RGB_LIGHT (RRGGBBWW)
		V_ID = 42,			// Used for reporting the sensor internal ids (E.g. DS1820b). 
		V_LIGHT_LEVEL_LUX = 43,  // S_LIGHT, Light level in lux
		V_UNIT_PREFIX = 44, // Allows sensors to send in a string representing the 
		V_SOUND_DB = 45,	// S_SOUND sound level in db
		V_VIBRATION_HZ = 46, // S_VIBRATION vibration i Hz
		V_ENCODER_VALUE = 47, // S_ROTARY_ENCODER. Rotary encoder value. 
	};

	enum _eInternalType
	{
		I_BATTERY_LEVEL = 0,			// Use this to report the battery level(in percent 0 - 100).
		I_TIME = 1,					// Sensors can request the current time from the Controller using this message.The time will be reported as the seconds since 1970
		I_VERSION = 2,				// Sensors report their library version at startup using this message type
		I_ID_REQUEST = 3,				// Use this to request a unique node id from the controller.
		I_ID_RESPONSE = 4,			// Id response back to sensor.Payload contains sensor id.
		I_INCLUSION_MODE = 5,			// Start / stop inclusion mode of the Controller(1 = start, 0 = stop).
		I_CONFIG = 6,					// Config request from node.Reply with(M)etric or(I)mperal back to sensor.
		I_FIND_PARENT = 7,			// When a sensor starts up, it broadcast a search request to all neighbor nodes.They reply with a I_FIND_PARENT_RESPONSE.
		I_FIND_PARENT_RESPONSE = 8,	// Reply message type to I_FIND_PARENT request.
		I_LOG_MESSAGE = 9,			// Sent by the gateway to the Controller to trace - log a message
		I_CHILDREN = 10,				// A message that can be used to transfer child sensors(from EEPROM routing table) of a repeating node.
		I_SKETCH_NAME = 11,			// Optional sketch name that can be used to identify sensor in the Controller GUI
		I_SKETCH_VERSION = 12,		// Optional sketch version that can be reported to keep track of the version of sensor in the Controller GUI.
		I_REBOOT = 13,				// Used by OTA firmware updates.Request for node to reboot.
		I_GATEWAY_READY = 14,			// Send by gateway to controller when startup is complete.
	};

	struct _tMySensorsReverseTypeLookup
	{
		int SType;
		const char *stringType;
	};

	struct _tMySensorSensor
	{
		int nodeID;
		int childID;
		_eSetType devType;

		//values
		float floatValue;
		int intvalue;
		bool bValidValue;
		std::string stringValue;

		//battery
		bool hasBattery;
		int batValue;

		time_t lastreceived;

		_tMySensorSensor()
		{
			nodeID = -1;
			childID = 1;
			hasBattery = false;
			batValue = 255;
			floatValue = 0;
			intvalue = 0;
			bValidValue = true;
			devType = V_TEMP;
		}
	};

	struct _tMySensorNode
	{
		int nodeID;
		std::string SketchName;
		std::string SketchVersion;
		time_t lastreceived;
		std::vector<_tMySensorSensor> m_sensors;
		std::vector<_ePresentationType> m_types;
		_tMySensorNode()
		{
			lastreceived = 0;
			nodeID = -1;
		}
		void AddType(const _ePresentationType ptype)
		{
			std::vector<_ePresentationType>::const_iterator itt;
			for (itt = m_types.begin(); itt != m_types.end(); ++itt)
			{
				if (*itt == ptype)
					return;
			}
			m_types.push_back(ptype);
		}
		bool FindType(const _ePresentationType ptype)
		{
			std::vector<_ePresentationType>::const_iterator itt;
			for (itt = m_types.begin(); itt != m_types.end(); ++itt)
			{
				if (*itt == ptype)
					return true;
			}
			return false;
		}
	} MySensorNode;

	MySensorsBase(void);
	~MySensorsBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	virtual void WriteInt(const std::string &sendStr) = 0;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	bool GetReverseValueLookup(const std::string &ValueString, _eSetType &retSetType);
	bool GetReversePresentationLookup(const std::string &ValueString, _ePresentationType &retSetType);
	bool GetReverseTypeLookup(const std::string &ValueString, _eMessageType &retSetType);

	void SendCommand(const int NodeID, const int ChildID, const _eMessageType messageType, const int SubType, const std::string &Payload);
	void UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);

	bool GetSwitchValue(const unsigned char Idx, const int SubUnit, const int sub_type, std::string &sSwitchValue);

	bool GetBlindsValue(const int NodeID, const int ChildID, int &blind_value);

	void LoadDevicesFromDatabase();
	void Add2Database(const int nodeID, const std::string &SketchName, const std::string &SketchVersion);
	void DatabaseUpdateSketchName(const int nodeID, const std::string &SketchName);
	void DatabaseUpdateSketchVersion(const int nodeID, const std::string &SketchVersion);

	void SendSensor2Domoticz(const _tMySensorNode *pNode, const _tMySensorSensor *pSensor);

	void MakeAndSendWindSensor(const int nodeID);

	_tMySensorNode* FindNode(const int nodeID);
	_tMySensorNode* InsertNode(const int nodeID);
	void RemoveNode(const int nodeID);
	int FindNextNodeID();
	_tMySensorSensor* FindSensor(_tMySensorNode *pNode, const int childID, _eSetType devType);
	_tMySensorSensor* FindSensor(const int nodeID, _eSetType devType);
	void InsertSensor(_tMySensorSensor device);
	void UpdateNodeBatteryLevel(const int nodeID, const int Level);

	void UpdateVar(const int NodeID, const int ChildID, const int VarID, const std::string &svalue);
	bool GetVar(const int NodeID, const int ChildID, const int VarID, std::string &sValue);

	std::map<int, _tMySensorNode> m_nodes;

	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;

	static const _tMySensorsReverseTypeLookup m_MySenserReverseValueTable[];
	static const _tMySensorsReverseTypeLookup m_MySenserReversePresentationTable[];
	static const _tMySensorsReverseTypeLookup m_MySenserReverseTypeTable[];
};

