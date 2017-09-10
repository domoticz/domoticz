local log
local dz
local _ = require('lodash')

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('Stage: 2, ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
end

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

local checkAttributes = function(dev, attributes)
	local res = true
	for attr, value in pairs(attributes) do
		res = res and expectEql(dev[attr], value, attr)
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
	tstMsg('Test air quality device', res)
	return res
end

local testSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
	})
	tstMsg('Test switch device', res)
	return res
end

local testDimmer = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 39,
		["state"] = "On",
		["lastLevel"] = 75, -- this script is NOT triggered by the dimmer so lastLevel is the current level
		["level"] = 75;
	})
	tstMsg('Test dimmer', res)
	return res
end

local testAlert = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "Hey I am red",
		["color"] = 4,
	})
	tstMsg('Test alert sensor device', res)
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
	tstMsg('Test barometer device', res)
	return res
end

local testCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["counterToday"] = 0;
		["counter"] = 1.234;
	})
	tstMsg('Test counter device', res)
	return res
end

local testCounterIncremental = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["counter"] = 1.234;
		["counterToday"] = 0;
	})
	tstMsg('Test counter incremental device', res)
	return res
end

local testCustomSensor = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "1234";
	})
	tstMsg('Test custom sensor device', res)
	return res
end

local testDistance = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = '42.44',
		["distance"] = 42.44,
	})
	tstMsg('Test distance device', res)
	return res
end

local testElectricInstanceCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['WhTotal'] = 20,
		['WhActual'] = 10,
		['counterToday'] = 0.020,
		['usage'] = 10.0,
	})
	tstMsg('Test electric instance counter device', res)
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
	tstMsg('Test gas device', res)
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
	tstMsg('Test humidity device', res)
	return res
end

local testLeafWetness = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['wetness'] = 55,
	})
	tstMsg('Test leaf wetness device', res)
	return res
end

local testLux = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['lux'] = 355,
	})
	tstMsg('Test lux device', res)
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
	tstMsg('Test p1 smart meter device', res)
	return res
end

local testPercentage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['percentage'] = 99.99,
	})
	tstMsg('Test percentage device', res)
	return res
end

local testPressureBar = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["pressure"] = 88,
	})
	tstMsg('Test pressure device', res)
	return res
end

local testRain = function(name)
	local dev = dz.devices(name)
	local res = true
	expectEql(3000, tonumber(dev.rawData[1]))
	expectEql(6660, tonumber(dev.rawData[2]))
	tstMsg('Test rain device', res)
	return res
end

local testRGB = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
		["level"] = 15,
	})
	tstMsg('Test rgb device', res)
	return res
end

local testRGBW = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["state"] = "On",
		["level"] = 15,
	})
	tstMsg('Test rgbw device', res)
	return res
end

local testScaleWeight = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		['weight'] = 33.5,
	})
	tstMsg('Test scale weight device', res)
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

	tstMsg('Test selector switch device', res)
	return res
end

local testSoilMoisture = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["moisture"] = 34,
	})
	tstMsg('Test soil moisture device', res)
	return res
end

local testSolarRadiation = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["radiation"] = 34,
	})
	tstMsg('Test solar radiation device', res)
	return res
end

local testSoundLevel = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["level"] = 120,
	})
	tstMsg('Test sound level device', res)
	return res
end

local testTemperature = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 120,
	})
	tstMsg('Test temperature device', res)
	return res
end

local testAPITemperature = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["temperature"] = 42,
	})
	tstMsg('Test API temperature device', res)
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
	tstMsg('Test temperature+humidity device', res)
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
	tstMsg('Test temperature+humidity+barometer device', res)
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
	tstMsg('Test temperature+barometer device', res)
	return res
end


local testText = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["text"] = "Oh my Darwin, what a lot of tests!"
	})
	tstMsg('Test text device', res)
	return res
end

local testThermostatSetpoint = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["setPoint"] = 22,
	})
	tstMsg('Test thermostat device', res)
	return res
end

local testUsageElectric = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["WhActual"] = 1922,
	})
	tstMsg('Test usage electric device', res)
	return res
end

local testUV = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["uv"] = 12.33,
	})
	tstMsg('Test uv device', res)
	return res
end

local testVisibility = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(1, dev.visibility)
	tstMsg('Test visibility device', res)
	return res
end

local testVoltage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["voltage"] = 220,
	})
	tstMsg('Test voltage device', res)
	return res
end

local testWaterflow = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["flow"] = 15,
	})
	tstMsg('Test waterflow device', res)
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
	tstMsg('Test wind device', res)
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
	tstMsg('Test group', res)
	return res
end

local testVariableInt = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 43
	})
	tstMsg('Test variable: int', res)
	return res
end

local testVariableFloat = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 43
	})
	tstMsg('Test variable: float', res)
	return res
end

local testVariableString = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		['value'] = 'Zork is a dork'
	})
	tstMsg('Test variable: string', res)
	return res
end

local testVariableDate = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and expectEql(var.date.month, 11)
	res = res and expectEql(var.date.day, 20)
	res = res and expectEql(var.date.year, 2016)
	tstMsg('Test variable: date', res)
	return res
end

local testVariableTime = function(name)
	local var = dz.variables(name)
	local res = true

	res = res and expectEql(var.time.hour, 9)
	res = res and expectEql(var.time.min, 54)
	tstMsg('Test variable: time', res)
	return res
end

local testAmpere1 = function(name)
	local dev = dz.devices(name)
	local res = true

	res = res and checkAttributes(dev, {
		['current'] = 123,
	})
	tstMsg('Test ampere 1 device', res)
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

	tstMsg('Test ampere 3 device', res)
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

	tstMsg('Test repeat switch', res)
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

	tstMsg('Test lastUpdates', results)
	return results
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
		res = res and testHumidity('vdHumidity')
		res = res and testLeafWetness('vdLeafWetness')
		res = res and testLux('vdLux')
		res = res and testP1SmartMeter('vdP1SmartMeterElectric')
		res = res and testPercentage('vdPercentage')
		res = res and testPressureBar('vdPressureBar')
		res = res and testRain('vdRain')
		res = res and testRGB('vdRGBSwitch')
		res = res and testRGBW('vdRGBWSwitch')
		res = res and testScaleWeight('vdScaleWeight')
		res = res and testSelectorSwitch('vdSelectorSwitch')
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

		res = res and testLastUpdates(stage2Trigger)
		res = res and testRepeatSwitch('vdRepeatSwitch')

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
			dz.devices('endResult').updateText('SUCCEEDED')
		end

		log('Finishing stage 2')
	end
}