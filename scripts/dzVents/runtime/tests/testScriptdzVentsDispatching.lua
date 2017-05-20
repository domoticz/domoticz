local _ = require 'lodash'
_G._ = require 'lodash'

local scriptPath = ''
--package.path = package.path .. ";../?.lua"
package.path = package.path .. ";../../?.lua" -- two folders up
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'


describe('script_time_main', function()

	setup(function()
		_G.TESTMODE = true

		_G.timeofday = {
			Daytime = 'dt',
			Nighttime = 'nt',
			SunriseInMinutes = 'sunrisemin',
			SunsetInMinutes = 'sunsetmin'
		}

		_G.globalvariables = {
			['radix_separator'] = '.',
			Security = 'sec',
			['script_path'] = scriptPath
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
				["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)",
				["subType"] = "Switch",
				["timedOut"] = "false",
				["signalLevel"] = 255,
				["data"] = {
					["_state"] = "On",
					["icon"] = "lightbulb",
					["maxDimLevel"] = "100",
				},
				["batteryLevel"] = 255,
				["description"] = "",
				["hardwareName"] = "dummy",
				["hardwareTypeID"] = 15,
				["hardwareID"] = "2",
				["rawData"] = {
					[1] = "0",
				},
				["name"] = "onscript1",
				["baseType"] = "device",
				["changed"] = "false",
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
				["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)",
				["subType"] = "Switch",
				["timedOut"] = "false",
				["signalLevel"] = 255,
				["data"] = {
					["_state"] = "On",
					["icon"] = "lightbulb",
					["maxDimLevel"] = "100",
				},
				["batteryLevel"] = 255,
				["description"] = "",
				["hardwareName"] = "dummy",
				["hardwareTypeID"] = 15,
				["hardwareID"] = "2",
				["rawData"] = {
					[1] = "0",
				},
				["name"] = "device1",
				["baseType"] = "device",
				["changed"] = "false",
				["lastUpdate"] = "2017-05-18 09:52:19",
				["switchType"] = "On/Off",
			},
		}
	end)

	after_each(function()
		_G.domoticzData = {}
		package.loaded['dzVents'] = nil
	end)

	teardown(function()

	end)

	it('should dispatch device events', function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'device'

		local main = require('dzVents')

		assert.is_same({
			{ ["onscript1"] = "Off" },
			{ ["onscript1"] = "Set Level 10" },
			{ ["UpdateDevice"] = "1|0|123" }
		}, main)
	end)


	it("should dispatch timer events", function()
		_G.commandArray = {}
		_G.globalvariables['script_reason'] = 'time'
		local main = require('dzVents')
		assert.is_same({
			{ ["onscript1"] = "Off" },
			{ ["SendNotification"] = "Me#timer every minute#0#pushover" },
			{ ["Scene:scene 1"] = "On" }
		}, main)
	end)

end)