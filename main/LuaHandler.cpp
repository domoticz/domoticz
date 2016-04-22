#include "stdafx.h"
#include "Helper.h"
#include "Logger.h"
#include "RFXtrx.h"
#include "../main/LuaHandler.h"

extern "C" {
#ifdef WITH_EXTERNAL_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#else
#include "../lua/src/lua.h"
#include "../lua/src/lualib.h"
#include "../lua/src/lauxlib.h"
#endif
}

#include "../tinyxpath/xpath_processor.h"
#include "../json/json.h"
#include "SQLHelper.h"
#include "mainworker.h"
#include "../hardware/hardwaretypes.h"

extern std::string szUserDataFolder;

int CLuaHandler::l_domoticz_applyXPath(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 2)
	{
		if (lua_isstring(lua_state, 1) && lua_isstring(lua_state, 2))
		{
			std::string buffer = lua_tostring(lua_state, 1);
			std::string xpath = lua_tostring(lua_state, 2);

			TiXmlDocument doc;
			doc.Parse(buffer.c_str(), 0, TIXML_ENCODING_UTF8);

			TiXmlElement* root = doc.RootElement();
			if (!root)
			{
				_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Invalid data received!");
				return 0;
			}
			TinyXPath::xpath_processor processor(root, xpath.c_str());
			TiXmlString xresult = processor.S_compute_xpath();
			lua_pushstring(lua_state, xresult.c_str());
			return 1;
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (applyXPath from LUA) : Not enough parameters");
	}
	return 0;
}

int CLuaHandler::l_domoticz_applyJsonPath(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 2)
	{
		if (lua_isstring(lua_state, 1) && lua_isstring(lua_state, 2))
		{
			std::string buffer = lua_tostring(lua_state, 1);
			std::string jsonpath = lua_tostring(lua_state, 2);

			Json::Value root;
			Json::Reader jReader;
			if (!jReader.parse(buffer, root))
			{
				_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid Json data received");
				return 0;
			}

			// Grab optional arguments
			Json::PathArgument arg1;
			Json::PathArgument arg2;
			Json::PathArgument arg3;
			Json::PathArgument arg4;
			Json::PathArgument arg5;
			if (nargs >= 3)
			{
				if (lua_isstring(lua_state, 3))
				{
					arg1 = Json::PathArgument(lua_tostring(lua_state, 3));
				}
				else
				{
					_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #1 for domoticz_applyJsonPath");
					return 0;
				}
				if (nargs >= 4)
				{
					if (lua_isstring(lua_state, 4))
					{
						arg2 = Json::PathArgument(lua_tostring(lua_state, 4));
					}
					else
					{
						_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #2 for domoticz_applyJsonPath");
						return 0;
					}
					if (nargs >= 5)
					{
						if (lua_isstring(lua_state, 5))
						{
							arg3 = Json::PathArgument(lua_tostring(lua_state, 5));
						}
						else
						{
							_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #3 for domoticz_applyJsonPath");
							return 0;
						}
						if (nargs >= 6)
						{
							if (lua_isstring(lua_state, 6))
							{
								arg2 = Json::PathArgument(lua_tostring(lua_state, 6));
							}
							else
							{
								_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Invalid extra argument #4 for domoticz_applyJsonPath");
								return 0;
							}
							if (nargs >= 7)
							{
								if (lua_isstring(lua_state, 7))
								{
									arg5 = Json::PathArgument(lua_tostring(lua_state, 7));
								}
								else
								{
									_log.Log(LOG_ERROR, "WebServer (applyJsonPath from LUA) : Invalid extra argument #5 for domoticz_applyJsonPath");
									return 0;
								}
							}
						}
					}
				}
			}

			// Apply the JsonPath to the Json
			Json::Path path(jsonpath, arg1, arg2, arg3, arg4, arg5);
			Json::Value& node = path.make(root);

			// Check if some data has been found
			if (!node.isNull())
			{
				if (node.isDouble())
				{
					lua_pushnumber(lua_state, node.asDouble());
					return 1;
				}
				if (node.isInt())
				{
					lua_pushnumber(lua_state, (double)node.asInt());
					return 1;
				}
				if (node.isInt64())
				{
					lua_pushnumber(lua_state, (double)node.asInt64());
					return 1;
				}
				if (node.isString())
				{
					lua_pushstring(lua_state, node.asCString());
					return 1;
				}
				lua_pushnil(lua_state);
				return 1;
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (applyJsonPath from LUA) : Incorrect parameters count");
	}
	return 0;
}

int CLuaHandler::l_domoticz_createVirtualDevice(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs == 4 || nargs == 5)
	{
		if (lua_isnumber(lua_state, 1) && lua_isnumber(lua_state, 2) && lua_isstring(lua_state, 3) && lua_isnumber(lua_state, 4))
		{
			int hwdID = (int)lua_tointeger(lua_state, 1);
			int idx = (int)lua_tointeger(lua_state, 2);
			std::string ssensorname = lua_tostring(lua_state, 3);
			int type = (int)lua_tointeger(lua_state, 4);
			std::string soptions;
			if (nargs == 5 && lua_isstring(lua_state, 5))
			{
				soptions = lua_tostring(lua_state, 5);
			}

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID FROM DeviceStatus WHERE (DeviceID==%d AND HardwareID=%d)", idx, hwdID);
			if (!result.empty())
			{
				// already exists, just quit
				return 0;
			}

			bool bCreated = false;

			//Make a unique number for ID
			result = m_sql.safe_query("SELECT MAX(ID) FROM DeviceStatus");

			unsigned long nid = idx;
			char ID[40];
			sprintf(ID, "%lu", nid);

			std::string devname;

			bool bPrevAcceptNewHardware = m_sql.m_bAcceptNewHardware;
			m_sql.m_bAcceptNewHardware = true;
			unsigned long long DeviceRowIdx = -1;
			switch (type)
			{
			case 1:
				//Pressure (Bar)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypePressure, 12, 255, 0, "0.0", devname);
				bCreated = true;
			}
			break;
			case 2:
				//Percentage
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypePercentage, 12, 255, 0, "0.0", devname);
				bCreated = true;
			}
			break;
			case 3:
				//Gas
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeP1Gas, sTypeP1Gas, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 4:
				//Voltage
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeVoltage, 12, 255, 0, "0.000", devname);
				bCreated = true;
			}
			break;
			case 5:
				//Text
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, "Hello World", devname);
				bCreated = true;
			}
			break;
			case 6:
				//Switch
			{
				sprintf(ID, "%08lX", nid);
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeGeneralSwitch, sSwitchGeneralSwitch, 12, 255, 0, "100", devname);
				bCreated = true;
			}
			break;
			case 7:
				//Alert
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeGeneral, sTypeAlert, 12, 255, 0, "No Alert!", devname);
				bCreated = true;
				break;
			case 8:
				//Thermostat Setpoint
			{
				unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
				unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
				unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
				unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
				sprintf(ID, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
			}
			DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeThermostat, sTypeThermSetpoint, 12, 255, 0, "20.5", devname);
			bCreated = true;
			break;
			case 9:
				//Current/Ampere
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeCURRENT, sTypeELEC1, 12, 255, 0, "0.0;0.0;0.0", devname);
				bCreated = true;
				break;
			case 10:
				//Sound Level
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeSoundLevel, 12, 255, 0, "65", devname);
				bCreated = true;
			}
			break;
			case 11:
				//Barometer (hPa)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeBaro, 12, 255, 0, "1021.34;0", devname);
				bCreated = true;
			}
			break;
			case 12:
				//Visibility (km)
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeGeneral, sTypeVisibility, 12, 255, 0, "10.3", devname);
				bCreated = true;
				break;
			case 13:
				//Distance (cm)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeDistance, 12, 255, 0, "123.4", devname);
				bCreated = true;
			}
			break;
			case 14: //Counter Incremental
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeGeneral, sTypeCounterIncremental, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case 15:
				//Soil Moisture
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeSoilMoisture, 12, 255, 3, devname);
				bCreated = true;
			}
			break;
			case 16:
				//Leaf Wetness
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeLeafWetness, 12, 255, 2, devname);
				bCreated = true;
			}
			break;
			case 17:
				//Thermostat Clock
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeZWaveClock, 12, 255, 0, "24:12:00", devname);
				bCreated = true;
			}
			break;
			case 18:
				//kWh
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeKwh, 12, 255, 0, "0;0.0", devname);
				bCreated = true;
			}
			break;
			case 19:
				//Current (Single)
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeCurrent, 12, 255, 0, "6.4", devname);
				bCreated = true;
			}
			break;
			case 20:
				//Solar Radiation
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeSolarRadiation, 12, 255, 0, "1.0", devname);
				bCreated = true;
			}
			break;
			case pTypeLimitlessLights:
				//RGB switch
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeLimitlessLights, sTypeLimitlessRGB, 12, 255, 1, devname);
				if (DeviceRowIdx != -1)
				{
					//Set switch type to dimmer
					m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%llu)", STYPE_Dimmer, DeviceRowIdx);
				}
				bCreated = true;
			}
			break;
			case pTypeTEMP:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeTEMP, sTypeTEMP1, 12, 255, 0, "0.0", devname);
				bCreated = true;
				break;
			case pTypeHUM:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeHUM, sTypeTEMP1, 12, 255, 50, "1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeTEMP_HUM, sTypeTH1, 12, 255, 0, "0.0;50;1", devname);
				bCreated = true;
				break;
			case pTypeTEMP_HUM_BARO:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeTEMP_HUM_BARO, sTypeTHB1, 12, 255, 0, "0.0;50;1;1010;1", devname);
				bCreated = true;
				break;
			case pTypeWIND:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeWIND, sTypeWIND1, 12, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case pTypeRAIN:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeRAIN, sTypeRAIN3, 12, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeUV:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeUV, sTypeUV1, 12, 255, 0, "0;0", devname);
				bCreated = true;
				break;
			case pTypeRFXMeter:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeRFXMeter, sTypeRFXMeterCount, 10, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeAirQuality:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeAirQuality, sTypeVoltcraft, 12, 255, 0, devname);
				bCreated = true;
				break;
			case pTypeUsage:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeUsage, sTypeElectric, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeLux:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeLux, sTypeLux, 12, 255, 0, "0", devname);
				bCreated = true;
				break;
			case pTypeP1Power:
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeP1Power, sTypeP1Power, 12, 255, 0, "0;0;0;0;0;0", devname);
				bCreated = true;
				break;
			case 1000:
				//Waterflow
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeWaterflow, 12, 255, 0, "0.0", devname);
				bCreated = true;
			}
			break;
			case 1001:
				//Wind + Temp + Chill
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeWIND, sTypeWIND4, 12, 255, 0, "0;N;0;0;0;0", devname);
				bCreated = true;
				break;
			case 1002:
				//Selector Switch
			{
				unsigned char ID1 = (unsigned char)((nid & 0xFF000000) >> 24);
				unsigned char ID2 = (unsigned char)((nid & 0x00FF0000) >> 16);
				unsigned char ID3 = (unsigned char)((nid & 0x0000FF00) >> 8);
				unsigned char ID4 = (unsigned char)((nid & 0x000000FF));
				sprintf(ID, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
				DeviceRowIdx = m_sql.UpdateValue(hwdID, ID, 1, pTypeGeneralSwitch, sSwitchTypeSelector, 12, 255, 0, "0", devname);
				if (DeviceRowIdx != -1)
				{
					//Set switch type to selector
					m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%llu)", STYPE_Selector, DeviceRowIdx);
					//Set default device options
					m_sql.SetDeviceOptions(DeviceRowIdx, m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Level1|Level2|Level3", false));
				}
				bCreated = true;
			}
			break;
			case 1003:
				//RGBW switch
			{
				std::string rID = std::string(ID);
				padLeft(rID, 8, '0');
				DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeLimitlessLights, sTypeLimitlessRGBW, 12, 255, 1, devname);
				if (DeviceRowIdx != -1)
				{
					//Set switch type to dimmer
					m_sql.safe_query("UPDATE DeviceStatus SET SwitchType=%d WHERE (ID==%llu)", STYPE_Dimmer, DeviceRowIdx);
				}
				bCreated = true;
			}
			break;
			case 1004:
				//Custom
				if (!soptions.empty())
				{
					std::string rID = std::string(ID);
					padLeft(rID, 8, '0');
					DeviceRowIdx = m_sql.UpdateValue(hwdID, rID.c_str(), 1, pTypeGeneral, sTypeCustom, 12, 255, 0, "0.0", devname);
					if (DeviceRowIdx != -1)
					{
						//Set the Label
						m_sql.safe_query("UPDATE DeviceStatus SET Options='%q' WHERE (ID==%llu)", soptions.c_str(), DeviceRowIdx);
					}
					bCreated = true;
				}
				break;
			}

			m_sql.m_bAcceptNewHardware = bPrevAcceptNewHardware;

			if (DeviceRowIdx != -1)
			{
				m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', Used=1 WHERE (ID==%llu)", ssensorname.c_str(), DeviceRowIdx);
			}
		}
	}
	return 0;
}

int CLuaHandler::l_domoticz_updateDevice(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);
	if (nargs >= 3 && nargs <= 5)
	{
		// Supported format ares :
		// - deviceId (integer), svalue (string), nvalue (string), [rssi(integer)], [battery(integer)]
		// - deviceId (integer), svalue (string), nvalue (integer), [rssi(integer)], [battery(integer)]
		if (lua_isnumber(lua_state, 1) && (lua_isstring(lua_state, 2) || lua_isnumber(lua_state, 2)) && lua_isstring(lua_state, 3))
		{
			// Extract the parameters from the lua 'updateDevice' function
			int ideviceId = (int)lua_tointeger(lua_state, 1);
			std::string nvalue = lua_tostring(lua_state, 2);
			std::string svalue = lua_tostring(lua_state, 3);
			if (((lua_isstring(lua_state, 3) && nvalue.empty()) && svalue.empty()))
			{
				_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : nvalue and svalue are empty ");
				return 0;
			}

			// Parse
			int invalue = (!nvalue.empty()) ? atoi(nvalue.c_str()) : 0;
			int signallevel = 12;
			if (nargs >= 4 && lua_isnumber(lua_state, 4))
			{
				signallevel = (int)lua_tointeger(lua_state, 4);
			}
			int batterylevel = 255;
			if (nargs == 5 && lua_isnumber(lua_state, 5))
			{
				batterylevel = (int)lua_tointeger(lua_state, 5);
			}
			_log.Log(LOG_NORM, "CLuaHandler (updateDevice from LUA) : idx=%d nvalue=%s svalue=%s invalue=%d signallevel=%d batterylevel=%d", ideviceId, nvalue.c_str(), svalue.c_str(), invalue, signallevel, batterylevel);

			// Get the raw device parameters
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (DeviceID==%d)", ideviceId);
			if (result.empty())
				return 0;
			std::string hid = result[0][0];
			std::string did = result[0][1];
			std::string dunit = result[0][2];
			std::string dtype = result[0][3];
			std::string dsubtype = result[0][4];

			int HardwareID = atoi(hid.c_str());
			std::string DeviceID = did;
			int unit = atoi(dunit.c_str());
			int devType = atoi(dtype.c_str());
			int subType = atoi(dsubtype.c_str());

			std::stringstream sstr;
			unsigned long long ulIdx;
			sstr << ideviceId;
			sstr >> ulIdx;
			m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, invalue, svalue, signallevel, batterylevel);
		}
		else
		{
			_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : Incorrect parameters type");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "CLuaHandler (updateDevice from LUA) : Not enough parameters");
	}
	return 0;
}

int CLuaHandler::l_domoticz_print(lua_State* lua_state)
{
	int nargs = lua_gettop(lua_state);

	for (int i = 1; i <= nargs; i++)
	{
		if (lua_isstring(lua_state, i))
		{
			//std::string lstring=lua_tostring(lua_state, i);
			_log.Log(LOG_NORM, "CLuaHandler: udevices: %s", lua_tostring(lua_state, i));
		}
		else
		{
			/* non strings? */
		}
	}
	return 0;
}

CLuaHandler::CLuaHandler(int hwdID)
{
	m_HwdID = hwdID;
}

void CLuaHandler::luaThread(lua_State *lua_state, const std::string &filename)
{
	int status;

	status = lua_pcall(lua_state, 0, LUA_MULTRET, 0);
	report_errors(lua_state, status);
	lua_close(lua_state);
}

void CLuaHandler::luaStop(lua_State *L, lua_Debug *ar)
{
	if (ar->event == LUA_HOOKCOUNT)
	{
		(void)ar;  /* unused arg. */
		lua_sethook(L, NULL, 0, 0);
		luaL_error(L, "LuaHandler: Lua script execution exceeds maximum number of lines");
		lua_close(L);
	}
}

void CLuaHandler::report_errors(lua_State *L, int status)
{
	if (status != 0) {
		_log.Log(LOG_ERROR, "CLuaHandler: %s", lua_tostring(L, -1));
		lua_pop(L, 1); // remove error message
	}
}

bool CLuaHandler::executeLuaScript(const std::string &script, const std::string &content)
{
	std::vector<std::string> allParameters;
	return executeLuaScript(script, content, allParameters);
}

bool CLuaHandler::executeLuaScript(const std::string &script, const std::string &content, std::vector<std::string>& allParameters)
{
	std::stringstream lua_DirT;
#ifdef WIN32
	lua_DirT << szUserDataFolder << "scripts\\lua_parsers\\";
#else
	lua_DirT << szUserDataFolder << "scripts/lua_parsers/";
#endif
	std::string lua_Dir = lua_DirT.str();

	lua_State *lua_state;
	lua_state = luaL_newstate();

	luaL_openlibs(lua_state);
	lua_pushcfunction(lua_state, l_domoticz_print);
	lua_setglobal(lua_state, "print");

	lua_pushcfunction(lua_state, l_domoticz_updateDevice);
	lua_setglobal(lua_state, "domoticz_updateDevice");

	lua_pushcfunction(lua_state, l_domoticz_createVirtualDevice);
	lua_setglobal(lua_state, "domoticz_createVirtualDevice");

	lua_pushcfunction(lua_state, l_domoticz_applyJsonPath);
	lua_setglobal(lua_state, "domoticz_applyJsonPath");

	lua_pushcfunction(lua_state, l_domoticz_applyXPath);
	lua_setglobal(lua_state, "domoticz_applyXPath");

	lua_pushinteger(lua_state, m_HwdID);
	lua_setglobal(lua_state, "hwdId");

	lua_createtable(lua_state, 1, 0);
	lua_pushstring(lua_state, "content");
	lua_pushstring(lua_state, content.c_str());
	lua_rawset(lua_state, -3);
	lua_setglobal(lua_state, "request");

	// Push all url parameters as a map indexed by the parameter name
	// Each entry will be uri[<param name>] = <param value>
	int totParameters = (int)allParameters.size();
	lua_createtable(lua_state, totParameters, 0);
	for (int i = 0; i < totParameters; i++)
	{
		std::vector<std::string> parameterCouple;
		StringSplit(allParameters[i], "=", parameterCouple);
		if (parameterCouple.size() == 2) {
			// Add an url parameter after 'url' decoding it
			lua_pushstring(lua_state, CURLEncode::URLDecode(parameterCouple[0]).c_str());
			lua_pushstring(lua_state, CURLEncode::URLDecode(parameterCouple[1]).c_str());
			lua_rawset(lua_state, -3);
		}
	}
	lua_setglobal(lua_state, "uri");

	std::string fullfilename = lua_Dir + script;
	int status = luaL_loadfile(lua_state, fullfilename.c_str());
	if (status == 0)
	{
		lua_sethook(lua_state, luaStop, LUA_MASKCOUNT, 10000000);
		boost::thread aluaThread(boost::bind(&CLuaHandler::luaThread, this, lua_state, fullfilename));
		aluaThread.timed_join(boost::posix_time::seconds(10));
		return true;
	}
	else
	{
		report_errors(lua_state, status);
		lua_close(lua_state);
	}
	return false;
}
