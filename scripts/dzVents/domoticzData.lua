-- Persistent Data
local multiRefObjects = {

} -- multiRefObjects
local obj1 = {
	[1] = {
		["hardwareName"] = "dummy";
		["description"] = "";
		["id"] = 1;
		["baseType"] = "device";
		["value"] = "On";
		["data"] = {
		};
		["devType"] = "Light/Switch";
		["deviceID"] = "00014051";
		["signalLevel"] = 255;
		["name"] = "s1";
		["batteryLevel"] = 255;
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["subType"] = "Switch";
		["level"] = 61;
		["switchTypeValue"] = 0;
		["hardwareID"] = "2";
		["changed"] = "true";
		["lastUpdate"] = "2017-04-19 20:32:02";
		["rawData"] = {
			[1] = "0";
		};
		["hardwareTypeID"] = 15;
		["switchType"] = "On/Off";
	};
	[2] = {
		["hardwareName"] = "dummy";
		["description"] = "";
		["id"] = 2;
		["baseType"] = "device";
		["value"] = "0.0";
		["data"] = {
			["temperature"] = 0;
		};
		["devType"] = "Temp";
		["deviceID"] = "82002";
		["signalLevel"] = 255;
		["name"] = "myTemp";
		["batteryLevel"] = 255;
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["subType"] = "LaCrosse TX3";
		["level"] = 0;
		["switchTypeValue"] = 0;
		["hardwareID"] = "2";
		["changed"] = "false";
		["lastUpdate"] = "2017-04-18 15:35:31";
		["rawData"] = {
			[1] = "0.0";
		};
		["hardwareTypeID"] = 15;
		["switchType"] = "On/Off";
	};
	[3] = {
		["hardwareName"] = "dummy";
		["description"] = "";
		["id"] = 3;
		["baseType"] = "device";
		["value"] = "0.0;50;1";
		["data"] = {
			["humidity"] = 50;
			["dewPoint"] = -9.1964807510376;
			["temperature"] = 0;
		};
		["devType"] = "Temp + Humidity";
		["deviceID"] = "82003";
		["signalLevel"] = 255;
		["name"] = "myTempHum";
		["batteryLevel"] = 255;
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["subType"] = "THGN122/123, THGN132, THGR122/228/238/268";
		["level"] = 0;
		["switchTypeValue"] = 0;
		["hardwareID"] = "2";
		["changed"] = "false";
		["lastUpdate"] = "2017-04-18 15:35:41";
		["rawData"] = {
			[1] = "0.0";
			[2] = "50";
			[3] = "1";
		};
		["hardwareTypeID"] = 15;
		["switchType"] = "On/Off";
	};
	[4] = {
		["hardwareName"] = "dummy";
		["description"] = "";
		["id"] = 4;
		["baseType"] = "device";
		["value"] = "0.0;50;1;1010;1";
		["data"] = {
			["humidity"] = 50;
			["dewPoint"] = -9.1964807510376;
			["pressure"] = 1010;
			["temperature"] = 0;
		};
		["devType"] = "Temp + Humidity + Baro";
		["deviceID"] = "82004";
		["signalLevel"] = 255;
		["name"] = "myTempHumBaro";
		["batteryLevel"] = 255;
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["subType"] = "THB1 - BTHR918, BTHGN129";
		["level"] = 0;
		["switchTypeValue"] = 0;
		["hardwareID"] = "2";
		["changed"] = "false";
		["lastUpdate"] = "2017-04-18 15:35:51";
		["rawData"] = {
			[1] = "0.0";
			[2] = "50";
			[3] = "1";
			[4] = "1010";
			[5] = "1";
		};
		["hardwareTypeID"] = 15;
		["switchType"] = "On/Off";
	};
	[5] = {
		["hardwareName"] = "dummy";
		["description"] = "";
		["id"] = 5;
		["baseType"] = "device";
		["value"] = "Hello World";
		["data"] = {
		};
		["devType"] = "General";
		["deviceID"] = "00082005";
		["signalLevel"] = 255;
		["name"] = "myText";
		["batteryLevel"] = 255;
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["subType"] = "Text";
		["level"] = 0;
		["switchTypeValue"] = 0;
		["hardwareID"] = "2";
		["changed"] = "false";
		["lastUpdate"] = "2017-04-19 15:59:52";
		["rawData"] = {
			[1] = "Hello World";
		};
		["hardwareTypeID"] = 15;
		["switchType"] = "On/Off";
	};
	[6] = {
		["id"] = 1;
		["baseType"] = "scene";
		["name"] = "scene1";
		["value"] = "Off";
		["lastUpdate"] = "2017-04-18 15:31:19";
	};
	[7] = {
		["id"] = 2;
		["baseType"] = "group";
		["name"] = "group1";
		["value"] = "Off";
		["lastUpdate"] = "2017-04-18 15:31:26";
	};
	[8] = {
		["id"] = 3;
		["baseType"] = "scene";
		["name"] = "scene2";
		["value"] = "Off";
		["lastUpdate"] = "2017-04-19 20:31:50";
	};
	[9] = {
		["id"] = 4;
		["baseType"] = "group";
		["name"] = "group2";
		["value"] = "Off";
		["lastUpdate"] = "2017-04-19 20:31:57";
	};
	[10] = {
		["id"] = 1;
		["baseType"] = "uservariable";
		["name"] = "myVar1";
		["lastUpdate"] = "2017-04-18 15:31:36";
		["variableType"] = "integer";
		["value"] = 1;
	};
}
return obj1
