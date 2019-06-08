local log
local dz
local _ = require('lodash')
local resTable = {}

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('Stage: 2, ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
end

local handleResult = function(msg,res)
	tstMsg(msg,res)
	if res then
		resTable[msg] = res
	else
		resTable[msg] = msg .. " Failed"
	end
end

local descriptionString = "Ieder nadeel heb zn voordeel"

local expectEql = function(attr, test, marker)
	if (attr ~= test) then
		local msg = tostring(attr) .. '~=' .. tostring(test)
		if (marker ~= nil) then
			msg = msg .. ' (' .. tostring(marker) .. ')'
		end
		err(msg)
		print(debug.traceback())
		return false
	end
	return true
end

local checkAttributes = function(item, attributes)
	local res = true
	for attr, value in pairs(attributes) do
		res = res and expectEql(item[attr], value, attr)
	end
	return res
end

local testAirQuality = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["quality"] = 'Bad',
		["co2"] = 1600,
	})
	handleResult('Test air quality device', res)
	return res
end

local testSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
	})
	handleResult ('Test switch device', res)
	return res
end

local testDimmer = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 40,
		["state"] = "On",
		["lastLevel"] = 75, -- this script is NOT triggered by the dimmer so lastLevel is the current level
		["level"] = 75;
	})
	handleResult ('Test dimmer', res)
	return res
end

local testAlert = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "Hey I am red",
		["color"] = 4,
	})
	handleResult('Test alert sensor device', res)
	return res
end

local testBarometer = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["forecastString"] = "Thunderstorm";
		["forecast"] = 4;
		["barometer"] = 1234
	})
	handleResult('Test barometer device', res)
	return res
end

local testCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["counterToday"] = 0;
		["counter"] = 1.234;
	})
	handleResult('Test counter device', res)
	return res
end

local testCounterIncremental = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["counter"] = 1.244;
		["counterToday"] = 0;
	})
	handleResult('Test counter incremental device', res)
	return res
end

local testCustomSensor = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "1234";
	})
	handleResult('Test custom sensor device', res)
	return res
end

local testDistance = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = '42.44',
		["distance"] = 42.44,
	})
	handleResult('Test distance device', res)
	return res
end

local testElectricInstanceCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['WhTotal'] = 20,
		['WhActual'] = 10,
		-- only works after at least 5 minutes ['counterToday'] = 0.020,
		['usage'] = 10.0,
	})
	handleResult('Test electric instance counter device', res)
	return res
end

local testGas = function(name)
	local dev = dz.devices(name)
	local res = true
	-- this doesn't have a direct effect so we cannot test this unfortunately
	--	res = res and checkAttributes(dev, {
	--		['counterToday'] = 66,
	--		['counter'] = 66,
	--	})
	handleResult('Test gas device', res)
	return res
end

local testHumidity = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["humidityStatus"] = "Wet";
		["humidityStatusValue"] = 3;
		["humidity"] = 88;
	})
	handleResult('Test humidity device', res)
	return res
end

local testLeafWetness = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['wetness'] = 55,
	})
	handleResult('Test leaf wetness device', res)
	return res
end

local testLux = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['lux'] = 355,
	})
	handleResult('Test lux device', res)
	return res
end


local testManagedCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["counter"] = 1.234;
		["counterToday"] = 0;
	})
	handleResult('Test managed counter', res)
	return res
end


local testP1SmartMeter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["WhActual"] = 666,
		['usage1'] = 10,
		['usage2'] = 20,
		['return1'] = 100,
		['return2'] = 200,
		['usage'] = 666,
		['usageDelivered'] = 777,
		['counterDeliveredToday'] = 0,
		['counterToday'] = 0,
	})
	handleResult('Test p1 smart meter device', res)
	return res
end

local testPercentage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['percentage'] = 99.99,
	})
	handleResult('Test percentage device', res)
	return res
end

local testPressureBar = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["pressure"] = 88,
	})
	handleResult('Test pressure device', res)
	return res
end

local testRain = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(3000, tonumber(dev.rawData[1]))
	res = res and expectEql(6660, tonumber(dev.rawData[2]))
	handleResult('Test rain device', res)
	return res
end

local testRGB = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
		["level"] = 15,

	})
	handleResult('Test rgb device ', res)
	return res
end

local testRGBW = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
		["level"] = 15,
		["color"] = '{"b":255,"cw":0,"g":85,"m":3,"r":0,"t":0,"ww":0}'
	})
	handleResult('Test rgbw device', res)
	return res
end

local testScaleWeight = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['weight'] = 33.5,
	})
	handleResult('Test scale weight device', res)
	return res
end

local testSelectorSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['levelName'] = 'Level3',
		["state"] = 'Level3',
		['level'] = 30
	})

	handleResult('Test selector switch device', res)
	return res
end

local testSoilMoisture = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["moisture"] = 34,
	})
	handleResult('Test soil moisture device', res)
	return res
end

local testSolarRadiation = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["radiation"] = 34,
	})
	handleResult('Test solar radiation device', res)
	return res
end

local testSoundLevel = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["level"] = 120,
	})
	handleResult('Test sound level device', res)
	return res
end

local testTemperature = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 120,
	})
	handleResult('Test temperature device', res)
	return res
end

local testAPITemperature = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 42,
	})
	handleResult('Test API temperature device', res)
	return res
end


local testTempHum = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 34,
		["humidity"] = 88,
		["humidityStatus"] = "Wet";
		["humidityStatusValue"] = 3;
	})
	handleResult('Test temperature+humidity device', res)
	return res
end

local testTempHumBaro = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 34,
		["humidity"] = 88,
		["barometer"] = 1033,
		["forecastString"] = "Partly Cloudy";
		["forecast"] = 2;
		["humidityStatus"] = "Wet";
		["humidityStatusValue"] = 3;
	})
	handleResult('Test temperature+humidity+barometer device', res)
	return res
end

local testTempBaro = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 34,
		["barometer"] = 1033,
		["forecastString"] = "Cloudy";
		["forecast"] = 2;
	})
	handleResult('Test temperature+barometer device', res)
	return res
end

local testGetColorRGBW = function(name)
	local dev = dz.devices(name)
	local res = true
	local ct = dev.getColor()
	res = res and checkAttributes(ct, {
		["r"] = 0,
		["red"] = 0,
		["hue"] = 220,
		["saturation"] = 100,
		["value"] = 100,
		["isWhite"] = false,
		["b"] = 255,
		["blue"] = 255,
		["g"] = 85,
		["green"] = 85,
		["warm white"] = 0,
		["cold white"] = 0,
		["temperature"] = 0,
		["mode"] = 3,
		["brightness"] = 100,
	})
	handleResult('Test getColor RGBW(' .. dev.color .. ')', res)
	return res
end

local testGetColorRGB = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and (dev.getColor() == nil)
	handleResult('Test getColor RGB (' .. dev.color .. ')', res)
	return res
end

local testText = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["text"] = "Oh my Darwin, what a lot of tests!"
	})

	local Time = require('Time')

	local stage1Time = Time(dz.globalData.stage1Time)

	local diff = dev.lastUpdate.compare(stage1Time).secs

	res = res and (diff >= 3)

	res = expectEql(true, res, 'Text device should have been delayed with 3 seconds.')

	handleResult('Test text device', res)
	return res
end

local testThermostatSetpoint = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["setPoint"] = 22.00,
	})
	handleResult('Test thermostat device', res)
	return res
end

local testUsageElectric = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["WhActual"] = 1922,
	})
	handleResult('Test usage electric device', res)
	return res
end

local testUV = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["uv"] = 12.33,
	})
	handleResult('Test uv device', res)
	return res
end

local testVisibility = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(1, dev.visibility)
	handleResult('Test visibility device', res)
	return res
end

local testVoltage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["voltage"] = 220,
	})
	handleResult('Test voltage device', res)
	return res
end

local testWaterflow = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["flow"] = 15,
	})
	handleResult('Test waterflow device', res)
	return res
end

local testWind = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["gust"] = 66,
		["temperature"] = 77,
		["speed"] = 55,
		["direction"] = 120,
		['directionString'] = "SW",
		['chill'] = 88,
	})
	handleResult('Test wind device', res)
	return res
end

local testGroup = function(name)
	local group = dz.groups(name)
	local res = true
	res = res and checkAttributes(group, {
		["id"] = 2,
		["name"] = name,
		['state'] = 'On',
		['baseType'] = 'group'
	})
	handleResult('Test group', res)
	return res
end

local testVariableInt = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 43
	})
	handleResult('Test variable: int', res)
	return res
end

local testVariableFloat = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 43
	})
	handleResult('Test variable: float', res)
	return res
end

local testVariableString = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 'Zork is a dork'
	})
	handleResult('Test variable: string', res)
	return res
end

local testVariableDate = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and expectEql(var.date.month, 11)
	res = res and expectEql(var.date.day, 20)
	res = res and expectEql(var.date.year, 2016)
	handleResult('Test variable: date', res)
	return res
end

local testVariableTime = function(name)
	local var = dz.variables(name)
	local res = true

	res = res and expectEql(var.time.hour, 9)
	res = res and expectEql(var.time.min, 54)
	handleResult('Test variable: time', res)
	return res
end

local testAmpere1 = function(name)
	local dev = dz.devices(name)
	local res = true

	res = res and checkAttributes(dev, {
		['current'] = 123,
	})
	handleResult('Test ampere 1 device', res)
	return res
end

local testAmpere3 = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['current1'] = 123,
		['current2'] = 456,
		['current3'] = 789,
	})

	handleResult('Test ampere 3 device', res)
	return res
end

local testRepeatSwitchDelta = function(expectedState, expectedDelta, state, delta)

	local res = true

	res = res and expectEql(state, expectedState, 'State is not correct')

	local deltaDiff = delta - expectedDelta
	local ok = (deltaDiff <= 1) -- some leniency
	res = res and expectEql(true, ok, 'Time between events is not correct')

	return res
end

local testRepeatSwitch = function(name)
	local dev = dz.devices(name)
	local res = true

	local start = dz.globalData.repeatSwitch.getOldest()
	local firstOn = dz.globalData.repeatSwitch.get(4)
	local firstOff = dz.globalData.repeatSwitch.get(3)
	local secondOn = dz.globalData.repeatSwitch.get(2)
	local secondOff = dz.globalData.repeatSwitch.get(1)

	res = res and testRepeatSwitchDelta('On', 8, firstOn.data.state, firstOn.data.delta)
	res = res and testRepeatSwitchDelta('Off', 2, firstOff.data.state, firstOff.data.delta)
	res = res and testRepeatSwitchDelta('On', 5, secondOn.data.state, secondOn.data.delta)
	res = res and testRepeatSwitchDelta('Off', 2, secondOff.data.state, secondOff.data.delta)

	handleResult('Test repeat switch', res)
	return res
end

local testCancelledRepeatSwitch = function(name)
	local res = true
	local count = dz.globalData.cancelledRepeatSwitch
	res = res and expectEql(1, count)
	handleResult('Cancelled repeat switch', res)
	return res
end

local testLastUpdates = function(stage2Trigger)

	local Time = require('Time')

	local stage1Time = Time(dz.globalData.stage1Time)
	local stage1SecsAgo = stage1Time.secondsAgo

	local results = true

	-- check if stage2Trigger.lastUpdate is older than the current time
	local now = dz.time.secondsSinceMidnight
	results = results and (stage2Trigger.lastUpdate.secondsSinceMidnight < now)

	expectEql(true, results, 'stage2Trigger.lastUpdate should be in the past')

	if (results) then
		results = dz.devices().reduce(function(acc, device)

			if (device.name ~= 'endResult' and
				device.name ~= 'stage1Trigger' and
				device.name ~= 'vdRepeatSwitch' and
				device.anme ~= 'vdText' and
				device.name ~= 'stage2Trigger') then
				local delta = stage1SecsAgo - device.lastUpdate.secondsAgo

				-- test if lastUpdate for the device is close to stage1Time
				local ok = (delta <= 10)
				acc = acc and ok -- should be significantly less that the time between stage1 and stage2
				if (not expectEql(true, ok, device.name .. ' lastUpdate is not in the past')) then
					print('stage1Time:' .. stage1Time.secondsSinceMidnight .. ' device: ' .. device.lastUpdate.secondsSinceMidnight .. ' delta: ' .. delta)
				end
			end

			return acc

		end, results)
	end

	handleResult('Test lastUpdates', results)
	return results
end

local testVarCancelled = function(name)
	local res = true
	local var = dz.variables(name)
	res = res and expectEql(0, var.value)
	handleResult('Test ' .. name .. ' variable', res)
	return res
end

local testVarDocumentation = function(name)
	local res = true
	local var = dz.variables(name)
	res = res and expectEql(0, var.value)
	handleResult('Test ' .. name .. ' variable', res)
	return res
end

local testSetValuesSensor = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(12, tonumber(dev.rawData[1]))
	res = res and expectEql(34, tonumber(dev.rawData[2]))
	res = res and expectEql(45, tonumber(dev.rawData[3]))
	handleResult('Test setValues sensor', res)
	return res
end

local testSetIconSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(10, tonumber(dev.description))
	handleResult('Test setIcon switch', res)
	return res
end

local testDeviceDump = function(name)
	local utils = require('Utils')
	local dev = dz.devices(name)
	local res = true
	res = res and ( utils.dumpTable(dev, '> ') == nil) 
	handleResult('Test device dump', res)
	return res
end

local testCameraDump = function()
	local utils = require('Utils')
	local cam = dz.cameras(1)
	local res = true
	res = res and (utils.dumpTable(cam, '> ') == nil)
	handleResult('Test camera dump', res)
	return res
end

local testSettingsDump = function()
	local utils = require('Utils')
	local settings = dz.settings
	local res = true
	res = res and (utils.dumpTable(settings, '> ') == nil)
	handleResult('Test settings dump', res)
	return res
end

local testIFTTT = function(event)
	res = true
	print('triggerIFTTT should fail now because IFTTT is disabled before stage 2'  )
	dz.triggerIFTTT(event) 
	dz.triggerIFTTT(event).afterSec(3) 
	handleResult('Test IFTTT call', res)
	return res
end

local testCancelledScene = function(name)
	local res = true
	local count = dz.globalData.cancelledScene
	res = res and expectEql(2, count)
	handleResult('Test cancelled repeat scene', res)
	return res
end

local testHTTPSwitch = function(name)
	local res = true
	local trigger = dz.globalData.httpTrigger
	res = res and expectEql('OKOKOK', trigger)
	handleResult('Test http trigger switch device', res)
	return res
end

local testDescription = function(name, description, type)
	local res = true
	local dev = dz.devices(name)
	res = res and checkAttributes(dev, {
		["description"] = description,
	})
	handleResult('Test description ' .. type ..' device for string: ' .. description, res)
	return res
end

local testQuietOn = function(name)
	local res = true
	local dev = dz.devices(name)
	res = res and checkAttributes(dev, {
		["state"] = "On",
	})
	handleResult('Test QuietOn switch device', res)
	return res
end

local testQuietOff = function(name)
	local res = true
	local dev = dz.devices(name)
	res = res and checkAttributes(dev, {
		["state"] = "Off",
	})
	handleResult('Test QuietOff switch device', res)
	return res
end


local testVersion = function(name)
	local res = true
	local utils = require('Utils')
	res = res and expectEql(utils.DZVERSION , dz.settings.dzVentsVersion)
	handleResult('Test version strings to equal (' .. utils.DZVERSION .. ') and (' ..  dz.settings.dzVentsVersion .. ')',res)
	return res
end

local writeResultsTofile = function(file, resTable)
	local utils = require('Utils')
	local results = assert(io.open(file, "wb"))
	results:write(utils.toJSON(resTable))
	results:close()
end

return {
	active = true,
	on = {
		devices = { 'stage2Trigger' }
	},
	execute = function(domoticz, stage2Trigger)

		local res = true
		dz = domoticz
		log = dz.log

		log('Starting stage 2')

		res = res and testAirQuality('vdAirQuality')
		res = res and testSwitch('vdSwitch')
		res = res and testAlert('vdAlert')
		res = res and testBarometer('vdBarometer')
		res = res and testCounter('vdCounter')
		res = res and testCounterIncremental('vdCounterIncremental')
		res = res and testCustomSensor('vdCustomSensor')
		res = res and testDistance('vdDistance')
		res = res and testElectricInstanceCounter('vdElectricInstanceCounter')
		res = res and testGas('vdGas')
		res = res and testGetColorRGBW('vdRGBWSwitch')
		res = res and testGetColorRGB('vdRGBSwitch')
		res = res and testHumidity('vdHumidity')
		res = res and testLeafWetness('vdLeafWetness')
		res = res and testLux('vdLux')
		res = res and testP1SmartMeter('vdP1SmartMeterElectric')
		res = res and testPercentage('vdPercentage')
		res = res and testPressureBar('vdPressureBar')
		res = res and testQuietOff('vdQuietOffSwitch')
		res = res and testQuietOn('vdQuietOnSwitch')
		res = res and testRain('vdRain')
		res = res and testRGB('vdRGBSwitch')
		res = res and testRGBW('vdRGBWSwitch')
		res = res and testScaleWeight('vdScaleWeight')
		res = res and testSelectorSwitch('vdSelectorSwitch')
		res = res and testSetIconSwitch('vdSetIconSwitch')
		res = res and testSetValuesSensor('vdSetValueSensor')
		res = res and testSoilMoisture('vdSoilMoisture')
		res = res and testSolarRadiation('vdSolarRadiation')
		res = res and testSoundLevel('vdSoundLevel')
		res = res and testTemperature('vdTemperature')
		res = res and testTempHum('vdTempHum')
		res = res and testTempHumBaro('vdTempHumBaro')
		res = res and testTempBaro('vdTempBaro')
		res = res and testText('vdText')
		res = res and testThermostatSetpoint("vdThermostatSetpoint")
		res = res and testUsageElectric("vdUsageElectric")
		res = res and testUV("vdUV")
		res = res and testVisibility("vdVisibility")
		res = res and testVoltage("vdVoltage")
		res = res and testWaterflow("vdWaterflow")
		res = res and testWind('vdWind')
		res = res and testWind('vdWindTempChill')
		res = res and testGroup('gpGroup')
		res = res and testVariableInt('varInteger')
		res = res and testVariableFloat('varFloat')
		res = res and testVariableString('varString')
		res = res and testVariableDate('varDate')
		res = res and testVariableTime('varTime')
		res = res and testAmpere1('vdAmpere1')
		res = res and testAmpere3('vdAmpere3')
		res = res and testDimmer('vdSwitchDimmer')
		res = res and testAPITemperature('vdAPITemperature')
		res = res and testManagedCounter('vdManagedCounter')
		res = res and testCancelledRepeatSwitch('vdCancelledRepeatSwitch')
		res = res and testLastUpdates(stage2Trigger)
		res = res and testRepeatSwitch('vdRepeatSwitch')
		res = res and testVarCancelled('varCancelled')
		res = res and testVarDocumentation('varUpdateDocument')
		res = res and testCancelledScene('scCancelledScene')
		res = res and testHTTPSwitch('vdHTTPSwitch');
		res = res and testDescription('vdDescriptionSwitch', descriptionString, "device")
		res = res and testDescription('sceneDescriptionSwitch1', descriptionString, "scene")
		res = res and testDescription('groupDescriptionSwitch1', descriptionString, "group")
		res = res and testDeviceDump(vdSwitchDimmer)
		res = res and testCameraDump()
		res = res and testSettingsDump()
		res = res and testIFTTT('myEvent')
		res = res and testVersion('version')

		-- test a require
		local m = require('some_module')
		local output = m.dzVents()
		if (output ~= 'Rocks!') then
			err('Module some_module did not load and run properly')
		end
		res = res and (output == 'Rocks!')

		if (not res) then
			log('Results stage 2: FAILED!!!!', dz.LOG_ERROR)
			dz.devices('endResult').updateText('FAILED')
		else
			log('Results stage 2: SUCCEEDED')
			dz.devices('endResult').updateText('ENDRESULT SUCCEEDED')
		end

		writeResultsTofile("/tmp/Stage2results.json", resTable)
		log('Finishing stage 2')
	end
}
