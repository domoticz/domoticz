_G._ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

local testData = require('tstData')


local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

local function getDevice_(
	domoticz,
	name,
	state,
	changed,
	type,
	subType,
	rawData,
	additionalRootData,
	additionalDataData,
	hardwareType,
	hardwaryTypeValue)

	local Device = require('Device')

	if (rawData == nil) then
		rawData = {
			[1] = "1",
			[2] = '2',
			[3] = '3'
		}
	end

	if (additionalRootData == nil) then
		additionalRootData = {}
	end

	if (additionalDataData == nil) then
		additionalDataData = {}
	end

	if (hardwareType == nil) then
		hardwareType = 'ht1'
	end
	if (hardwareTypeValue == nil) then
		hardwareTypeValue = 'ht1'
	end

	local data = {
		["id"] = 1,
		["name"] = name,
		["description"] = "Description 1",
		["batteryLevel"] = 10,
		["signalLevel"] = '10',
		["deviceType"] = type and type or "someSubType",
		["subType"] = subType and subType or "someDeviceType",
		["hardwareName"] = "hw1",
		["hardwareType"] = hardwareType,
		["hardwareTypeID"] = 0,
		["hardwareTypeValue"] = hardwaryTypeValue,
		["hardwareID"] = 1,
		["timedOut"] = true,
		["switchType"] = "Contact",
		["switchTypeValue"] = 2,
		["lastUpdate"] = "2016-03-20 12:23:00",
		["data"] = {["_state"] = state,},
		["deviceID"] = "",
		["rawData"] = rawData,
		["baseType"] = "device",
		["changed"] = changed,
		["changedAttribute"] = 'temperature' --tbd
	}

	for attribute, value in pairs(additionalRootData) do
		data[attribute] = value
	end

	for attribute, value in pairs(additionalDataData) do
		data.data[attribute] = value
	end

	return Device(domoticz, data)
end

local function getDevice(domoticz, options)

	return getDevice_(domoticz,
		options.name,
		options.state,
		options.change,
		options.type,
		options.subType,
		options.rawData,
		options.additionalRootData,
		options.additionalDataData,
		options.hardwareType,
		options.hardwareTypeValue)

end

describe('device', function()
	local Device
	local commandArray = {}
	local cmd
	local device

	local domoticz = {
		settings = {
			['Domoticz url'] = 'http://127.0.0.1:8080',
			['Log level'] = 2
		},
		['radixSeparator'] = '.',
		sendCommand = function(command, value)
			table.insert(commandArray, {[command] = value})
			return commandArray[#commandArray], command, value
		end
	}

	setup(function()
		_G.logLevel = 1
		_G.TESTMODE = true
		_G.globalvariables = {
			Security = 'sec',
			['radix_separator'] = '.',
			['script_reason'] = 'device',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080'
		}

		Device = require('Device')

	end)

	teardown(function()
		Device = nil
	end)

	before_each(function()
		device = getDevice_(domoticz, 'myDevice', 'On', true)
		device.id = 100
		utils = device._getUtilsInstance()
		utils.print = function()  end
	end)

	after_each(function()
		device = nil
		commandArray = {}

	end)

	describe('Adapters', function()

		it('should detect a lux device', function()
			local device = getDevice_(domoticz, 'myDevice', nil, true, 'Lux', 'Lux')
			assert.is_same(1, device.lux)

			device.updateLux(333)
			assert.is_same({ { ["UpdateDevice"] = "1|0|333" } }, commandArray)

		end)

		it('should detect a zone heating device', function()
			local rawData = {
				[1] = 1,
				[2] = '12.5',
				[3] = 'Cozy'
			}
			local device = getDevice_(domoticz, 'myDevice', nil, true, 'Heating', 'Zone', rawData)
			assert.is_same(12.5, device.setPoint)
			assert.is_same('Cozy', device.heatingMode)

		end)

		it('should detect a kwh device', function()

			local data = {
				['counterToday'] = '1.234 kWh'
			}

			local device = getDevice_(domoticz, 'myDevice', nil, true, 'General', 'kWh', nil, nil, data)

			assert.is_same(1234, device.WhToday)

		end)

		it('should detect an electric usage device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type']= 'Usage',
				['subType'] = 'Electric',
				['rawData'] = { [1] = 12345 }
			})

			assert.is_same(12345, device.WhActual)
		end)

		it('should detect a p1 smart meter device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'P1 Smart Meter',
				['subType'] = 'Energy',
				['rawData'] = { [5] = 12345 }
			})

			assert.is_same(12345, device.WhActual)
		end)

		it('should detect a thermostat setpoint device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Thermostat',
				['hardwareTypeValue'] = 15,
				['subType'] = 'SetPoint',
				['rawData'] = { [1] = 12.5 }
			})

			assert.is_same(12.5, device.SetPoint)

			local res;

			domoticz.openURL = function(url)
				res = url;
			end

			device.updateSetPoint(14)

			assert.is_same('http://127.0.0.1:8080/json.htm?type=command&param=udevice&idx=1&nvalue=0&svalue=14', res)

		end)

		it('should detect a text device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Thermostat',
				['subType'] = 'Text',
				['rawData'] = { [1] = 'dzVents rocks' }
			})

			assert.is_same('dzVents rocks', device.text)
			device.updateText('foo')
			assert.is_same({ { ["UpdateDevice"] = "1|0|foo" } }, commandArray)

		end)

		it('should detect a rain device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Rain',
			})

			device.updateRain(10, 20)
			assert.is_same({ { ["UpdateDevice"] = "1|0|10;20" } }, commandArray)
		end)

		it('should detect an air quality device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Air Quality',
			})

			device.updateAirQuality(44)
			assert.is_same({ { ["UpdateDevice"] = "1|44" } }, commandArray)
		end)

		it('should detect an evohome device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Thermostat',
				['subType'] = 'Zone',
				['hardwareTypeValue'] = 39,
				['rawData'] = { [1] = 12.5 }
			})

			local res;
			domoticz.openURL = function(url)
				res = url;
			end

			assert.is_same(12.5, device.SetPoint)

			device.updateSetPoint(14, 'Permanent', '2016-04-29T06:32:58Z')

			assert.is_same('http://127.0.0.1:8080/json.htm?type=setused&idx=1&setpoint=14&mode=Permanent&used=true&until=2016-04-29T06:32:58Z', res)
		end)

		it('should detect an opentherm gateway device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Thermostat',
				['hardwareTypeValue'] = 20,
				['subType'] = 'SetPoint',
				['rawData'] = { [1] = 12.5 }
			})

			local res;

			domoticz.openURL = function(url)
				res = url;
			end

			assert.is_same(12.5, device.SetPoint)

			device.updateSetPoint(14)

			assert.is_same('http://127.0.0.1:8080/json.htm?type=command&param=udevice&idx=1&nvalue=0&svalue=14', res)
		end)

		it('should detect a wind device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Wind',
				['rawData'] = {
					[1] = "243";
					[2] = "SE";
					[3] = "660";
					[4] = "120";
					[5] = "33";
					[6] = "32";
				}
			})
			assert.is.same(12, device.gust)
			assert.is.same(33, device.temperature)
			assert.is.same(32, device.chill)

			device.updateWind(1, 2, 3, 4, 5, 6)
			assert.is_same({ { ["UpdateDevice"] = "1|0|1;2;30;40;5;6" } }, commandArray)

		end)

		it('should detect a uv device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'UV',
				['rawData'] = {
					[1] = "123.55";
					[2] = "0";
				}
			})
			assert.is.same(123.55, device.uv)

			device.updateUV(33.5)
			assert.is_same({ { ["UpdateDevice"] = "1|0|33.5;0" } }, commandArray)
		end)

		it('should detect a barometer device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Barometer',
			})

			device.updateBarometer(1024, 'Purple rain soon')
			assert.is_same({ { ["UpdateDevice"] = "1|0|1024;Purple rain soon" } }, commandArray)
		end)

		it('should detect a temperature device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp',
			})

			device.updateTemperature(23)
			assert.is_same({ { ["UpdateDevice"] = "1|0|23" } }, commandArray)
		end)

		it('should detect a humidity device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Humidity',
			})

			device.updateHumidity(66, 'wet')
			assert.is_same({ { ["UpdateDevice"] = "1|66|wet" } }, commandArray)
		end)

		it('should detect a temp+humidity device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp + Humidity',
			})

			device.updateTempHum(10, 12, 'wet')
			assert.is_same({ { ["UpdateDevice"] = "1|0|10;12;wet" } }, commandArray)
		end)

		it('should detect a temp+humidity+barometer device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp + Humidity + Baro'
			})

			device.updateTempHumBaro(10, 12, 'wet', 1000, 'thunder')
			assert.is_same({ { ["UpdateDevice"] = '1|0|10;12;wet;1000;thunder' } }, commandArray)
		end)


		describe('Kodi', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'bla',
				['hardwareType'] = 'Kodi Media Server'
			})

			it('should switchOff', function()
				device.kodiSwitchOff()
				assert.is_same({ { ['myDevice'] = 'Off' } }, commandArray)
			end)

			it('should stop', function()
				device.kodiStop()
				assert.is_same({ { ['myDevice'] = 'Stop' } }, commandArray)
			end)

			it('should play', function()
				device.kodiPlay()
				assert.is_same({ { ['myDevice'] = 'Play' } }, commandArray)
			end)

			it('should pause', function()
				device.kodiPause()
				assert.is_same({ { ['myDevice'] = 'Pause' } }, commandArray)
			end)

			it('should set volume', function()
				device.kodiSetVolume(22)
				assert.is_same({ { ['myDevice'] = 'Set Volume 22' } }, commandArray)
			end)

			it('should not set volume if not in range', function()
				local msg, tp
				local utils = device._getUtilsInstance()

				utils.log = function(message, type)
					msg = message
					tp = type
				end
				device.kodiSetVolume(101)

				assert.is_same({}, commandArray)
				assert.is_same('Volume must be between 0 and 100. Value = 101', msg)
				assert.is_same(LOG_ERROR, tp)

				tp = ''
				msg = ''

				device.kodiSetVolume(-1)
				assert.is_same({}, commandArray)
				assert.is_same('Volume must be between 0 and 100. Value = -1', msg)
				assert.is_same(LOG_ERROR, tp)
			end)

			it('should play a playlist', function()
				device.kodiPlayPlaylist('daList', 12)
				assert.is_same({ { ['myDevice'] = 'Play Playlist daList 12' } }, commandArray)

				commandArray = {}
				device.kodiPlayPlaylist('daList')
				assert.is_same({ { ['myDevice'] = 'Play Playlist daList 0' } }, commandArray)
			end)

			it('should play favorites', function()
				device.kodiPlayFavorites(12)
				assert.is_same({ { ['myDevice'] = 'Play Favorites 12' } }, commandArray)

				commandArray = {}

				device.kodiPlayFavorites()
				assert.is_same({ { ['myDevice'] = 'Play Favorites 0' } }, commandArray)
			end)

			it('should execute an addon', function()
				device.kodiExecuteAddOn('daAddOn')
				assert.is_same({ { ['myDevice'] = 'Execute daAddOn' } }, commandArray)
			end)
		end)

	end)

	it('should instantiate', function()
		assert.is_not_nil(device)
	end)

	it('should have a name', function()
		assert.is_same('myDevice', device.name)
	end)

	it('should be marked as changed when the device was changed', function()
		assert.is_true(device.changed)
	end)

	it('should not be marked as changed when the device was not changed', function()
		local device = getDevice_(domoticz, 'myDevice', 'On', false)
		assert.is_false(device.changed)
	end)

	it('should not have a state when it doesnt have one', function()
		local device = getDevice_(domoticz, 'myDevice', nil, false)
		assert.is_nil(device.state)
	end)

	it('should extract level', function()
		local device = getDevice_(domoticz, 'myDevice', 'Set Level 55%', false)
		assert.is_same('On', device.state)
		assert.is_number(device.level)
		assert.is_same(55, device.level)
	end)

	it('should have a bState when possible', function()
		local device = getDevice_(domoticz, 'myDevice', 'Set Level 55%', false)
		assert.is_true(device.bState)

		device = getDevice_(domoticz, 'myDevice', '', false)
		assert.is_false(device.bState)


		device = getDevice_(domoticz, 'myDevice', 'On', false)
		assert.is_true(device.bState)

		local states = device._States

		_.forEach(states, function(value, key)
			--			print(key, value)
			device = getDevice_(domoticz, 'myDevice', key, false)
			if (value.b) then
				assert.is_true(device.bState)
			else
				assert.is_false(device.bState)
			end
		end)

	end)

	it('should set the state', function()
		local cmd = device.setState('Off')
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Off"}, cmd._latest)
	end)

	it('should switch on', function()
		device = getDevice_(domoticz, 'myDevice', 'Off', false)

		local cmd = device.switchOn()
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="On"}, cmd._latest)
	end)

	it('should switch off', function()
		device = getDevice_(domoticz, 'myDevice', 'On', false)

		local cmd = device.switchOff()
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Off"}, cmd._latest)
	end)

	it('should close', function()
		device = getDevice_(domoticz, 'myDevice', 'Open', false)

		local cmd = device.close()
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Closed"}, cmd._latest)
	end)

	it('should open', function()
		device = getDevice_(domoticz, 'myDevice', 'Closed', false)

		local cmd = device.open()
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Open"}, cmd._latest)
	end)

	it('should stop', function()
		device = getDevice_(domoticz, 'myDevice', 'Closed', false)

		local cmd = device.stop()
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Stop"}, cmd._latest)
	end)

	it('should dim to a level', function()
		device = getDevice_(domoticz, 'myDevice', 'On', false)

		local cmd = device.dimTo(5)
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Set Level 5"}, cmd._latest)
	end)

	it('should switch a selector', function()
		device = getDevice_(domoticz, 'myDevice', 'On', false)

		local cmd = device.switchSelector(15)
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Set Level 15"}, cmd._latest)
	end)

	describe('Updating', function()
		it('should send generic update commands', function()
			device.update(1,2,3,4,5)
			assert.is_same({{["UpdateDevice"]="100|1|2|3|4|5"}}, commandArray)
		end)

		it('should update temperature and humidity and barometer', function()
			device.updateTempHumBaro(10, 20, 2, 5,7)
			assert.is_same({{["UpdateDevice"]="100|0|10;20;2;5;7"}}, commandArray)
		end)

		it('should update counter', function()
			device.updateCounter(22)
			assert.is_same({{["UpdateDevice"]="100|0|22"}}, commandArray)
		end)

		it('should update electricity', function()
			device.updateElectricity(220, 1000)
			assert.is_same({{["UpdateDevice"]="100|0|220;1000"}}, commandArray)
		end)

		it('should update P1', function()
			device.updateP1(1,2,3,4,5,6)
			assert.is_same({{["UpdateDevice"]="100|0|1;2;3;4;5;6"}}, commandArray)
		end)

		it('should update pressure', function()
			device.updatePressure(1200)
			assert.is_same({{["UpdateDevice"]="100|0|1200"}}, commandArray)
		end)

		it('should update percentage', function()
			device.updatePercentage(88)
			assert.is_same({{["UpdateDevice"]="100|0|88"}}, commandArray)
		end)

		it('should update gas', function()
			device.updateGas(880)
			assert.is_same({{["UpdateDevice"]="100|0|880"}}, commandArray)
		end)

		it('should update voltage', function()
			device.updateVoltage(120)
			assert.is_same({{["UpdateDevice"]="100|0|120"}}, commandArray)
		end)

		it('should update alert sensor', function()
			device.updateAlertSensor(45, 'o dear')
			assert.is_same({{["UpdateDevice"]="100|45|o dear"}}, commandArray)
		end)

		it('should update distance', function()
			device.updateDistance(67)
			assert.is_same({{["UpdateDevice"]="100|0|67"}}, commandArray)
		end)

		it('should update a custom sensor', function()
			device.updateCustomSensor(67)
			assert.is_same({ { ["UpdateDevice"] = "100|0|67" } }, commandArray)
		end)


	end)

	it('should mark an attribute as changed', function()
		assert.is_true(device.attributeChanged('temperature'))
	end)


	it('should toggle a switch', function()
		local cmd = device.toggleSwitch()
		assert.is_same({["myDevice"]="Off"}, cmd._latest)

		device = getDevice_(domoticz, 'myDevice', 'Off', false)
		cmd = device.toggleSwitch()
		assert.is_same({["myDevice"]="On"}, cmd._latest)

		device = getDevice_(domoticz, 'myDevice', 'Playing', false)
		cmd = device.toggleSwitch()
		assert.is_same({["myDevice"]="Pause"}, cmd._latest)

		device = getDevice_(domoticz, 'myDevice', 'Chime', false)
		cmd = device.toggleSwitch() -- shouldn't do anything
		assert.is_nil(cmd)

		device = getDevice_(domoticz, 'myDevice', 'All On', false)
		cmd = device.toggleSwitch()
		assert.is_same({["myDevice"]="All Off"}, cmd._latest)

	end)
end)
