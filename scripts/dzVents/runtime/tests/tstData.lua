local testData = {

	domoticzData = {
		[1] = {
			["id"] = 1,
			["name"] = "device1",
			["description"] = "Description 1";
			["batteryLevel"] = 10,
			["signalLevel"] = 10,
			["subType"] = "Zone";
			["timedOut"] = true,
			["switchType"] = "Contact",
			["switchTypeValue"] = 2,
			["lastUpdate"] = "2016-03-20 12:23:00";
			["data"] = {
				["_state"] = "On",
				temperature = 37,
				dewpoint = 55,
				humidity = 66,
				setPoint = 2,
				["deviceType"] = "Heating",
				["hardwareName"] = "hw1",
				["hardwareType"] = "ht1",
				["hardwareTypeValue"] = 1;
				["hardwareID"] = 1,
				heatingMode = '3'
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "1",
				[2] = '2',
				[3] = '3'
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},
		[2] = {
			["id"] = 2,
			["name"] = "device2",
			["description"] = "Description 2";
			["batteryLevel"] = 20,
			["signalLevel"] = 20,
			["subType"] = "Lux";
			["deviceType"] = "Lux",
			["timedOut"] = false,
			["switchType"] = "Motion Sensor",
			["switchTypeValue"] = 8,
			["lastUpdate"] = "2016-03-20 12:23:00";
			["data"] = {
				["_state"] = "Off",
				["hardwareName"] = "hw2",
				["hardwareType"] = "ht2",
				["hardwareTypeValue"] = 2;
				["hardwareID"] = 2,
				temperature = 12,
				dewpoint = 66,
				humidity = 67,
				--lux = 4
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "4",
				[2] = '5',
				[3] = '6'
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},
		[3] = {
			["id"] = 3,
			["name"] = "device3",
			["description"] = "Description 3";
			["batteryLevel"] = 30,
			["signalLevel"] = 30,
			["subType"] = "Energy";
			["deviceType"] = "P1 Smart Meter",
			["timedOut"] = false,
			["switchType"] = "On/Off",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2016-03-20 12:23:00";
			["data"] = {
				["_state"] = 120,
				["hardwareName"] = "hw3",
				["hardwareType"] = "ht3",
				["hardwareTypeValue"] = 3;
				["hardwareID"] = 3,
				WActual = 11,
				uv = 23
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "7",
				[2] = '8',
				[3] = '9',
				[4] = '10',
				[5] = '11'
			},
			["baseType"] = "device";
			["changed"] = false;
			["changedAttribute"] = nil --tbd
		},
		[4] = {
			["id"] = 4,
			["name"] = "device4",
			["description"] = "Description 4";
			["batteryLevel"] = 40,
			["signalLevel"] = 0,
			["subType"] = "SetPoint";
			["deviceType"] = "Thermostat",
			["timedOut"] = false,
			["switchType"] = "Security",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2016-03-20 12:23:00";
			["data"] = {
				["_state"] = "Set Level 5%",
				level = 5,
				barometer = 333,
				["hardwareName"] = "hw4",
				["hardwareType"] = "ht4",
				["hardwareTypeValue"] = 4;
				["hardwareID"] = 4,
				utility = 123,
				weather = 'Nice',
				rainLastHour = 12,
				rain = 666,
				setPoint = 10
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "10",
				[2] = '11',
				[3] = '12'
			},
			["baseType"] = "device";
			["changed"] = false;
			["changedAttribute"] = nil --tbd
		},
		[5] = {
			["id"] = 5,
			["name"] = "device5",
			["description"] = "Description 5";
			["batteryLevel"] = 40,
			["signalLevel"] = 0,
			["subType"] = "kWh";
			["deviceType"] = "General",
			["timedOut"] = false,
			["switchType"] = "Security",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["_state"] = "On",
				counterToday = '1.234 kWh',
				counter = '567 kWh',
				["hardwareName"] = "hw5",
				["hardwareType"] = "ht5",
				["hardwareTypeValue"] = 5;
				["hardwareID"] = 0,
				WhTotal = 14,
				WActual = 13,
				WhToday = 1.234,
				level = 10,
				counterTotal = '567 kWh'
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "13",
				[2] = '14',
				[3] = '15'
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},
		[6] = {
			["id"] = 6,
			["name"] = "device6",
			["description"] = "Description 6";
			["batteryLevel"] = 40,
			["signalLevel"] = 0,
			["subType"] = "Electric";
			["deviceType"] = "Usage",
			["timedOut"] = false,
			["switchType"] = "",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["hardwareName"] = "hw4",
				["hardwareType"] = "ht4",
				["hardwareTypeValue"] = 4;
				["hardwareID"] = 4,
				["value"] = 16.5, -- ?
			};
			["deviceID"] = "",
			["rawData"] = {},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},
		[7] = {
			["id"] = 7,
			["name"] = "device7",
			["description"] = "";
			["batteryLevel"] = 10,
			["signalLevel"] = 10,
			["subType"] = "";
			["deviceType"] = "",
			["timedOut"] = true,
			["switchType"] = "",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["hardwareName"] = "",
				["hardwareType"] = "",
				["hardwareTypeValue"] = 0;
				["hardwareID"] = 0,
				["_state"] = 16.5,
				WActual = 16.5
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "16.5"
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},
		[8] = {
			["id"] = 8,
			["name"] = "device8",
			["description"] = "";
			["batteryLevel"] = 10,
			["signalLevel"] = 10,
			["subType"] = "Text";
			["deviceType"] = "General",
			["timedOut"] = true,
			["switchType"] = "",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["hardwareName"] = "",
				["hardwareType"] = "",
				["hardwareTypeValue"] = 0;
				["hardwareID"] = 0,
				["_state"] = 16.5,
				text = 'Blah'
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "16.5"
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},

		[9] = {
			["id"] = 9,
			["name"] = "device9", -- double name
			["description"] = "";
			["batteryLevel"] = 10,
			["signalLevel"] = 10,
			["subType"] = "Text";
			["deviceType"] = "General",
			["timedOut"] = true,
			["switchType"] = "",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["hardwareName"] = "",
				["hardwareType"] = "",
				["hardwareTypeValue"] = 0;
				["hardwareID"] = 0,
				["_state"] = 16.5,
				text = 'Blah'
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "16.5"
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},

		[10] = {
			["id"] = 10,
			["name"] = "device9", -- double name
			["description"] = "";
			["batteryLevel"] = 10,
			["signalLevel"] = 10,
			["subType"] = "Text";
			["deviceType"] = "General",
			["timedOut"] = true,
			["switchType"] = "",
			["switchTypeValue"] = 0,
			["lastUpdate"] = "2017-04-18 20:15:23";
			["data"] = {
				["hardwareName"] = "",
				["hardwareType"] = "",
				["hardwareTypeValue"] = 0;
				["hardwareID"] = 0,
				["_state"] = 16.5,
				text = 'Blah'
			};
			["deviceID"] = "",
			["rawData"] = {
				[1] = "16.5"
			},
			["baseType"] = "device";
			["changed"] = true;
			["changedAttribute"] = nil --tbd
		},


		--- vars
		[11] = {
			["id"] = 1,
			["name"] = "x",
			["changed"] = true,
			["variableType"] = 'integer',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:15:23";
			data = {
				["value"] = 1
			}
		},
		[12] = {
			["id"] = 2,
			["name"] = "y",
			["changed"] = false,
			["variableType"] = 'float',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:16:23";
			data = { ["value"] = 2.3 }
		},
		[13] = {
			["id"] = 3,
			["name"] = "z",
			["changed"] = true,
			["variableType"] = 'string',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:16:23";
			data = { ["value"] = 'some value'}
		},
		[14] = {
			["id"] = 4,
			["name"] = "a",
			["changed"] = true,
			["variableType"] = 'date',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:16:23";
			data = { ["value"] = '3/12/2017' }
		},
		[15] = {
			["id"] = 5,
			["name"] = "b",
			["changed"] = true,
			["variableType"] = 'time',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:16:23";
			data = { ["value"] = '19:34' }
		},
		[16] = {
			["id"] = 6,
			["name"] = "var with spaces",
			["changed"] = true,
			["variableType"] = 'string',
			["baseType"] = "uservariable";
			["lastUpdate"] = "2017-04-18 20:16:23";
			data = { ["value"] = 'some value' }
		},
		--- scenes and groups
		[17] = {
			["id"] = 1;
			["baseType"] = "scene";
			["description"] = 'Descr scene 1',
			["name"] = "Scene1";
			["data"] = {
				_state = "Off"
			},
			["lastUpdate"] = "2017-04-18 15:31:19";
		},
		[18] = {
			["id"] = 3;
			["baseType"] = "group";
			["description"] = 'Descr group 1',
			["name"] = "Group1";
			["data"] = {
				_state = "On"
			},
			["lastUpdate"] = "2017-04-18 15:31:26";
		},
		[19] = {
			["id"] = 2;
			["baseType"] = "scene";
			["description"] = 'Descr scene 2',
			["name"] = "Scene2";
			["data"] = {
				_state = "Off"
			},
			["lastUpdate"] = "2017-04-19 20:31:50";
		},
		[20] = {
			["id"] = 4;
			["baseType"] = "group";
			["description"] = 'Descr group 2',
			["name"] = "Group2";
			["data"] = {
				_state = "Mixed"
			},
			["lastUpdate"] = "2017-04-19 20:31:57";
		},


		-- groups

	}

}

return testData