local _ = require 'lodash'
_G._ = _

local scriptPath = ''

--package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

local testData = require('tstData')
local function values(t)
	local values = _.values(t)
	table.sort(values)
	return values
end
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
			['domoticz_start_time'] = '2017-08-02 08:30:00',
			['script_reason'] = 'device',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080',
			['currentTime'] = '2017-08-17 12:13:14.123'
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
		d1 = domoticz.devices('device1')
		d2 = domoticz.devices('device2')
		d3 = domoticz.devices('device3')
		d4 = domoticz.devices('device4')
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

		it('should return the start time', function()
			assert.is_same('2017-08-02 08:30:00', domoticz.startTime.raw)
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

	describe('Interacting with the collections', function()

		it('should give you a device when you need one', function()

			assert.is_same('device1', domoticz.devices('device1').name)

			-- should be in the cache
			assert.is_same('device1', domoticz.__devices['device1'].name)
			assert.is_same(1, domoticz.__devices[1].id)

			-- test if next call uses the case
			domoticz.__devices[1].id = 1111
			assert.is_same(1111, domoticz.devices(1).id)

		end)

		it('should return nil if you ask for a non-existing device', function()
			assert.is_nil(domoticz.devices('bla'))
		end)

		it('should return the first device when the name is not unique and log an error', function()
			local utils = domoticz._getUtilsInstance()
			local _log = utils.log
			local logged = false
			local msg
			local level

			utils.log = function(_msg, _level)
				if (_level == 1) then
					msg = _msg
					level = _level
					logged = true
				end
			end

			assert.is_same(9, domoticz.devices('device9').id)
			assert.is_true(logged)
			assert.is_same('Multiple items found for device9 (device). Please make sure your names are unique or use ids instead.', msg)
			assert.is_same(1, level)
			utils.log = _log

		end)

		it('should return iterators when called with no id', function()

			local collection = domoticz.devices()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(device)
				table.insert(res, device.name)
			end)
			assert.is_same({ "device1", "device3", "device7", "device8", "device4", "device9", "device9", "device2", "device5", "device6" }, res)


			local found = collection.find(function(device)
				return device.name == 'device8'
			end)

			assert.is_same(8, found.id)

			found = collection.find(function(device)
				return device.name == 'device8asdfads'
			end)

			assert.is_nil(found)

			local filtered = collection.filter(function(device)
				return device.id < 4
			end)

			local _f = _.pluck(filtered, {'name'})
			assert.is_same(_f, { "device1", "device2", "device3" })

			local res2 = {}
			filtered.forEach(function(device)
				table.insert(res2, device.id)
			end)

			assert.is_same({1,2,3}, res2)

			-- check if the filtered set also has a find function
			found = filtered.find(function(d)
				return d.name == 'device3'
			end)

			assert.is_same(3, found.id)

			local reduced = collection.reduce(function(acc, device)
				acc = acc + device.id

				return acc

			end, 0)

			assert.is_same(55, reduced)

			local reduced2 = filtered.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(6, reduced2)
		end)

		it('should give you a scene when you need one', function()

			assert.is_same('Scene1', domoticz.scenes('Scene1').name)

			-- should be in the cache
			assert.is_same('Scene1', domoticz.__scenes['Scene1'].name)
			assert.is_same(1, domoticz.__scenes[1].id)

			-- test if next call uses the case
			domoticz.__scenes[1].id = 1111
			assert.is_same(1111, domoticz.scenes(1).id)
		end)

		it('should return nil if you ask for a non-existing scene', function()
			assert.is_nil(domoticz.scenes('bla'))
		end)

		it('should return scene iterators when called with no id', function()

			local collection = domoticz.scenes()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(scene)
				table.insert(res, scene.name)
			end)
			assert.is_same({ "Scene1", "Scene2" }, res)


			local filtered = collection.filter(function(scene)
				return scene.id < 2
			end)

			local _f = _.pluck(filtered, { 'name' })
			assert.is_same(_f, { "Scene1" })

			local res2 = {}
			filtered.forEach(function(scene)
				table.insert(res2, scene.id)
			end)

			assert.is_same({ 1 }, res2)


			local reduced = collection.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(3, reduced)

			local reduced2 = filtered.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(1, reduced2)
		end)

		it('should give you a group when you need one', function()

			assert.is_same('Group1', domoticz.groups('Group1').name)

			-- should be in the cache
			assert.is_same('Group1', domoticz.__groups['Group1'].name)
			assert.is_same(3, domoticz.__groups[3].id)

			-- test if next call uses the case
			domoticz.__groups[3].id = 1111
			assert.is_same(1111, domoticz.groups(3).id)
		end)

		it('should return nil if you ask for a non-existing group', function()
			assert.is_nil(domoticz.groups('bla'))
		end)

		it('should return group iterators when called with no id', function()

			local collection = domoticz.groups()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(group)
				table.insert(res, group.name)
			end)
			assert.is_same({ "Group1", "Group2" }, values(res))


			local filtered = collection.filter(function(group)
				return group.id < 4
			end)

			local _f = _.pluck(filtered, { 'name' })
			assert.is_same(_f, { "Group1" })

			local res2 = {}
			filtered.forEach(function(group)
				table.insert(res2, group.id)
			end)

			assert.is_same({ 3 }, res2)


			local reduced = collection.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(7, reduced)

			local reduced2 = filtered.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(3, reduced2)
		end)

		it('should give you a variable when you need one', function()

			assert.is_same('x', domoticz.variables('x').name)

			-- should be in the cache
			assert.is_same('x', domoticz.__variables['x'].name)
			assert.is_same(1, domoticz.__variables[1].id)

			-- test if next call uses the case
			domoticz.__variables[1].id = 1111
			assert.is_same(1111, domoticz.variables(1).id)
		end)

		it('should return nil if you ask for a non-existing variable', function()
			assert.is_nil(domoticz.variables('bla'))
		end)

		it('should return variable iterators when called with no id', function()

			local collection = domoticz.variables()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(variable)
				table.insert(res, variable.name)
			end)
			assert.is_same({ "a", "b", "var with spaces", "x", "y", "z"}, values(res))


			local filtered = collection.filter(function(variable)
				return variable.id < 4
			end)

			local _f = _.pluck(filtered, { 'name' })
			assert.is_same(_f, { "x", "y", "z" })

			local res2 = {}
			filtered.forEach(function(variable)
				table.insert(res2, variable.id)
			end)

			assert.is_same({ 1, 2, 3 }, values(res2))


			local reduced = collection.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(21, reduced)

			local reduced2 = filtered.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(6, reduced2)
		end)

		it('should give you a changed device when you need one', function()

			assert.is_same('device1', domoticz.changedDevices('device1').name)

			-- should be in the cache
			assert.is_same('device1', domoticz.__devices['device1'].name)
			assert.is_same(1, domoticz.__devices[1].id)

			-- test if next call uses the case
			domoticz.__devices[1].id = 1111
			assert.is_same(1111, domoticz.devices(1).id)
		end)

		it('should return nil if you ask for a non-existing device', function()
			assert.is_nil(domoticz.changedDevices('bla'))
		end)

		it('should return changed device iterators when called with no id', function()

			local collection = domoticz.changedDevices()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(device)
				table.insert(res, device.name)
			end)
			assert.is_same({ "device1", "device2", "device5", "device6", "device7", "device8", "device9", "device9" }, values(res))


			local filtered = collection.filter(function(device)
				return device.id < 4
			end)

			local _f = _.pluck(filtered, { 'name' })
			assert.is_same(_f, { "device1", "device2" })

			local res2 = {}
			filtered.forEach(function(device)
				table.insert(res2, device.id)
			end)

			assert.is_same({ 1, 2}, res2)


			local reduced = collection.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(48, reduced)

			local reduced2 = filtered.reduce(function(acc, device)
				acc = acc + device.id

				return acc
			end, 0)

			assert.is_same(3, reduced2)
		end)

		it('should give you a changed vars when you need one', function()

			assert.is_same('x', domoticz.changedVariables('x').name)

			-- should be in the cache
			assert.is_same('x', domoticz.__variables['x'].name)
			assert.is_same(1, domoticz.__variables[1].id)

			-- test if next call uses the case
			domoticz.__variables[1].id = 1111
			assert.is_same(1111, domoticz.changedVariables(1).id)
		end)

		it('should return nil if you ask for a non-existing device', function()
			assert.is_nil(domoticz.changedVariables('bla'))
		end)

		it('should return changed variable iterators when called with no id', function()

			local collection = domoticz.changedVariables()
			assert.is_function(collection.forEach)

			local res = {}

			collection.forEach(function(var)
				table.insert(res, var.name)
			end)
			assert.is_same({ "a", "b", "var with spaces", "x", "z",  }, values(res))


			local filtered = collection.filter(function(var)
				return var.id < 4
			end)

			local _f = _.pluck(filtered, { 'name' })
			assert.is_same(_f, { "x", "z" })

			local res2 = {}
			filtered.forEach(function(var)
				table.insert(res2, var.id)
			end)

			assert.is_same({ 1, 3 }, values(res2))


			local reduced = collection.reduce(function(acc, var)
				acc = acc + var.id

				return acc
			end, 0)

			assert.is_same(19, reduced)

			local reduced2 = filtered.reduce(function(acc, var)
				acc = acc + var.id

				return acc
			end, 0)

			assert.is_same(4, reduced2)
		end)

		it('should give you a changed scene when you need one', function()

			assert.is_same('Scene2', domoticz.changedScenes('Scene2').name)

			-- should be in the cache
			assert.is_same('Scene2', domoticz.__scenes['Scene2'].name)
			assert.is_same(2, domoticz.__scenes[2].id)

			-- test if next call uses the case
			domoticz.__scenes[2].id = 1111
			assert.is_same(1111, domoticz.scenes(2).id)
		end)

		it('should give you a changed group when you need one', function()

			assert.is_same('Group1', domoticz.changedGroups('Group1').name)

			-- should be in the cache
			assert.is_same('Group1', domoticz.__groups['Group1'].name)
			assert.is_same(3, domoticz.__groups[3].id)

			-- test if next call uses the case
			domoticz.__groups[3].id = 1111
			assert.is_same(1111, domoticz.groups(3).id)
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
			assert.is_equal(d1, domoticz.devices(1))
			assert.is_equal(d2, domoticz.devices(2))
			assert.is_equal(d3, domoticz.devices(3))
			assert.is_equal(d4, domoticz.devices(4))
		end)

		it('should have created a text device', function()
			assert.is_same('device8', domoticz.devices('device8').name)
		end)
	end)

	it('should convert to Celsius', function()
		assert.is_same(35, domoticz.toCelsius(95))
		assert.is_same(10, domoticz.toCelsius(18, true))
	end)

	it('should url encode', function()

		local s = 'a b c'
		assert.is_same('a+b+c', domoticz.urlEncode(s))

	end)

	it('should round', function()
		assert.is_same(10, domoticz.round(10.4, 0))
		assert.is_same(10.0, domoticz.round(10, 1))
		assert.is_same(10.00, domoticz.round(10, 2))
		assert.is_same(10.10, domoticz.round(10.1, 2))
		assert.is_same(10.1, domoticz.round(10.05, 1))
		assert.is_same(10.14, domoticz.round(10.144, 2))
		assert.is_same(10.144, domoticz.round(10.144, 3))
		assert.is_same(-10.144, domoticz.round(-10.144, 3))
		assert.is_same(-10.001, domoticz.round(-10.0009, 3))
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


end)
