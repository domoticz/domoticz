local _ = require 'lodash'
_G._ = _
package.path = package.path .. ";../?.lua"

describe('#onlyDomoticz', function()
	local Domoticz, domoticz, settings, d1, d2, d3, d4

	setup(function()
		_G.TESTMODE = true

		_G.timeofday = {
			Daytime = 'dt',
			Nighttime = 'nt',
			SunriseInMinutes = 'sunrisemin',
			SunsetInMinutes = 'sunsetmin'
		}

		_G.globalvariables = {
			Security = 'sec',
			['script_reason'] = 'device',
			['script_path'] = debug.getinfo(1).source:match("@?(.*/)")
		}

		_G.devicechanged = {
			['device1'] = 'On',
			['device2'] = 'Off',
			['device1_Temperature'] = 123
		}
		_G.otherdevices = {
			['device1'] = 'On',
			['device2'] = 'Off',
			['device3'] = 120,
			['device4'] = 'Set Level 5%',
			['device5'] = 'On',
			['device7'] = '16.5',
		}
		_G.otherdevices_temperature = {
			['device1'] = 37,
			['device2'] = 12
		}
		_G.otherdevices_dewpoint = {
			['device1'] = 55,
			['device2'] = 66
		}
		_G.otherdevices_humidity = {
			['device1'] = 66,
			['device2'] = 67
		}
		_G.otherdevices_barometer = {
			['device4'] = 333,
		}
		_G.otherdevices_utility = {
			['device4'] = 123,
		}
		_G.otherdevices_weather = {
			['device4'] = 'Nice',
		}
		_G.otherdevices_rain = {
			['device4'] = 666
		}
		_G.otherdevices_rain_lasthour = {
			['device4'] = 12
		}
		_G.otherdevices_uv = {
			['device3'] = 23
		}
		_G.otherdevices_lastupdate = {
			['device1'] = '2016-03-20 12:23:00',
			['device2'] = '2016-03-20 12:23:00',
			['device3'] = '2016-03-20 12:23:00',
			['device4'] = '2016-03-20 12:23:00'
		}
		_G.otherdevices_idx = {
			['device1'] = 1,
			['device2'] = 2,
			['device3'] = 3,
			['device4'] = 4,
			['device5'] = 5,
			['device6'] = 6,
			['device7'] = 7
		}
		_G.otherdevices_svalues = {
			['device1'] = '1;2;3',
			['device2'] = '4;5;6',
			['device3'] = '7;8;9;10;11',
			['device4'] = '10;11;12',
			['device5'] = '13;14;15',
			['device7'] = '16.5'

		}
		_G.otherdevices_scenesandgroups = {
			['Scene1'] = 'Off',
			['Scene2'] = 'Off',
			['Group1'] = 'On',
			['Group2'] = 'Mixed',
		}
		_G.uservariables = {
			x = 1,
			y = 2
		}
		_G['uservariables_lastupdate'] = {
			['myVar'] = '2016-03-20 12:23:00'
		}


		_G['domoticzData'] = {
			[1] = {
				["id"] = 1,
				["description"] = "Description 1";
				["batteryLevel"] = 10,
				["signalLevel"] = 10,
				["subType"] = "Switch";
				["deviceType"] = "Light/Switch",
				["hardwareName"] = "dummy",
				["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)",
				["hardwareTypeID"] = 15;
				["hardwareTypeValue"] = 1;
				["hardwareID"] = 2,
				["timedOut"] = true,
				["switchType"] = "On/Off",
				["switchTypeValue"] = 2,
				["lastUpdate"] = "2017-04-18 20:15:23";
				["name"] = "device1",
				["data"] = {
					["_state"] = "On",
					temperature = 123,
					humidity = 55
				};
				["deviceID"] = "00014051",
				["rawData"] = {
					[1] = "0"
				},
				["baseType"] = "device";
				["changed"] = true;
				["changedAttributes"] = {
					[1] = 'temperature',
					[2] = 'humidity'
				}
			};
		}

		settings = {
			['Domoticz ip'] = '10.0.0.8',
			['Domoticz port'] = '8080',
			['Fetch interval'] = 'every 30 minutes',
			['Enable http fetch'] = true,
			['Log level'] = 2
		}

		Domoticz = require('Domoticz')


	end)

	teardown(function()
		Domoticz = nil
		domoticz = nil
	end)

	before_each(function()
		domoticz = Domoticz(settings)
		d1 = domoticz.devices['device1']
		d2 = domoticz.devices['device2']
		d3 = domoticz.devices['device3']
		d4 = domoticz.devices['device4']
	end)

	after_each(function()
		domoticz = nil
	end)

	it('should instantiate', function()
		assert.is_not_nil(domoticz)

		domoticz.logDevice(domoticz.devices['device1'])
	end)

end)
