_G._ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

local testData = require('tstData')

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

local utils = require('Utils')
local function values(t)
	local values = _.values(t)
	table.sort(values)
	return values
end
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
	hardwaryTypeValue,
	baseType,
	dummyLogger,
	batteryLevel,
	signalLevel)

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
		["batteryLevel"] = batteryLevel and batteryLevel or 50,
		["signalLevel"] = signalLevel and signalLevel or 55,
		["deviceType"] = type and type or "someSubType",
		["deviceID"] = "123abc",
		["subType"] = subType and subType or "someDeviceType",
		["timedOut"] = true,
		["switchType"] = "Contact",
		["switchTypeValue"] = 2,
		["lastUpdate"] = "2016-03-20 12:23:00",
		["data"] = {
			["_state"] = state,
			["hardwareName"] = "hw1",
			["hardwareType"] = hardwareType,
			["hardwareTypeValue"] = hardwaryTypeValue,
			["hardwareID"] = 1,
			['_nValue'] = 123,
			['unit'] = 1
		},
		["rawData"] = rawData,
		["baseType"] = baseType ~= nil and baseType or "device",
		["changed"] = changed,
		["changedAttribute"] = 'temperature' --tbd
	}

	for attribute, value in pairs(additionalRootData) do
		data[attribute] = value
	end

	for attribute, value in pairs(additionalDataData) do
		data.data[attribute] = value
	end

	return Device(domoticz, data, dummyLogger)
end

local function getDevice(domoticz, options)

	return getDevice_(domoticz,
		options.name,
		options.state,
		options.changed,
		options.type,
		options.subType,
		options.rawData,
		options.additionalRootData,
		options.additionalDataData,
		options.hardwareType,
		options.hardwareTypeValue,
		options.baseType,
		options.dummyLogger,
		options.batteryLevel,
		options.signalLevel)

end

describe('device', function()
	local Device
	local commandArray = {}
	local cmd
	local device
	local TimedCommand
	local _d
	local domoticz = {
		settings = {
			['Domoticz url'] = 'http://127.0.0.1:8080',
			['Log level'] = 2
		},
		['radixSeparator'] = '.',
		switchGroup = function(group, value)
			return TimedCommand(_d, 'Group:' .. group, value, 'device')
		end,
		setScene = function(scene, value)
			return TimedCommand(_d, 'Scene:' .. scene, value, 'device')
		end,
		sendCommand = function(command, value)
			table.insert(commandArray, {[command] = value})
			return commandArray[#commandArray], command, value
		end,
		openURL = function(url)
			return table.insert(commandArray, url)
		end
	}
	_d = domoticz

	setup(function()
		_G.logLevel = 1
		_G.TESTMODE = true
		_G.globalvariables = {
			Security = 'sec',
			['radix_separator'] = '.',
			['script_reason'] = 'device',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080',
			['currentTime'] = '2017-08-17 12:13:14.123'
		}

		TimedCommand = require('TimedCommand')

		Device = require('Device')

	end)

	teardown(function()
		Device = nil
	end)

	before_each(function()
		device = getDevice(domoticz, {
			['name'] = 'myDevice',
			['type'] = 'Light/Switch',
			['state'] = 'On',
			['changed'] = true,
		})
		utils = device._getUtilsInstance()
		utils.print = function()  end
	end)

	after_each(function()
		device = nil
		commandArray = {}

	end)

	describe('Adapters', function()

		it('should apply the generic device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				changed = true,
				type = 'sometype',
				subType = 'sub',
				hardwareType = 'hwtype',
				hardwareTypeValue = 'hvalue',
				state = 'bla'
			})

			assert.is_same(true, device.changed)
			assert.is_same('Description 1', device.description)
			assert.is_same('sometype', device.deviceType)
			assert.is_same('hw1', device.hardwareName)
			assert.is_same('hwtype', device.hardwareType)
			assert.is_same(1, device.hardwareId)
			assert.is_same('hvalue', device.hardwareTypeValue)
			assert.is_same('Contact', device.switchType)
			assert.is_same(2, device.switchTypeValue)
			assert.is_same(true, device.timedOut)
			assert.is_same(50, device.batteryLevel)
			assert.is_same(55, device.signalLevel)
			assert.is_same('sub', device.deviceSubType)
			assert.is_same(2016, device.lastUpdate.year)
			assert.is_same({'1', '2', '3'}, device.rawData)
			assert.is_same(123, device.nValue)
			assert.is_same('123abc', device.deviceId)
			assert.is_same(1, device.idx)

			assert.is_not_nil(device.setState)
			assert.is_same('bla', device.state)

		end)

		it('should deal with percentages', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				changed = true,
				type = 'sometype',
				subType = 'sub',
				hardwareType = 'hwtype',
				hardwareTypeValue = 'hvalue',
				state = 'bla',
				batteryLevel = 200,
				signalLevel = 200
			})

			assert.is_nil(device.batteryLevel)
			assert.is_nil(device.signalLevel)

			device = getDevice(domoticz, {
				['name'] = 'myDevice',
				changed = true,
				type = 'sometype',
				subType = 'sub',
				hardwareType = 'hwtype',
				hardwareTypeValue = 'hvalue',
				state = 'bla',
				batteryLevel = 100,
				signalLevel = 100
			})

			assert.is_same(100, device.batteryLevel)
			assert.is_same(100, device.signalLevel)

			device = getDevice(domoticz, {
				['name'] = 'myDevice',
				changed = true,
				type = 'sometype',
				subType = 'sub',
				hardwareType = 'hwtype',
				hardwareTypeValue = 'hvalue',
				state = 'bla',
				batteryLevel = 0,
				signalLevel = 0
			})

			assert.is_same(0, device.batteryLevel)
			assert.is_same(0, device.signalLevel)
		end)

		it('should detect a lux device', function()
			local device = getDevice_(domoticz, 'myDevice', nil, true, 'Lux', 'Lux')
			assert.is_same(1, device.lux)

			device.updateLux(333)
			assert.is_same({ { ["UpdateDevice"] = "1|0|333 TRIGGER" } }, commandArray)

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

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'kWh',
				['additionalDataData'] = {
					['counterToday'] = '1.234 kWh',
					['usage'] = '654.44 Watt'
				}
			})

			assert.is_same(1234, device.WhToday)
			assert.is_same(1.234, device['counterToday'])
			assert.is_same(654.44, device['usage'])

			device.updateElectricity(220, 1000)
			assert.is_same({ { ["UpdateDevice"] = "1|0|220;1000 TRIGGER" } }, commandArray)
		end)

		it('should detect an electric usage device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type']= 'Usage',
				['subType'] = 'Electric',
				['rawData'] = { [1] = 12345 }
			})

			assert.is_same(12345, device.WhActual)
			device.updateEnergy(1000)
			assert.is_same({ { ["UpdateDevice"] = "1|0|1000 TRIGGER" } }, commandArray)
		end)

		it('should detect a visibility device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'General',
				['subType'] = 'Visibility',
				['additionalDataData'] = {
					['visibility'] = 33
				}
			})

			assert.is_same(33, device.visibility)
			device.updateVisibility(22)
			assert.is_same({ { ["UpdateDevice"] = "1|0|22 TRIGGER" } }, commandArray)
		end)

		it('should detect a p1 smart meter device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'P1 Smart Meter',
				['subType'] = 'Energy',
				['rawData'] = {
					[1] = "1",
					[2] = "2",
					[3] = "10",
					[4] = "20",
					[5] = "12345",
					[6] = "280"
				},
				['additionalDataData'] = {
					["counterDelivered"] = 0.03,
					["usage"] = "840 Watt",
					["usageDelivered"] = "280 Watt",
					["counterToday"] = "5.6780 kWh",
					["counter"] = "1.003",
					["counterDeliveredToday"] = "5.789 kWh",
				}
			})

			assert.is_same(1, device.usage1)
			assert.is_same(2, device.usage2)
			assert.is_same(10, device.return1)
			assert.is_same(20, device.return2)
			assert.is_same(12345, device.usage)
			assert.is_same(280, device.usageDelivered)
			assert.is_same(5.789, device.counterDeliveredToday)
			assert.is_same(5.6780, device.counterToday)
			assert.is_same(12345, device.WhActual)

			device.updateP1(1, 2, 3, 4, 5, 6)
			assert.is_same({ { ["UpdateDevice"] = '1|0|1;2;3;4;5;6 TRIGGER' } }, commandArray)


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

			assert.is_same('http://127.0.0.1:8080/json.htm?type=command&param=setsetpoint&idx=1&setpoint=14', res)

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
			assert.is_same({ { ["UpdateDevice"] = "1|0|foo TRIGGER" } }, commandArray)

		end)

		it('should detect a rain device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Rain',
			})

			device.updateRain(10, 20)
			assert.is_same({ { ["UpdateDevice"] = "1|0|10;20 TRIGGER" } }, commandArray)
		end)

		it('should detect a 2-phase ampere device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'General',
				['subType'] = 'Current',
				['additionalDataData'] = {
					["current"] = 123,
				}
			})

			assert.is_same(123, device.current)
			device.updateCurrent(10)
			assert.is_same({ { ["UpdateDevice"] = "1|0|10 TRIGGER" } }, commandArray)
		end)

		it('should detect a 3-phase ampere device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Current',
				['subType'] = 'CM113, Electrisave',
				['rawData'] = {
					[1] = 123,
					[2] = 456,
					[3] = 789,
				}
			})

			assert.is_same(123, device.current1)
			assert.is_same(456, device.current2)
			assert.is_same(789, device.current3)
			device.updateCurrent(10, 20, 30)
			assert.is_same({ { ["UpdateDevice"] = "1|0|10;20;30 TRIGGER" } }, commandArray)
		end)

		it('should detect an air quality device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Air Quality',
				['additionalDataData'] = {
					['_nValue'] = 12
				}
			})

			assert.is_same(12, device.co2)

			device.updateAirQuality(44)
			assert.is_same({ { ["UpdateDevice"] = "1|44|0 TRIGGER" } }, commandArray)
		end)

		it('should detect a security device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Security',
				['subType'] = 'Security Panel'
			})

			device.disarm().afterSec(2)

			assert.is_same({ { ['myDevice'] = 'Disarm AFTER 2 SECONDS' } }, commandArray)

			commandArray = {}

			device.armAway().afterSec(3)

			assert.is_same({ { ['myDevice'] = 'Arm Away AFTER 3 SECONDS' } }, commandArray)

			commandArray = {}

			device.armHome().afterSec(4)

			assert.is_same({ { ['myDevice'] = 'Arm Home AFTER 4 SECONDS' } }, commandArray)

		end)

		describe('dummy methods', function()

			it('should set dummy methods', function()

				local dummies = {}

				local device = getDevice(domoticz, {
					['name'] = 'myDevice',
					['type'] = 'some crazy type',
					['subType'] = 'some crazy subtype',
					['hardwareType'] = 'some crazy hardware type',
					['dummyLogger' ] = function(device, name)
						table.insert(dummies, name)
					end
				})

				local adapterManager = device.getAdapterManager()

				assert.is_same({
					"armAway",
					"armHome",
					"close",
					"dimTo",
					"disarm",
					"kodiExecuteAddOn",
					"kodiPause",
					"kodiPlay",
					"kodiPlayFavorites",
					"kodiPlayPlaylist",
					"kodiSetVolume",
					"kodiStop",
					"kodiSwitchOff",
					"open",
					"stop",
					"switchOff",
					"switchOn",
					"switchSelector",
					"toggleSwitch",
					"updateAirQuality",
					"updateAlertSensor",
					"updateBarometer",
					"updateCounter",
					"updateCustomSensor",
					"updateDistance",
					"updateElectricity",
					"updateGas",
					"updateHumidity",
					"updateLux",
					"updateMode",
					"updateP1",
					"updatePercentage",
					"updatePressure",
					"updateRadiation",
					"updateRain",
					"updateSetPoint",
					"updateSoilMoisture",
					"updateSoundLevel",
					"updateTempBaro",
					"updateTempHum",
					"updateTempHumBaro",
					"updateTemperature",
					"updateText",
					"updateUV",
					"updateVisibility",
					"updateVoltage",
					"updateWaterflow",
					"updateWeight",
					"updateWetness",
					"updateWind" }, values(dummies))
			end)

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

		it('should detect a Z-Wave Thermostat mode device', function()

			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Thermostat Mode',
				['additionalDataData'] = {
					["modes"] = "0;Off;1;Heat;2;Heat Econ;",
					["mode"] = 2;
				}
			})

			assert.is_same({'Off', 'Heat', 'Heat Econ'}, device.modes)
			assert.is_same(2, device.mode)
			assert.is_same('Heat Econ', device.modeString)


			device.updateMode('Heat')
			assert.is_same({ { ["UpdateDevice"] = "1|1|1 TRIGGER" } }, commandArray)
			commandArray = {}
			device.updateMode('Off')
			assert.is_same({ { ["UpdateDevice"] = "1|0|0 TRIGGER" } }, commandArray)
			commandArray = {}
			device.updateMode('Heat Econ')
			assert.is_same({ { ["UpdateDevice"] = "1|2|2 TRIGGER" } }, commandArray)

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
			assert.is_same({ { ["UpdateDevice"] = "1|0|1;2;30;40;5;6 TRIGGER" } }, commandArray)

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
			assert.is_same({ { ["UpdateDevice"] = "1|0|33.5;0 TRIGGER" } }, commandArray)
		end)

		it('should detect a barometer device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Barometer',
			})

			device.updateBarometer(1024, 'thunderstorm')
			assert.is_same({ { ["UpdateDevice"] = "1|0|1024;4 TRIGGER" } }, commandArray)
		end)

		it('should detect a temperature device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp',
			})

			device.updateTemperature(23)
			assert.is_same({ { ["UpdateDevice"] = "1|0|23 TRIGGER" } }, commandArray)
		end)

		it('should detect a humidity device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Humidity',
				['additionalDataData'] = {
					["humidityStatus"] = "Wet";
				}
			})

			device.updateHumidity(66, 'wet')
			assert.is_same({ { ["UpdateDevice"] = "1|66|wet TRIGGER" } }, commandArray)
			assert.is_same(3, device.humidityStatusValue)
		end)

		it('should detect a temp+humidity device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp + Humidity',
				['additionalDataData'] = {
					["humidityStatus"] = "Dry";
				}
			})

			device.updateTempHum(10, 12, 'wet')
			assert.is_same({ { ["UpdateDevice"] = "1|0|10;12;wet TRIGGER" } }, commandArray)
			assert.is_same(2, device.humidityStatusValue)
		end)

		it('should detect a temp+humidity+barometer device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp + Humidity + Baro',
				['additionalDataData'] = {
					["humidityStatus"] = "Comfortable";
				}
			})

			device.updateTempHumBaro(10, 12, 'wet', 1000, 'rain')
			assert.is_same({ { ["UpdateDevice"] = '1|0|10;12;wet;1000;4 TRIGGER' } }, commandArray)
			assert.is_same(1, device.humidityStatusValue)
		end)

		it('should detect a temp+barometer device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Temp + Baro'
			})

			device.updateTempBaro(10, 1000, 'thunderstorm')
			assert.is_same({ { ["UpdateDevice"] = '1|0|10;1000;4 TRIGGER' } }, commandArray)
		end)


		it('should detect a counter device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'RFXMeter counter',
				['additionalDataData'] = {
					["counterToday"] = "123.44 Watt";
					["counter"] = "6.7894 kWh";
				}
			})
			assert.is_same(123.44, device.counterToday)
			assert.is_same(6.7894, device.counter)

			device.updateCounter(555)
			assert.is_same({ { ["UpdateDevice"] = '1|0|555 TRIGGER' } }, commandArray)

		end)

		it('should detect an incremental counter device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Counter Incremental',
				['additionalDataData'] = {
					["counterToday"] = "123.44 Watt";
					["counter"] = "6.7894 kWh";
				}
			})
			assert.is_same(123.44, device.counterToday)
			assert.is_same(6.7894, device.counter)

			device.updateCounter(555)
			assert.is_same({ { ["UpdateDevice"] = '1|0|555 TRIGGER' } }, commandArray)
		end)

		it('should detect a pressure device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Pressure'
			})

			device.updatePressure(567)
			assert.is_same({ { ["UpdateDevice"] = "1|0|567 TRIGGER" } }, commandArray)
		end)

		it('should detect a gas device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Gas',
				['additionalDataData'] = {
					["counterToday"] = "5.421 m3";
					["counter"] = "123.445";
				}
			})

			assert.is_same(5.421, device.counterToday)
			assert.is_same(123.445, device.counter)

			device.updateGas(567)
			assert.is_same({ { ["UpdateDevice"] = "1|0|567 TRIGGER" } }, commandArray)
		end)

		it('should detect a percentage device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Percentage',
				['rawData'] = { [1] = 99.98 }
			})

			assert.is_same( 99.98, device.percentage )
			device.updatePercentage(12.55)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12.55 TRIGGER" } }, commandArray)
		end)

		it('should detect a voltage device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Voltage',
				['additionalDataData'] = { ['voltage'] = 230.12 }
			})

			device.updateVoltage(123.55)
			assert.is_same({ { ["UpdateDevice"] = "1|0|123.55 TRIGGER" } }, commandArray)
		end)

		it('should detect an alert device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Alert',
				['rawData'] = { [1] = 'some text' },
				['additionalDataData'] = { ['_nValue'] = 4 }
			})

			assert.is_same(4, device.color)
			assert.is_same('some text', device.text)
			device.updateAlertSensor(0, 'Oh dear!')
			assert.is_same({ { ["UpdateDevice"] = "1|0|Oh dear! TRIGGER" } }, commandArray)
		end)

		it('should detect a distance device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Distance',
				['rawData'] = { [1]="44.33" }
			})

			assert.is_same(44.33, device.distance)

			device.updateDistance(3.14)
			assert.is_same({ { ["UpdateDevice"] = "1|0|3.14 TRIGGER" } }, commandArray)
		end)

		it('should detect a custom sensor device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Custom Sensor'
			})

			device.updateCustomSensor(12)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12 TRIGGER" } }, commandArray)
		end)

		it('should detect a solar radiation device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Solar Radiation'
			})

			device.updateRadiation(12)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12 TRIGGER" } }, commandArray)
		end)

		it('should detect a leaf wetness device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Leaf Wetness',
				['additionalDataData'] = { ['_nValue'] = 4 }
			})

			assert.is_same(4, device.wetness)
			device.updateWetness(12)
			assert.is_same({ { ["UpdateDevice"] = "1|12|0 TRIGGER" } }, commandArray)
		end)

		it('should detect a scale weight device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Weight',
				['rawData'] = { [1] = "44" }
			})

			assert.is_same(44, device.weight)
			device.updateWeight(12)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12 TRIGGER" } }, commandArray)
		end)

		it('should detect a sound level device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Sound Level',
				['state'] = '120'
			})

			assert.is_same(120, device.level)
			device.updateSoundLevel(33)
			assert.is_same({ { ["UpdateDevice"] = "1|0|33 TRIGGER" } }, commandArray)
		end)

		it('should detect a waterflow device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Waterflow',
				['rawData'] = { [1] = "44" }
			})

			assert.is_same(44, device.flow)
			device.updateWaterflow(33)
			assert.is_same({ { ["UpdateDevice"] = "1|0|33 TRIGGER" } }, commandArray)
		end)


		it('should detect a soil moisture device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Soil Moisture',
				['additionalDataData'] = { ['_nValue'] = 34 }
			})

			assert.is_same(34, device.moisture)
			device.updateSoilMoisture(12)
			assert.is_same({ { ["UpdateDevice"] = "1|12|0 TRIGGER" } }, commandArray)
		end)


		describe('Switch', function()

			local switch
			local commandArray = {}
			local domoticz = {
				settings = {
					['Domoticz url'] = 'http://127.0.0.1:8080',
					['Log level'] = 2
				},
				['radixSeparator'] = '.',
				sendCommand = function(command, value)
					table.insert(commandArray, { [command] = value })
					return commandArray[#commandArray], command, value
				end
			}

			before_each(function()
				switch = getDevice(domoticz, {
					['type'] = 'Light/Switch',
					['name'] = 's1',
					['state'] = 'On',
				})
			end)

			after_each(function()
				switch = nil
				commandArray = {}
			end)

			it('should toggle the switch', function()
				switch.toggleSwitch()
				assert.is_same({ { ["s1"] = "Off" } }, commandArray)
			end)

			it('should switch on', function()
				switch.switchOn()
				assert.is_same({ { ["s1"] = "On" } }, commandArray)
			end)

			it('should switch off', function()
				switch.switchOff()
				assert.is_same({ { ["s1"] = "Off" } }, commandArray)
			end)

			it('should open', function()
				switch.open()
				assert.is_same({ { ["s1"] = "On" } }, commandArray)
			end)

			it('should close', function()
				switch.close()
				assert.is_same({ { ["s1"] = "Off" } }, commandArray)
			end)

			it('should stop', function()
				switch.stop()
				assert.is_same({ { ["s1"] = "Stop" } }, commandArray)
			end)

			it('should dim', function()
				switch.dimTo(50)
				assert.is_same({ { ["s1"] = "Set Level 50" } }, commandArray)
			end)

			it('should switch a selector', function()
				switch.switchSelector(15)
				assert.is_same({ {["s1"]="Set Level 15"}}, commandArray)
			end)

			it('should detect a selector', function()
					local switch = getDevice(domoticz, {
						['type'] = 'Light/Switch',
						['name'] = 's1',
						['state'] = 'bb',
						['rawData'] = { [1] = "10" },
						['additionalRootData'] = { ['switchType'] = 'Selector'},
						['additionalDataData'] = {
							levelNames = "Off|bb|cc"
						}
				})
				assert.is_same(10, switch.level)
				assert.is_same({'Off', 'bb', 'cc'},  _.values(switch.levelNames))

			end)

		end)

		it('should detect a scene', function()
			local scene = getDevice(domoticz, {
				['baseType'] = 'scene',
				['name'] = 'myScene',
			})

			scene.switchOn()
			assert.is_same({ { ['Scene:myScene'] = 'On' } }, commandArray)

		end)

		it('should detect a huelight', function()
			local device = getDevice(domoticz, {
				['name'] = 'myHue',
				['state'] = 'Set Level: 88%',
				['type'] = 'Lighting 2'
			})

			assert.is_same( 88, device.level)

			device.toggleSwitch()
			assert.is_same({ { ["myHue"] = "Off" } }, commandArray)

			commandArray = {}
			device.switchOn()
			assert.is_same({ { ["myHue"] = "On" } }, commandArray)


			commandArray = {}
			device.switchOff()
			assert.is_same({ { ["myHue"] = "Off" } }, commandArray)
		end)

		it('should detect a group', function()
			local group = getDevice(domoticz, {
				['baseType'] = 'group',
				['name'] = 'myGroup',
				['state'] = 'On'
			})
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
		local device = getDevice_(domoticz, 'myDevice', 'Set Level: 55%', false)
		assert.is_same('On', device.state)
		assert.is_number(device.level)
		assert.is_same(55, device.level)
	end)

	it('should have a bState/active when possible', function()
		local device = getDevice_(domoticz, 'myDevice', 'Set Level: 55%', false)
		assert.is_true(device.bState)
		assert.is_true(device.active)


		device = getDevice_(domoticz, 'myDevice', '', false)
		assert.is_false(device.bState)
		assert.is_false(device.active)


		device = getDevice_(domoticz, 'myDevice', 'On', false)
		assert.is_true(device.bState)
		assert.is_true(device.active)

		local states = device._States

		_.forEach(states, function(value, key)
			device = getDevice_(domoticz, 'myDevice', key, false)
			if (value.b) then
				assert.is_true(device.bState)
				assert.is_true(device.active)
			else
				assert.is_false(device.bState)
				assert.is_true(device.active)
			end
		end)

	end)

	it('should set the state', function()
		local cmd = device.setState('Off')
		assert.is_table(cmd)
		assert.is_same({["myDevice"]="Off"}, cmd._latest)
	end)

	describe('Updating', function()
		it('should send generic update commands', function()
			device.update(1,2,3,4,5)
			assert.is_same({{["UpdateDevice"]="1|1|2|3|4|5 TRIGGER"}}, commandArray)
		end)

		it('should send generic update commands', function()
			device.update(1, 2, 3, 4, 5).silent()
			assert.is_same({ { ["UpdateDevice"] = "1|1|2|3|4|5" } }, commandArray)
		end)


	end)


end)
