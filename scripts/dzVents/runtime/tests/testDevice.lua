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
	dummyLogger)

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
		["batteryLevel"] = 128,
		["signalLevel"] = 132,
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
		["data"] = {["_state"] = state, ['_nValue'] = 123},
		["deviceID"] = "",
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
		options.dummyLogger)

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
			return TimedCommand(_d, 'Group:' .. group, value)
		end,
		setScene = function(scene, value)
			return TimedCommand(_d, 'Scene:' .. scene, value)
		end,
		sendCommand = function(command, value)
			table.insert(commandArray, {[command] = value})
			return commandArray[#commandArray], command, value
		end,
		openURL = function(url)
			_.print('url') --                    TODO - >> REMOVE << -
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
			['domoticz_listening_port'] = '8080'
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
			assert.is_same(128, device.batteryLevel)
			assert.is_same(50, device.batteryPercentage)
			assert.is_same(132, device.signalLevel)
			assert.is_same(51, device.signalPercentage)
			assert.is_same('sub', device.deviceSubType)
			assert.is_same(2016, device.lastUpdate.year)
			assert.is_same({'1', '2', '3'}, device.rawData)
			assert.is_same(123, device.nValue)

			assert.is_not_nil(device.setState)
			assert.is_same('bla', device.state)

		end)

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
			assert.is_same({ { ["UpdateDevice"] = "1|0|220;1000" } }, commandArray)
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
			assert.is_same({ { ["UpdateDevice"] = '1|0|1;2;3;4;5;6' } }, commandArray)


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
				['additionalDataData'] = {
					['_nValue'] = 12
				}
			})

			local res
			domoticz.openURL = function(url)
				res = url;
			end

			assert.is_same(12, device.co2)

			device.updateAirQuality(44)
			assert.is_same('http://127.0.0.1:8080/json.htm?type=command&param=udevice&idx=1&nvalue=44', res)
		end)

		it('should detect a security device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['type'] = 'Security',
				['subType'] = 'Security Panel'
			})

			device.disarm()

			assert.is_same({ { ['myDevice'] = 'Disarm' } }, commandArray)

			commandArray = {}

			device.armAway()

			assert.is_same({ { ['myDevice'] = 'Arm Away' } }, commandArray)

			commandArray = {}

			device.armHome()

			assert.is_same({ { ['myDevice'] = 'Arm Home' } }, commandArray)

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
					"updateP1",
					"updatePercentage",
					"updatePressure",
					"updateRadiation",
					"updateRain",
					"updateSetPoint",
					"updateTempHum",
					"updateTempHumBaro",
					"updateTemperature",
					"updateText",
					"updateUV",
					"updateVoltage",
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

			device.updateBarometer(1024, 'thunderstorm')
			assert.is_same({ { ["UpdateDevice"] = "1|0|1024;4" } }, commandArray)
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

			device.updateTempHumBaro(10, 12, 'wet', 1000, 'rain')
			assert.is_same({ { ["UpdateDevice"] = '1|0|10;12;wet;1000;4' } }, commandArray)
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
			assert.is_same({ { ["UpdateDevice"] = '1|0|555' } }, commandArray)

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
			assert.is_same({ { ["UpdateDevice"] = '1|0|555' } }, commandArray)
		end)

		it('should detect a pressure device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Pressure'
			})

			device.updatePressure(567)
			assert.is_same({ { ["UpdateDevice"] = "1|0|567" } }, commandArray)
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
			assert.is_same({ { ["UpdateDevice"] = "1|0|567" } }, commandArray)
		end)

		it('should detect a percentage device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Percentage',
				['rawData'] = { [1] = 99.98 }
			})

			assert.is_same( 99.98, device.percentage )
			device.updatePercentage(12.55)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12.55" } }, commandArray)
		end)

		it('should detect a voltage device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Voltage',
				['additionalDataData'] = { ['voltage'] = 230.12 }
			})

			device.updateVoltage(123.55)
			assert.is_same({ { ["UpdateDevice"] = "1|0|123.55" } }, commandArray)
		end)

		it('should detect an alert device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Alert'
			})

			device.updateAlertSensor(0, 'Oh dear!')
			assert.is_same({ { ["UpdateDevice"] = "1|0|Oh dear!" } }, commandArray)
		end)

		it('should detect a distance device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Distance',
				['rawData'] = { [1]="44.33" }
			})

			assert.is_same(44.33, device.distance)

			device.updateDistance(3.14)
			assert.is_same({ { ["UpdateDevice"] = "1|0|3.14" } }, commandArray)
		end)

		it('should detect a custom sensor device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Custom Sensor'
			})

			device.updateCustomSensor(12)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12" } }, commandArray)
		end)

		it('should detect a solar radiation device', function()
			local device = getDevice(domoticz, {
				['name'] = 'myDevice',
				['subType'] = 'Solar Radiation'
			})

			device.updateRadiation(12)
			assert.is_same({ { ["UpdateDevice"] = "1|0|12" } }, commandArray)
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
						['state'] = 'On',
						['additionalRootData'] = { ['switchType'] = 'Selector'},
						['additionalDataData'] = {
							levelName = "aa|bb|cc"
						}
				})

				assert.is_same({'aa', 'bb', 'cc'},  _.values(switch.levelNames))

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

		it('should detect a group', function()
			local group = getDevice(domoticz, {
				['baseType'] = 'group',
				['name'] = 'myGroup',
				['state'] = 'On'
			})

			group.switchOn()
			assert.is_same({ { ['Group:myGroup'] = 'On' } }, commandArray)

			commandArray = {}
			group.toggleGroup()
			assert.is_same({ { ['Group:myGroup'] = 'Off' } }, commandArray)

			commandArray = {}
			group.switchOff()
			assert.is_same({ { ['Group:myGroup'] = 'Off' } }, commandArray)
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

	describe('Updating', function()
		it('should send generic update commands', function()
			device.update(1,2,3,4,5)
			assert.is_same({{["UpdateDevice"]="1|1|2|3|4|5"}}, commandArray)
		end)


	end)


end)
