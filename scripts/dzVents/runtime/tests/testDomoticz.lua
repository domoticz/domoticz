local _ = require 'lodash'
_G._ = _

local scriptPath = ''

--package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

local testData = require('tstData')

describe('Domoticz', function()
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
			['radix_separator'] = '.',
			['script_reason'] = 'device',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080'
		}

		_G.domoticzData = testData.domoticzData

		settings = {
			['Domoticz url'] = 'http://127.0.0.1:8080',
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
	end)

	describe('properties', function()
		it('should have time properties', function()
			local now = os.date('*t')
			assert.is_same(domoticz.time.isDayTime, 'dt')
			assert.is_same(domoticz.time.isNightTime, 'nt')
			assert.is_same(domoticz.time.sunriseInMinutes, 'sunrisemin')
			assert.is_same(domoticz.time.sunsetInMinutes, 'sunsetmin')
			-- check for basic time props
			assert.is_same(now.hour, domoticz.time.hour)
		end)

		it('should have settings', function()
			assert.is_equal(domoticz.settings, settings)
		end)

		it('should have security info', function()
			assert.is_same('sec', domoticz.security)
		end)

		it('should have priority constants', function()
			assert.is_same(domoticz['PRIORITY_LOW'], -2)
			assert.is_same(domoticz['PRIORITY_MODERATE'], -1)
			assert.is_same(domoticz['PRIORITY_NORMAL'], 0)
			assert.is_same(domoticz['PRIORITY_HIGH'], 1)
			assert.is_same(domoticz['PRIORITY_EMERGENCY'], 2)
		end)

		it('should have sound constants', function()
			assert.is_same(domoticz['SOUND_DEFAULT'], 'pushover')
			assert.is_same(domoticz['SOUND_BIKE'], 'bike')
			assert.is_same(domoticz['SOUND_BUGLE'], 'bugle')
			assert.is_same(domoticz['SOUND_CASH_REGISTER'], 'cashregister')
			assert.is_same(domoticz['SOUND_CLASSICAL'], 'classical')
			assert.is_same(domoticz['SOUND_COSMIC'], 'cosmic')
			assert.is_same(domoticz['SOUND_FALLING'], 'falling')
			assert.is_same(domoticz['SOUND_GAMELAN'], 'gamelan')
			assert.is_same(domoticz['SOUND_INCOMING'], 'incoming')
			assert.is_same(domoticz['SOUND_INTERMISSION'], 'intermission')
			assert.is_same(domoticz['SOUND_MAGIC'], 'magic')
			assert.is_same(domoticz['SOUND_MECHANICAL'], 'mechanical')
			assert.is_same(domoticz['SOUND_PIANOBAR'], 'pianobar')
			assert.is_same(domoticz['SOUND_SIREN'], 'siren')
			assert.is_same(domoticz['SOUND_SPACEALARM'], 'spacealarm')
			assert.is_same(domoticz['SOUND_TUGBOAT'], 'tugboat')
			assert.is_same(domoticz['SOUND_ALIEN'], 'alien')
			assert.is_same(domoticz['SOUND_CLIMB'], 'climb')
			assert.is_same(domoticz['SOUND_PERSISTENT'], 'persistent')
			assert.is_same(domoticz['SOUND_ECHO'], 'echo')
			assert.is_same(domoticz['SOUND_UPDOWN'], 'updown')
			assert.is_same(domoticz['SOUND_NONE'], 'none')
		end)

		it('should have humidity constants', function()
			assert.is_same(domoticz['HUM_NORMAL'], 0)
			assert.is_same(domoticz['HUM_COMFORTABLE'], 1)
			assert.is_same(domoticz['HUM_DRY'], 2)
			assert.is_same(domoticz['HUM_WET'], 3)
		end)

		it('should have barometer constants', function()
			assert.is_same(domoticz['BARO_STABLE'], 'stable')
			assert.is_same(domoticz['BARO_SUNNY'], 'sunny')
			assert.is_same(domoticz['BARO_CLOUDY'], 'cloudy')
			assert.is_same(domoticz['BARO_UNSTABLE'], 'unstable')
			assert.is_same(domoticz['BARO_THUNDERSTORM'], 'thunderstorm')
			assert.is_same(domoticz['BARO_PARTLYCLOUDY'], 'partlycloudy')
			assert.is_same(domoticz['BARO_RAIN'], 'rain')
			assert.is_same(domoticz['BARO_NOINFO'], 'noinfo')

		end)

		it('should have alert level constants', function()
			assert.is_same(domoticz['ALERTLEVEL_GREY'], 0)
			assert.is_same(domoticz['ALERTLEVEL_GREEN'], 1)
			assert.is_same(domoticz['ALERTLEVEL_YELLOW'], 2)
			assert.is_same(domoticz['ALERTLEVEL_ORANGE'], 3)
			assert.is_same(domoticz['ALERTLEVEL_RED'], 4)
		end)

		it('should have security constants', function()
			assert.is_same(domoticz['SECURITY_DISARMED'], 'Disarmed')
			assert.is_same(domoticz['SECURITY_ARMEDAWAY'], 'Armed Away')
			assert.is_same(domoticz['SECURITY_ARMEDHOME'], 'Armed Home')
		end)

		it('should have log constants', function()
			assert.is_same(domoticz['LOG_INFO'], 3)
			assert.is_same(domoticz['LOG_DEBUG'], 4)
			assert.is_same(domoticz['LOG_ERROR'], 1)
			assert.is_same(domoticz['LOG_MODULE_EXEC_INFO'], 2)
		end)
	end)

	describe('commands', function()
		it('should send commands', function()
			local res, command, value = domoticz.sendCommand('do', 'it')
			assert.is_same('do', command)
			assert.is_same('it', value)
			assert.is_same({ ['do'] = 'it' }, res)
		end)

		it('should send multiple commands', function()
			domoticz.sendCommand('do', 'it')
			domoticz.sendCommand('and', 'some more')
			assert.is_same({ { ["do"] = "it" }, { ["and"] = "some more" } }, domoticz.commandArray)
		end)

		it('should return a reference to a commandArray item', function()
			local res = domoticz.sendCommand('do', 'it')
			domoticz.sendCommand('and', 'some more')
			-- now change it
			res['do'] = 'cancel it'
			assert.is_same({ { ["do"] = "cancel it" }, { ["and"] = "some more" } }, domoticz.commandArray)
		end)

		it('should notify', function()
			domoticz.notify('sub', 'mes', 1, 'noise', 'extra', domoticz.NSS_NMA)

			assert.is_same({ { ['SendNotification'] = 'sub#mes#1#noise#extra#nma' } }, domoticz.commandArray)
		end)

		it('should notify with defaults', function()
			domoticz.notify('sub')
			assert.is_same({ { ['SendNotification'] = 'sub##0#pushover##' } }, domoticz.commandArray)
		end)

		it('should notify with multiple subsystems as string', function()
			domoticz.notify('sub', nil, nil, nil, nil, domoticz.NSS_HTTP .. ';' .. domoticz.NSS_PROWL)
			assert.is_same({ { ['SendNotification'] = 'sub##0#pushover##http;prowl' } }, domoticz.commandArray)
		end)

		it('should notify with multiple subsystems as table', function()
			domoticz.notify('sub', nil, nil, nil, nil, { domoticz.NSS_HTTP, domoticz.NSS_PROWL })
			assert.is_same({ { ['SendNotification'] = 'sub##0#pushover##http;prowl' } }, domoticz.commandArray)
		end)

		it('should send email', function()
			domoticz.email('sub', 'mes', 'to@someone')
			assert.is_same({ { ['SendEmail'] = 'sub#mes#to@someone' } }, domoticz.commandArray)
		end)

		it('should send sms', function()
			domoticz.sms('mes')
			assert.is_same({ { ['SendSMS'] = 'mes' } }, domoticz.commandArray)
		end)

		it('should open a url', function()
			domoticz.openURL('some url')
			assert.is_same({ { ['OpenURL'] = 'some url' } }, domoticz.commandArray)
		end)

		it('should set a scene', function()
			local res = domoticz.setScene('scene1', 'on')
			assert.is_table(res)
			assert.is_same({ { ['Scene:scene1'] = 'on' } }, domoticz.commandArray)
		end)

		it('should switch a group', function()
			local res = domoticz.switchGroup('group1', 'on')
			assert.is_table(res)
			assert.is_same({ { ['Group:group1'] = 'on' } }, domoticz.commandArray)
		end)
	end)

	describe('devices', function()

		it('should create devices', function()
			assert.is_not_nil(d1)
			assert.is_not_nil(d2)
			assert.is_not_nil(d3)
			assert.is_not_nil(d4)
		end)

		it('should have set their ids', function()
			assert.is_same(1, d1.id)
			assert.is_same(2, d2.id)
			assert.is_same(3, d3.id)
			assert.is_same(4, d4.id)
		end)

		it('should have set value extentions', function()
			assert.is_same(37, d1.temperature)
			assert.is_same(12, d2.temperature)
			assert.is_same(55, d1.dewpoint)
			assert.is_same(66, d2.dewpoint)
			assert.is_same(66, d1.humidity)
			assert.is_same(67, d2.humidity)
			assert.is_same(333, d4.barometer)
			assert.is_same(123, d4.utility)
			assert.is_same('Nice', d4.weather)
			assert.is_same(666, d4.rain)
			assert.is_same(12, d4.rainLastHour)
			assert.is_same(23, d3.uv)
		end)

		it('should have set last update info', function()
			assert.is_same('2016-03-20 12:23:00', d1.lastUpdate.raw)
			assert.is_same('2016-03-20 12:23:00', d2.lastUpdate.raw)
			assert.is_same('2016-03-20 12:23:00', d3.lastUpdate.raw)
			assert.is_same('2016-03-20 12:23:00', d4.lastUpdate.raw)
		end)

		it('should have set rawData', function()
			assert.is_same({ '1', '2', '3' }, d1.rawData)
			assert.is_same({ '4', '5', '6' }, d2.rawData)
			assert.is_same({ '7', '8', '9', '10', '11' }, d3.rawData)
			assert.is_same({ '10', '11', '12' }, d4.rawData)
		end)

		it('should have created id entries', function()
			assert.is_equal(d1, domoticz.devices[1])
			assert.is_equal(d2, domoticz.devices[2])
			assert.is_equal(d3, domoticz.devices[3])
			assert.is_equal(d4, domoticz.devices[4])
		end)

		it('should have created changedDevices collection', function()
			assert.is_equal(d1, domoticz.changedDevices[1])
			assert.is_equal(d2, domoticz.changedDevices[2])
			assert.is_equal(d1, domoticz.changedDevices['device1'])
			assert.is_equal(d2, domoticz.changedDevices['device2'])
			assert.is_nil(domoticz.changedDevices[3])
			assert.is_nil(domoticz.changedDevices[4])
			assert.is_nil(domoticz.changedDevices['device3'])
			assert.is_nil(domoticz.changedDevices['device4'])
		end)

		it('should have created a text device', function()
			assert.is_same('device8', domoticz.devices['device8'].name)
		end)
	end)

	it('should have created iterators', function()
		assert.is_function(domoticz.devices.forEach)
		assert.is_function(domoticz.devices.filter)
		assert.is_function(domoticz.devices.reduce)
		assert.is_function(domoticz.devices.filter(function()
		end).forEach)
		assert.is_function(domoticz.devices.filter(function()
		end).reduce)

		assert.is_function(domoticz.changedDevices.forEach)
		assert.is_function(domoticz.changedDevices.filter)
		assert.is_function(domoticz.changedDevices.reduce)
		assert.is_function(domoticz.changedDevices.filter(function()
		end).forEach)
		assert.is_function(domoticz.changedDevices.filter(function()
		end).reduce)

		assert.is_function(domoticz.variables.forEach)
		assert.is_function(domoticz.variables.filter)
		assert.is_function(domoticz.variables.reduce)
		assert.is_function(domoticz.variables.filter(function()
		end).forEach)
		assert.is_function(domoticz.variables.filter(function()
		end).reduce)
	end)

	it('should have a working filter and foreach', function()
		local devices = {}
		domoticz.devices.filter(function(d)
			return (d.id == 1 or d.id == 3)
		end).forEach(function(d)
			table.insert(devices, d.id)
		end)

		table.sort(devices)
		assert.is_same({ 1, 3 }, devices)
	end)

	it('should have a working reducer', function()

		local result = domoticz.devices.reduce(function(acc, item)
			return acc + 1
		end, 1)

		assert.is_same(9, result)
	end)

	it('should have a filter that return {} when nothing matches', function()
		local res = domoticz.devices.filter(function(d)
			return false
		end)
		assert.is_same({ 'filter', 'forEach', 'reduce' }, _.keys(res))
	end)

	it('should have created variables', function()
		assert.is_same(1, domoticz.variables['x'].nValue)
		assert.is_same(2.3, domoticz.variables['y'].nValue)
	end)

	it('should have created scenes', function()
		assert.is_same({ 1, 2, 'Scene1', 'Scene2', 'filter', 'forEach', 'reduce' }, _.keys(domoticz.scenes))
		assert.is_same({ 'Scene1', 'Scene2', 'Scene1', 'Scene2' }, _.pluck(domoticz.scenes, { 'name' }))
		assert.is_same({ 'Off', 'Off', 'Off', 'Off' }, _.pluck(domoticz.scenes, { 'state' }))
	end)

	it('should have created groups', function()
		assert.is_same({ 3, 4, 'Group1', 'Group2', 'filter', 'forEach', 'reduce' }, _.keys(domoticz.groups))
		assert.is_same({ 'Group1', 'Group2', 'Group1', 'Group2' }, _.pluck(domoticz.groups, { 'name' }))
		assert.is_same({ 'On', 'Mixed', 'On', 'Mixed' }, _.pluck(domoticz.groups, { 'state' }))
	end)

	it('should log', function()
		local utils = domoticz._getUtilsInstance()
		local logged = false

		utils.log = function(msg, level)
			logged = true
		end

		domoticz.log('boeh', 1)
		assert.is_true(logged)
	end)

--
end)
