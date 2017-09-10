local log
local dz

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('Stage: 1, ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
end

local expectEql = function (attr, test, marker)
	if (attr ~= test) then
		local msg = tostring(attr) .. '~=' .. tostring(test)
		if (marker ~= nil) then
			msg = msg .. ' (' .. tostring(marker) ..')'
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
		["id"] = 2,
		["name"] = name,
		["quality"] = 'Excellent',
		["co2"] = 0,
		["deviceSubType"] = "Voltcraft CO-20";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "Air Quality";
	})
	dev.updateAirQuality(1600).silent()
	tstMsg('Test air quality device', res)
	return res
end

local testSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 1,
		["name"] = name,
		["maxDimLevel"] = 100,
		["state"] = "Off",
		["deviceSubType"] = "Switch";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "Light/Switch";
		["description"] = 'desc vdSwitch'
	})
	dev.switchOn().afterSec(1)
	tstMsg('Test switch device', res)
	return res
end

local testDimmer = function(name)
	local dev = dz.devices(name)

	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 39,
		["name"] = name,
		["maxDimLevel"] = 100,
		['level'] = 33,
		["lastLevel"] = 33, -- this script is NOT triggered by the dimmer so lastLevel is the current level
		["state"] = "On",
		["deviceSubType"] = "Switch";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["switchType"] = "Dimmer",
		["switchTypeValue"] = 7,
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "Light/Switch";
		["description"] = "desc vdSwitchDimmer"
	})

	dev.dimTo(75).afterSec(1)
	tstMsg('Test dimmer', res)
	return res
end

local testAlert = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 3,
		["name"] = name,
		["state"] = "No Alert!",
		["color"] = 0,
		["deviceSubType"] = "Alert";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "General";
	})
	dev.updateAlertSensor(dz.ALERTLEVEL_RED, 'Hey I am red')
	tstMsg('Test alert sensor device', res)
	return res
end

local testAmpere3 = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 4,
		['current1'] = 0,
		['current2'] = 0,
		['current3'] = 0,
		["name"] = name,
		["deviceSubType"] = "CM113, Electrisave";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "Current";
	})

	dev.updateCurrent(123, 456, 789)
	tstMsg('Test ampere 3 device', res)
	return res
end

local testAmpere1 = function(name)
	local dev = dz.devices(name)
	local res = true

	res = res and expectEql(64, math.floor(dev.current * 10))

	res = res and checkAttributes(dev, {
		["id"] = 5,
		["name"] = name,
		["state"] = "6.4",
		["deviceSubType"] = "Current";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "General";
	})

	dev.updateCurrent(123)
	tstMsg('Test ampere 1 device', res)

	return res
end

local testBarometer = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 6,
		["name"] = name,
		["forecastString"] = "Stable";
		["forecast"] = 0;
		["deviceSubType"] = "Barometer";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
		["deviceType"] = "General";
	})
	dev.updateBarometer(1234, dz.BARO_THUNDERSTORM)
	tstMsg('Test barometer device', res)
	return res
end

local testCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 7,
		["name"] = name,
		["counterToday"] = 0;
		["valueUnits"] = "";
		["valueQuantity"] = "";
		["counter"] = 0.000;
		["deviceSubType"] = "RFXMeter counter";
		["deviceType"] = "RFXMeter";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateCounter(1234)
	tstMsg('Test counter device', res)
	return res
end

local testCounterIncremental = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 8,
		["name"] = name,
		["counterToday"] = 0;
		["valueUnits"] = "";
		["valueQuantity"] = "";
		["counter"] = 0.000;
		["deviceSubType"] = "Counter Incremental";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateCounter(1234)
	tstMsg('Test counter incremental device', res)
	return res
end

local testCustomSensor = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 9,
		["name"] = name,
		["sensorUnit"] = "axis";
		["sensorType"] = 1;
		["state"] = "0.0";
		["deviceSubType"] = "Custom Sensor";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateCustomSensor(1234)
	tstMsg('Test custom sensor device', res)
	return res
end

local testDistance = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 10,
		["name"] = name,
		["state"] = '123.4',
		["distance"] = 123.4,
		["deviceSubType"] = "Distance";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateDistance(42.44)
	tstMsg('Test distance device', res)
	return res
end

local testElectricInstanceCounter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 11,
		["name"] = name,
		['WhTotal'] = 0,
		['WhActual'] = 0,
		['counterToday'] = 0,
		['usage'] = 0.0,
		["deviceSubType"] = "kWh";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateElectricity(10, 20)
	tstMsg('Test electric instance counter device', res)
	return res
end

local testGas = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 12,
		["name"] = name,
		['counterToday'] = 0,
		['counter'] = 0,
		["deviceSubType"] = "Gas";
		["deviceType"] = "P1 Smart Meter";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateGas(6000) -- this won't have a direct effect
	tstMsg('Test gas device', res)
	return res
end

local testHumidity = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 13,
		["name"] = name,
		["humidityStatus"] = "Comfortable";
		["humidity"] = 50;
		["deviceSubType"] = "LaCrosse TX3";
		["deviceType"] = "Humidity";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateHumidity(88, dz.HUM_WET)
	tstMsg('Test humidity device', res)
	return res
end

local testLeafWetness = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 14,
		["name"] = name,
		['wetness'] = 2,
		["deviceSubType"] = "Leaf Wetness";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateWetness(55)
	tstMsg('Test leaf wetness device', res)
	return res
end

local testLux = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 15,
		["name"] = name,
		['lux'] = 0,
		["deviceSubType"] = "Lux";
		["deviceType"] = "Lux";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateLux(355)
	tstMsg('Test lux device', res)
	return res
end

local testP1SmartMeter = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 16,
		["name"] = name,
		["WhActual"] = 0,
		['usage1'] = 0,
		['usage2'] = 0,
		['return1'] = 0,
		['return2'] = 0,
		['usage'] = 0,
		['usageDelivered'] = 0,
		['counterDeliveredToday'] = 0,
		['counterToday'] = 0,
		["deviceSubType"] = "Energy";
		["deviceType"] = "P1 Smart Meter";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateP1(10, 20, 100, 200, 666, 777)
	tstMsg('Test p1 smart meter device', res)
	return res
end

local testPercentage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 17,
		["name"] = name,
		['percentage'] = 0,
		["deviceSubType"] = "Percentage";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updatePercentage(99.99)
	tstMsg('Test percentage device', res)
	return res
end

local testPressureBar = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 18,
		["name"] = name,
		["pressure"] = 0,
		["deviceSubType"] = "Pressure";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updatePressure(88)
	tstMsg('Test pressure device', res)
	return res
end

local testRain = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 19,
		["name"] = name,
		["rain"] = 0,
		["rainRate"] = 0,
		["deviceSubType"] = "TFA";
		["deviceType"] = "Rain";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateRain(3000, 6660)
	tstMsg('Test rain device', res)
	return res
end

local testRGB = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 20,
		["name"] = name,
		["maxDimLevel"] = 100,
		["state"] = "On",
		["deviceSubType"] = "RGB";
		["deviceType"] = "Lighting Limitless/Applamp";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.dimTo(15)
	tstMsg('Test rgb device', res)
	return res
end

local testRGBW = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 21,
		["name"] = name,
		["maxDimLevel"] = 100,
		["state"] = "On",
		["deviceSubType"] = "RGBW";
		["deviceType"] = "Lighting Limitless/Applamp";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.dimTo(15)
	tstMsg('Test rgbw device', res)
	return res
end

local testScaleWeight = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 22,
		["name"] = name,
		['weight'] = 0,
		["deviceSubType"] = "BWR102";
		["deviceType"] = "Weight";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})
	dev.updateWeight(33.5)
	tstMsg('Test scale weight device', res)
	return res
end

local testSelectorSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 23,
		["name"] = name,
		['levelName'] = 'Off',
		['level'] = 0,
		["state"] = 'Off',
		["deviceSubType"] = "Selector Switch";
		["deviceType"] = "Light/Switch";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	res = res and expectEql('Off',  dev.levelNames[1])
	res = res and expectEql('Level1', dev.levelNames[2])
	res = res and expectEql('Level2', dev.levelNames[3])
	res = res and expectEql('Level3', dev.levelNames[4])

	dev.switchSelector(30) -- level3
	tstMsg('Test selector switch device', res)
	return res
end

local testSoilMoisture = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 24,
		["name"] = name,
		["moisture"] = 3,
		["deviceSubType"] = "Soil Moisture";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateSoilMoisture(34)
	tstMsg('Test soil moisture device', res)
	return res
end

local testSolarRadiation = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 25,
		["name"] = name,
		["radiation"] = 1,
		["deviceSubType"] = "Solar Radiation";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateRadiation(34)
	tstMsg('Test solar radiation device', res)
	return res
end

local testSoundLevel = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 26,
		["name"] = name,
		["level"] = 65,
		["deviceSubType"] = "Sound Level";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateSoundLevel(120)
	tstMsg('Test sound level device', res)
	return res
end

local testTemperature = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 27,
		["name"] = name,
		["temperature"] = 0,
		["deviceSubType"] = "LaCrosse TX3";
		["deviceType"] = "Temp";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateTemperature(120)
	tstMsg('Test temperature device', res)
	return res
end

local testAPITemperature = function(name)
	local dev = dz.devices(name)
	local res = true

	dz.openURL(dz.settings['Domoticz url'] .. '/json.htm?type=command%26param=udevice%26idx=' .. tonumber(42) .. '%26nvalue=0%26svalue=' .. tostring(42))

	tstMsg('Test API temperature device', res)
	return res
end


local testTempHum = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 28,
		["name"] = name,
		["temperature"] = 0,
		["humidity"] = 50,
		["humidityStatus"] = "Comfortable";
		["deviceSubType"] = "THGN122/123, THGN132, THGR122/228/238/268";
		--["deviceType"] = 'Temp + Humidity';
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateTempHum(34, 88, dz.HUM_WET)
	tstMsg('Test temperature+humidity device', res)
	return res
end

local testTempHumBaro = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 29,
		["name"] = name,
		["temperature"] = 0,
		["barometer"] = 1010,
		["forecastString"] = "Sunny";
		["forecast"] = 1;
		["deviceSubType"] = "THB1 - BTHR918, BTHGN129";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateTempHumBaro(34, 88, dz.HUM_WET, 1033, dz.BARO_PARTLYCLOUDY)
	tstMsg('Test temperature+humidity+barometer device', res)
	return res
end

local testTempBaro = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 40,
		["name"] = name,
		["temperature"] = 0,
		["barometer"] = 1038,
		["forecastString"] = "Stable";
		["forecast"] = 0;
		["deviceSubType"] = "BMP085 I2C";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateTempBaro(34, 1033, dz.BARO_CLOUDY)
	tstMsg('Test temperature+barometer device', res)
	return res
end

local testText = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 30,
		["name"] = name,
		["text"] = 'Hello World',
		["deviceSubType"] = "Text";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateText("Oh my Darwin, what a lot of tests!")
	tstMsg('Test text device', res)
	return res
end

local testThermostatSetpoint = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 31,
		["name"] = name,
		["setPoint"] = 20.5,
		["deviceSubType"] = "SetPoint";
		["deviceType"] = "Thermostat";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateSetPoint(22)
	tstMsg('Test thermostat device', res)
	return res
end

local testUsageElectric = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 32,
		["name"] = name,
		["WhActual"] = 0,
		["deviceSubType"] = "Electric";
		["deviceType"] = "Usage";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateEnergy(1922)
	tstMsg('Test usage electric device', res)
	return res
end

local testUV = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 33,
		["name"] = name,
		["uv"] = 0,
		["deviceSubType"] = "UVN128,UV138";
		["deviceType"] = "UV";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateUV(12.33)
	tstMsg('Test uv device', res)
	return res
end

local testVisibility = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and expectEql(103, math.floor(dev.visibility * 10))
	res = res and checkAttributes(dev, {
		["id"] = 34,
		["name"] = name,
		["deviceSubType"] = "Visibility";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateVisibility(1)
	tstMsg('Test visibility device', res)
	return res
end

local testVoltage = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 35,
		["name"] = name,
		["voltage"] = 0,
		["deviceSubType"] = "Voltage";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateVoltage(220)
	tstMsg('Test voltage device', res)
	return res
end

local testWaterflow = function(name)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = 36,
		["name"] = name,
		["flow"] = 0,
		["deviceSubType"] = "Waterflow";
		["deviceType"] = "General";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateWaterflow(15)
	tstMsg('Test waterflow device', res)
	return res
end

local testWind = function(name, id, subType)
	local dev = dz.devices(name)
	local res = true
	res = res and checkAttributes(dev, {
		["id"] = id,
		["name"] = name,
		["gust"] = 0,
		["temperature"] = 0,
		["speed"] = 0,
		["direction"] = 0,
		['directionString'] = "N",
		['chill'] = 0,
		["deviceSubType"] = subType,
		["deviceType"] = "Wind";
		["hardwareType"] = "Dummy (Does nothing, use for virtual switches only)";
		["hardwareName"] = "dummy";
		["hardwareTypeValue"] = 15;
		["hardwareId"] = 2;
		["batteryLevel"] = nil; -- 255 == nil
		["changed"] = false;
		["timedOut"] = false;
	})

	dev.updateWind(120, 'SW', 55, 66, 77, 88)
	tstMsg('Test wind device', res)
	return res
end

local testScene = function(name)
	local scene = dz.scenes(name)
	local res = true
	res = res and checkAttributes(scene, {
		["id"] = 1,
		["name"] = name,
		['state'] = 'Off',
		['baseType'] = 'scene'
	})

	scene.switchOn()
	tstMsg('Test scene', res)
	return res
end

local testGroup = function(name)
	local group = dz.groups(name)
	local res = true

	res = res and checkAttributes(group, {
		["id"] = 2,
		["name"] = name,
		['state'] = 'Off',
		['baseType'] = 'group'
	})

	group.switchOn()
	tstMsg('Test group', res)
	return res
end

local testVariableInt = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		["id"] = 1,
		["name"] = name,
		['value'] = 42
	})

	var.set(43)
	tstMsg('Test variable: int', res)
	return res
end

local testVariableFloat = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		["id"] = 2,
		["name"] = name,
		['value'] = 42.11
	})

	var.set(43)
	tstMsg('Test variable: float', res)
	return res
end

local testVariableString = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		["id"] = 3,
		["name"] = name,
		['value'] = 'Somestring'
	})

	var.set('Zork is a dork').afterSec(3)
	tstMsg('Test variable: string', res)
	return res
end

local testVariableDate = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		["id"] = 4,
		["name"] = name,
	})

	res = res and expectEql(var.date.month, 12)
	res = res and expectEql(var.date.day, 31)
	res = res and expectEql(var.date.year, 2017)
	var.set('20/11/2016');
	tstMsg('Test variable: date', res)
	return res
end

local testVariableTime = function(name)
	local var = dz.variables(name)
	local res = true
	res = res and checkAttributes(var, {
		["id"] = 5,
		["name"] = name,
	})

	res = res and expectEql(var.time.hour, 23)
	res = res and expectEql(var.time.min, 59)
	var.set('09:54');
	tstMsg('Test variable: time', res)
	return res
end

local testSilentSwitch = function(name)
	local dev = dz.devices(name)
	local res = true
	dev.switchOn().silent()
	tstMsg('Test silent switch device', res)
	return res
end

local testSilentVar = function(name)
	local var = dz.variables(name)
	local res = true
	var.set(1).silent();
	tstMsg('Test silent variable', res)
	return res
end

local testSilentScene = function(name)
	local scene = dz.scenes(name)
	local res = true
	scene.switchOn().silent()
	tstMsg('Test silent scene', res)
	return res
end

local testSilentGroup = function(name)
	local group = dz.groups(name)
	local res = true
	group.switchOn().silent()
	tstMsg('Test silent group', res)
	return res
end


local storeLastUpdates = function()

	dz.globalData.stage1Time = dz.time.raw

end

local testLastUpdates = function()

	local Time = require('Time')
	local results = true
	local now = dz.time.secondsSinceMidnight

	results = dz.devices().reduce(function(acc, device)

		if (device.name ~= 'endResult' and device.name ~= 'stage1Trigger' and device.name ~= 'stage2Trigger') then
			local devTime = device.lastUpdate.secondsSinceMidnight

			local delta = now - device.lastUpdate.secondsSinceMidnight
			--print('now:' .. now .. ' device: ' .. device.lastUpdate.secondsSinceMidnight .. ' delta: ' .. delta)
			-- test if lastUpdate for the device is close to domoticz time
			local ok = (devTime <= now and delta < 5)
			acc = acc and ok
			if (expectEql(true, ok, device.name .. ' lastUpdate is not correctly set') == false) then
				print('now:' .. now .. ' device: ' .. device.lastUpdate.secondsSinceMidnight .. ' delta: ' .. delta)
			end
		end

		return acc
	end, results)

	tstMsg('Test lastUpdates', results)
	return results
end

local testSecurity = function()
	local res = true
	res = res and expectEql(dz.security, dz.SECURITY_DISARMED)
	dz.devices('secPanel').armAway()
	return res
end

local testRepeatSwitch = function(name)
	local dev = dz.devices(name)
	dz.globalData.repeatSwitch.reset()
	dz.globalData.repeatSwitch.add({ state = 'Start', delta = 0 })
	dev.switchOn().afterSec(8).forSec(2).repeatAfterSec(5, 1) -- 17s total
	tstMsg('Test switch device', res)
	return true
end

return {
	active = true,
	on = {
		devices = {'stage1Trigger' }
	},
	execute = function(domoticz, trigger)
		local res = true
		dz = domoticz
		log = dz.log

		log('Starting stage 1')

		res = res and testAirQuality('vdAirQuality')
		res = res and testSwitch('vdSwitch')
		res = res and testAlert('vdAlert')
		res = res and testAmpere3('vdAmpere3')
		res = res and testAmpere1('vdAmpere1')
		res = res and testDimmer('vdSwitchDimmer')
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
		res = res and testWind('vdWind', 37, "WTGR800")
		res = res and testWind('vdWindTempChill', 38, "TFA")
		res = res and testScene('scScene')
		res = res and testGroup('gpGroup')
		res = res and testVariableInt('varInteger')
		res = res and testVariableFloat('varFloat')
		res = res and testVariableString('varString')
		res = res and testVariableDate('varDate')
		res = res and testVariableTime('varTime')
		res = res and testLastUpdates()
		res = res and testSecurity()
		res = res and testSilentSwitch('vdSilentSwitch')
		res = res and testSilentScene('scSilentScene')
		res = res and testSilentGroup('gpSilentGroup')
		res = res and testSilentVar('varSilent')
		res = res and testAPITemperature('vdAPITemperature');
		res = res and testRepeatSwitch('vdRepeatSwitch');

		storeLastUpdates()

		log('Finishing stage 1')
		if (not res) then
			log('Results stage 1: FAILED!!!!', dz.LOG_ERROR)
		else
			log('Results stage 1: SUCCEEDED')
		end

		dz.devices('stage2Trigger').switchOn().afterSec(20)
	end
}
