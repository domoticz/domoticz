local _ = require 'lodash'
_G._ = require 'lodash'

local scriptPath = ''
--package.path = package.path .. ";../?.lua"
package.path = package.path .. ";../../?.lua" -- two folders up
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'


describe('Event dispatching', function()

	setup(function()
		_G.TESTMODE = true
		_G.TESTTIME = 2017 .. '-' .. 6 .. '-' .. 13 .. ' ' .. 12 .. ':' .. 5 .. ':' .. 0

		_G.timeofday = {
			Daytime = 'dt',
			Nighttime = 'nt',
			SunriseInMinutes = 'sunrisemin',
			SunsetInMinutes = 'sunsetmin'
		}

		_G.globalvariables = {
			['radix_separator'] = '.',
			['Security'] = 'sec',
			['domoticz_listening_port'] = '8080',
			['script_path'] = scriptPath,
			['currentTime'] = '2017-08-17 12:13:14.123'
		}

	end)

	before_each(function()
		_G.domoticzData = {
			[1] = {
				["deviceType"] = "Light/Switch",
				["deviceID"] = "00014051",
				["lastLevel"] = 61,
				["switchTypeValue"] = 0,
				["id"] = 1,
				["subType"] = "Switch",
				["timedOut"] = "false",
				["signalLevel"] = 255,
				["data"] = {
					["_state"] = "On",
					["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)",
					["hardwareName"] = "dummy",
					["hardwareID"] = "2",
					["icon"] = "lightbulb",
					["maxDimLevel"] = "100",
				},
				["batteryLevel"] = 255,
				["description"] = "",
				["rawData"] = {
					[1] = "0",
				},
				["name"] = "onscript1",
				["baseType"] = "device",
				["changed"] = true,
				["lastUpdate"] = "2017-05-18 09:52:19",
				["switchType"] = "On/Off",
			},
			[2] = {
				["description"] = "",
				["name"] = "scene1",
				["id"] = 1,
				["data"] = {
					["_state"] = "On",
				},
				["lastUpdate"] = "2017-04-19 20:31:50",
				["baseType"] = "scene",
			},
			[3] = {
				["deviceType"] = "Light/Switch",
				["deviceID"] = "00014051",
				["lastLevel"] = 61,
				["switchTypeValue"] = 0,
				["id"] = 2,
				["subType"] = "Switch",
				["timedOut"] = "false",
				["signalLevel"] = 255,
				["data"] = {
					["_state"] = "On",
					["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)",
					["hardwareName"] = "dummy",
					["hardwareID"] = "2",
					["icon"] = "lightbulb",
					["maxDimLevel"] = "100",
				},
				["batteryLevel"] = 255,
				["description"] = "",
				["rawData"] = {
					[1] = "0",
				},
				["name"] = "device1",
				["baseType"] = "device",
				["changed"] = false,
				["lastUpdate"] = "2017-05-18 09:52:19",
				["switchType"] = "On/Off",
			},
			[4] = {
				["id"] = 1;
				["lastUpdate"] = "2017-05-27 17:18:07";
				["variableType"] = "integer";
				["data"] = {
					["value"] = 3345;
				};
				["baseType"] = "uservariable";
				["changed"] = true;
				["name"] = "myVar1";
			},
			[5] = {
				["id"] = 1;
				["lastUpdate"] = "2017-05-27 17:18:07";
				["baseType"] = "scene";
				["data"] = {
					["_state"] = 'On';
				};
				["changed"] = true;
				["name"] = "myscene1";
			},
			[6] = {
				["id"] = 2;
				["lastUpdate"] = "2017-05-27 17:18:07";
				["baseType"] = "group";
				["data"] = {
					["_state"] = 'Off';
				};
				["changed"] = true;
				["name"] = "mygroup1";
			}
		}
	end)

	after_each(function()
		_G.domoticzData = {}
		package.loaded['dzVents'] = nil
	end)

	it('should dispatch device events', function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'device'

		local main = require('dzVents')

		assert.is_same({
			{ ["onscript1"] = "Off" },
			{ ["onscript1"] = "Set Level 10" },
			{ ["SendNotification"] = 'Yo##0#pushover##' }
		}, main)
	end)

	it('should dispatch scene/group events', function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'scenegroup'

		local main = require('dzVents')

		assert.is_same({
			{ ["Scene:myscene1"] = 'Off' },
			{ ["Group:mygroup1"] = 'On' },
		}, main)
	end)

	it("should dispatch timer events", function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'time'
		local main = require('dzVents')
		assert.is_same({
			{ ["onscript1"] = "Off" },
			{ ['Scene:scene 2'] = 'On' },
			{ ["SendNotification"] = "Me#timer every minute#0#pushover##" },
			{ ["Scene:scene 1"] = "On" }
		}, main)
	end)

	it("should dispatch variable events", function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'uservariable'
		local main = require('dzVents')
		assert.is_same({
			{ ['Variable:myVar1'] =  '10 TRIGGER' }
		}, main)
	end)

end)