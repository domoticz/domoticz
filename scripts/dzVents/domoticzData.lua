-- Persistent Data
local multiRefObjects = {

} -- multiRefObjects
local obj1 = {
	[1] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 61;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "00014051";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "lightbulb";
			["maxDimLevel"] = "100";
			["_state"] = "On";
		};
		["name"] = "s1";
		["rawData"] = {
			[1] = "0";
		};
		["id"] = 1;
		["lastUpdate"] = "2017-05-12 11:14:42";
		["deviceType"] = "Light/Switch";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "Switch";
	};
	[2] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "82002";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "temperature";
			["temp"] = "0.0";
			["_state"] = "0.0";
		};
		["name"] = "myTemp";
		["rawData"] = {
			[1] = "0.0";
		};
		["id"] = 2;
		["lastUpdate"] = "2017-04-18 15:35:31";
		["deviceType"] = "Temp";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "LaCrosse TX3";
	};
	[3] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "82003";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "temperature";
			["humidityStatus"] = "Comfortable";
			["_state"] = "0.0;50;1";
			["humidity"] = "50";
			["temp"] = "0.0";
			["dewPoint"] = "-9.20";
		};
		["name"] = "myTempHum";
		["rawData"] = {
			[1] = "0.0";
			[2] = "50";
			[3] = "1";
		};
		["id"] = 3;
		["lastUpdate"] = "2017-04-18 15:35:41";
		["deviceType"] = "Temp + Humidity";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "THGN122/123, THGN132, THGR122/228/238/268";
	};
	[4] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "82004";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "temperature";
			["temp"] = "0.0";
			["humidityStatus"] = "Comfortable";
			["forecast"] = "1";
			["dewPoint"] = "-9.20";
			["_state"] = "0.0;50;1;1010;1";
			["forecastStr"] = "Sunny";
			["humidity"] = "50";
			["barometer"] = "1010";
		};
		["name"] = "myTempHumBaro";
		["rawData"] = {
			[1] = "0.0";
			[2] = "50";
			[3] = "1";
			[4] = "1010";
			[5] = "1";
		};
		["id"] = 4;
		["lastUpdate"] = "2017-04-18 15:35:51";
		["deviceType"] = "Temp + Humidity + Baro";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "THB1 - BTHR918, BTHGN129";
	};
	[5] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "00082005";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "text";
			["_state"] = "Hello World";
		};
		["name"] = "myText";
		["rawData"] = {
			[1] = "Hello World";
		};
		["id"] = 5;
		["lastUpdate"] = "2017-04-19 15:59:52";
		["deviceType"] = "General";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "Text";
	};
	[6] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "82006";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["typeImage"] = "current";
			["_state"] = "0";
		};
		["name"] = "myUsage";
		["rawData"] = {
			[1] = "0";
		};
		["id"] = 6;
		["lastUpdate"] = "2017-05-12 10:15:46";
		["deviceType"] = "Usage";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "Electric";
	};
	[7] = {
		["switchTypeValue"] = 0;
		["lastLevel"] = 0;
		["hardwareName"] = "dummy";
		["hardwareID"] = "2";
		["baseType"] = "device";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareTypeID"] = 15;
		["deviceID"] = "00082007";
		["timedOut"] = "true";
		["switchType"] = "On/Off";
		["data"] = {
			["counterToday"] = "0.000 kWh";
			["typeImage"] = "current";
			["_state"] = "0;0.0";
			["whActual"] = "0";
			["whTotal"] = "0.0";
			["usage"] = "0.0 Watt";
		};
		["name"] = "myElectric";
		["rawData"] = {
			[1] = "0";
			[2] = "0.0";
		};
		["id"] = 7;
		["lastUpdate"] = "2017-05-12 10:16:09";
		["deviceType"] = "General";
		["signalLevel"] = 255;
		["description"] = "";
		["batteryLevel"] = 255;
		["changed"] = "false";
		["subType"] = "kWh";
	};
	[8] = {
		["id"] = 1;
		["baseType"] = "scene";
		["data"] = {
			["_state"] = "Off";
		};
		["name"] = "scene1";
		["description"] = "";
		["lastUpdate"] = "2017-04-18 15:31:19";
	};
	[9] = {
		["id"] = 2;
		["baseType"] = "group";
		["data"] = {
			["_state"] = "Off";
		};
		["name"] = "group1";
		["description"] = "";
		["lastUpdate"] = "2017-04-18 15:31:26";
	};
	[10] = {
		["id"] = 3;
		["baseType"] = "scene";
		["data"] = {
			["_state"] = "Off";
		};
		["name"] = "scene2";
		["description"] = "";
		["lastUpdate"] = "2017-04-19 20:31:50";
	};
	[11] = {
		["id"] = 4;
		["baseType"] = "group";
		["data"] = {
			["_state"] = "Off";
		};
		["name"] = "group2";
		["description"] = "";
		["lastUpdate"] = "2017-04-19 20:31:57";
	};
	[12] = {
		["id"] = 1;
		["baseType"] = "uservariable";
		["data"] = {
			["_state"] = 1;
		};
		["variableType"] = "integer";
		["name"] = "myVar1";
		["lastUpdate"] = "2017-04-18 15:31:36";
	};
}
return obj1
