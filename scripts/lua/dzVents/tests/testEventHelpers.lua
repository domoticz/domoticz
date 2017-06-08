local _ = require 'lodash'
_G._ = require 'lodash'

package.path = package.path .. ";../?.lua"

local function keys(t)
	local keys = _.keys(t)
	return _.sortBy(keys, function(k)
		return tostring(k)
	end)
end

local function values(t)
	local values = _.values(t)
	table.sort(values)
	return values
end

describe('event helpers', function()
	local EventHelpers, helpers, utils, settings

	local domoticz = {
		['EVENT_TYPE_TIMER'] = 'timer',
		['EVENT_TYPE_DEVICE'] = 'device',
		['settings'] = {},
		['name'] = 'domoticz', -- used in script1
		['devices'] = {
			['device1'] = { name = '' },
			['onscript1'] = { name = 'onscript1', id = 1 },
			['onscript4'] = { name = 'onscript4', id = 4 },
			['on_script_5'] = { name = 'on_script_5', id = 5 },
			['wildcard'] = { name = 'wildcard', id = 6 },
			['someweirddevice'] = { name = 'someweirddevice', id = 7 },
			['mydevice'] = { name = 'mydevice', id = 8 }
		}
	}

	setup(function()
		settings = {
			['Log level'] = 1
		}

		_G.TESTMODE = true

		EventHelpers = require('EventHelpers')
	end)

	teardown(function()
		helpers = nil
	end)

	before_each(function()
		helpers = EventHelpers(settings, domoticz)
		utils = helpers._getUtilsInstance()
		utils.print = function() end
		utils.activateDevicesFile = function() end
	end)

	after_each(function()
		helpers = nil
		utils = nil
	end)

	describe('Reverse find', function()
		it('should find some string from behind', function()
			local s = 'my_Sensor_Temperature'
			local from, to = helpers.reverseFind(s, '_')
			assert.are_same(from, 10)
			assert.are_same(to, 10) -- lenght of _
		end)

		it('should find some string from behind (again)', function()
			local s = 'a_b'
			local from, to = helpers.reverseFind(s, 'b')
			assert.are_same(from, 3)
			assert.are_same(to, 3) -- lenght of _
		end)

		it('should find some string from behind (again)', function()
			local s = 'a_bbbb_c'
			local from, to = helpers.reverseFind(s, 'bb')
			assert.are_same(from, 5)
			assert.are_same(to, 6) -- lenght of _
		end)

		it('should return nil when not found', function()
			local s = 'mySensor_Temperature'
			local from, to = helpers.reverseFind(s, 'xx')
			assert.is_nil(from)
			assert.is_nil(to)
		end)
	end)

	describe('Device by event name', function()
		it('should return the device name without value extension', function()
			local deviceName = helpers.getDeviceNameByEvent('mySensor')
			assert.are_same(deviceName, 'mySensor')

		end)

		it('should return the device name with a known value extension', function()
			local deviceName = helpers.getDeviceNameByEvent('mySensor_Temperature')
			assert.are_same('mySensor', deviceName)
		end)

		it('should return the device name with underscores and value extensions', function()
			local deviceName = helpers.getDeviceNameByEvent('my_Sensor_Temperature')
			assert.are_same('my_Sensor',  deviceName)
		end)

		it('should return the device name with underscores', function()
			local deviceName = helpers.getDeviceNameByEvent('my_Sensor_blaba')
			assert.are_same('my_Sensor_blaba', deviceName)
		end)

	end)

	describe('Loading modules', function()
		it('should get a list of files in a folder', function()
			local files = helpers.scandir('scandir')
			local f = {'f1','f2','f3'}
			assert.are.same(f, files)
		end)
	end)

	describe('Evaluate time triggers', function()
		it('should compare time triggers at the current time', function()
			assert.is_true(helpers.evalTimeTrigger('Every minute', {['hour']=13, ['min']=6}))

			assert.is_true(helpers.evalTimeTrigger('Every 2 minutes', {['hour']=13, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every 2 minutes', {['hour']=13, ['min']=1}))

			assert.is_true(helpers.evalTimeTrigger('Every other minute', {['hour']=13, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every other minute', {['hour']=13, ['min']=1}))

			assert.is_false(helpers.evalTimeTrigger('Every 5 minutes', {['hour']=13, ['min']=6}))

			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=10}))
			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=20}))
			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=30}))
			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=40}))
			assert.is_true(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=50}))
			assert.is_false(helpers.evalTimeTrigger('Every 10 minutes', {['hour']=13, ['min']=11}))

			assert.is_true(helpers.evalTimeTrigger('Every 15 minutes', {['hour']=13, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 15 minutes', {['hour']=13, ['min']=15}))
			assert.is_true(helpers.evalTimeTrigger('Every 15 minutes', {['hour']=13, ['min']=30}))
			assert.is_true(helpers.evalTimeTrigger('Every 15 minutes', {['hour']=13, ['min']=45}))
			assert.is_false(helpers.evalTimeTrigger('Every 15 minutes', {['hour']=13, ['min']=1}))

			assert.is_true(helpers.evalTimeTrigger('Every 20 minutes', {['hour']=13, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 20 minutes', {['hour']=13, ['min']=20}))
			assert.is_true(helpers.evalTimeTrigger('Every 20 minutes', {['hour']=13, ['min']=40}))
			assert.is_false(helpers.evalTimeTrigger('Every 20 minutes', {['hour']=13, ['min']=2}))

			assert.is_true(helpers.evalTimeTrigger('Every 11 minutes', {['hour']=13, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 11 minutes', {['hour']=13, ['min']=11}))
			assert.is_true(helpers.evalTimeTrigger('Every 11 minutes', {['hour']=13, ['min']=22}))

			assert.is_true(helpers.evalTimeTrigger('Every hour', {['hour']=13, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every hour', {['hour']=0, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every hour', {['hour']=13, ['min']=1}))

			assert.is_true(helpers.evalTimeTrigger('Every other hour', {['hour']=0, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every other hour', {['hour']=1, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every other hour', {['hour']=2, ['min']=0}))

			assert.is_true(helpers.evalTimeTrigger('Every 2 hours', {['hour']=0, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every 2 hours', {['hour']=1, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 2 hours', {['hour']=2, ['min']=0}))

			assert.is_true(helpers.evalTimeTrigger('Every 3 hours', {['hour']=0, ['min']=0}))
			assert.is_true(helpers.evalTimeTrigger('Every 3 hours', {['hour']=3, ['min']=0}))
			assert.is_false(helpers.evalTimeTrigger('Every 3 hours', {['hour']=2, ['min']=0}))

			assert.is_true(helpers.evalTimeTrigger('at 12:23', {['hour']=12, ['min']=23}))
			assert.is_false(helpers.evalTimeTrigger('at 12:23', {['hour']=13, ['min']=23}))
			assert.is_true(helpers.evalTimeTrigger('at 0:1', {['hour']=0, ['min']=1}))
			assert.is_true(helpers.evalTimeTrigger('at 0:01', {['hour']=0, ['min']=1}))
			assert.is_true(helpers.evalTimeTrigger('at 1:1', {['hour']=1, ['min']=1}))
			assert.is_true(helpers.evalTimeTrigger('at 10:10', {['hour']=10, ['min']=10}))


			assert.is_true(helpers.evalTimeTrigger('at *:10', {['hour']=10, ['min']=10}))
			assert.is_true(helpers.evalTimeTrigger('at *:10', {['hour']=11, ['min']=10}))
			assert.is_false(helpers.evalTimeTrigger('at *:10', {['hour']=11, ['min']=11}))
			assert.is_false(helpers.evalTimeTrigger('at *:*', {['hour']=11, ['min']=10}))
			assert.is_false(helpers.evalTimeTrigger('at 2:*', {['hour']=11, ['min']=10}))
			assert.is_true(helpers.evalTimeTrigger('at 2:*', {['hour']=2, ['min']=10}))

			assert.is_true(helpers.evalTimeTrigger('at 1:*', {['hour']=1, ['min']=11}))
			assert.is_true(helpers.evalTimeTrigger('at: 1:*', {['hour']=1, ['min']=11}))
			assert.is_false(helpers.evalTimeTrigger('at 1:*', {['hour']=2, ['min']=11}))
			assert.is_false(helpers.evalTimeTrigger('at *:3', {['hour']=2, ['min']=11}))
			assert.is_false(helpers.evalTimeTrigger('at *:5', {['hour']=2, ['min']=11}))
			assert.is_true(helpers.evalTimeTrigger('at *:5', {['hour']=2, ['min']=5}))

			assert.is_true(helpers.evalTimeTrigger('at *:5 on mon, tue, fri', {['hour']=2, ['min']=5, ['day']=6}))
			assert.is_false(helpers.evalTimeTrigger('at *:5 on sat', {['hour']=2, ['min']=5, ['day']=5}))

			assert.is_true(helpers.evalTimeTrigger('every other minute on mon, tue, fri', {['hour']=2, ['min']=4, ['day']=2}))
			assert.is_false(helpers.evalTimeTrigger('every other minute on mon, tue, fri', {['hour']=2, ['min']=4, ['day']=1}))

			assert.is_true(helpers.evalTimeTrigger('at sunset', {['hour']=1, ['min']=4, ['SunsetInMinutes']=64}))
			assert.is_false(helpers.evalTimeTrigger('at sunset', {['hour']=1, ['min']=4, ['SunsetInMinutes']=63}))

			assert.is_true(helpers.evalTimeTrigger('at sunrise', {['hour']=1, ['min']=4, ['SunriseInMinutes']=64}))
			assert.is_false(helpers.evalTimeTrigger('at sunrise', {['hour']=1, ['min']=4, ['SunriseInMinutes']=63}))

			assert.is_true(helpers.evalTimeTrigger('at sunrise on mon', {['hour']=1, ['min']=4, ['day']=2, ['SunriseInMinutes']=64}))
			assert.is_false(helpers.evalTimeTrigger('at sunrise on fri', {['hour']=1, ['min']=4, ['day']=2, ['SunriseInMinutes']=64}))
		end)

		it('should check time defs', function()
			assert.is_true(helpers.checkTimeDefs({ 'Every minute' }, {['hour']=13, ['min']=6}))
			assert.is_false(helpers.checkTimeDefs({ 'Every hour' }, {['hour']=13, ['min']=6}))
			assert.is_true(helpers.checkTimeDefs({ 'Every hour' }, {['hour']=13, ['min']=0}))

			assert.is_false(helpers.checkTimeDefs({ 'Every 2 minutes', 'every hour'}, {['hour']=13, ['min']=1}))
			assert.is_true(helpers.checkTimeDefs({ 'Every 2 minutes', 'every hour'}, {['hour']=13, ['min']=2}))

			assert.is_true(helpers.checkTimeDefs({ 'Every 5 minutes', 'every 3 minutes'}, {['hour']=13, ['min']=5}))
			assert.is_true(helpers.checkTimeDefs({ 'Every 5 minutes', 'every 3 minutes'}, {['hour']=13, ['min']=9}))
			assert.is_false(helpers.checkTimeDefs({ 'Every 5 minutes', 'every 3 minutes'}, {['hour']=13, ['min']=11}))

			assert.is_true(helpers.checkTimeDefs({ 'at *:3', 'at *:5', 'at 1:*'}, {['hour']=13, ['min']=5}))
			assert.is_true(helpers.checkTimeDefs({ 'at *:3', 'at *:5', 'at 1:*'}, {['hour']=13, ['min']=3}))

			assert.is_true(helpers.checkTimeDefs({ 'at *:3', 'at *:5', 'at 1:*'}, {['hour']=1, ['min']=11}))

			assert.is_false(helpers.checkTimeDefs({ 'at *:3', 'at *:5', 'at 1:*'}, {['hour']=2, ['min']=11}))
		end)

	end)

	describe('Bindings', function()
		it('should return an array of scripts for the same trigger', function()
			local modules = helpers.getEventBindings()
			local script1 = modules['onscript1']

			assert.are.same('table', type(script1))

			local res = {}
			for i,mod in pairs(script1) do
				table.insert(res, mod.name)
			end
			table.sort(res)

			assert.are.same({'script1', 'script3', 'script_combined'}, res)

		end)

		it('should return scripts for all triggers', function()
			local modules = helpers.getEventBindings()
			assert.are.same({
				8,
				'on_script_5',
				'onscript1',
				'onscript2',
				'onscript4',
				'onscript7',
				'onscript7b',
				'some*device',
				'somedevice',
				'wild*' }, keys(modules))
			assert.are.same(10, _.size(modules))
		end)

		it('should detect erroneous modules', function()
			local err = false
			utils.log = function(msg,level)
				if (level == 1) then
					err = true
				end
			end

			local modules, errModules = helpers.getEventBindings()
			assert.are.same(true, err)
			assert.are.same(4, _.size(errModules))
			assert.are.same({
				'script_error',
				'script_incomplete_missing_execute',
				'script_incomplete_missing_on',
				'script_notable'
			}, values(errModules))
		end)

		it('should detect non-table modules', function()

			local err = false
			utils.log = function(msg,level)
				if (level == 1) then
					if ( string.find(msg, 'not a valid module') ~= nil) then
						err = true
					end
				end
			end

			local modules, errModules = helpers.getEventBindings()
			assert.are.same(true, err)
			assert.are.same(4, _.size(errModules))
			assert.are.same({
				'script_error',
				'script_incomplete_missing_execute',
				'script_incomplete_missing_on',
				'script_notable'
			}, values(errModules))
		end)

		it('should detect modules without on section', function()
			local err = false
			utils.log = function(msg,level)
				if (level == 1) then
					if ( string.find(msg, 'lua has no "on" and/or') ~= nil) then
						err = true
					end
				end
			end

			local modules, errModules = helpers.getEventBindings()
			assert.are.same(true, err)
			assert.are.same(4, _.size(errModules))
			assert.are.same({
				'script_error',
				'script_incomplete_missing_execute',
				'script_incomplete_missing_on',
				'script_notable'
			}, values(errModules))

		end)

		it('should evaluate active functions', function()
			domoticz.devices['device1'].name = 'Device 1'
			local modules = helpers.getEventBindings()

			local script = modules['onscript_active']
			assert.is_not_nil(script)
			domoticz.devices['device1'].name = ''
		end)

		it('should handle combined triggers', function()

			local modules = helpers.getEventBindings()
			local script2 = modules['onscript2']

			assert.are.same('table', type(script2))

			local res = {}
			for i,mod in pairs(script2) do
				table.insert(res, mod.name)
			end
			table.sort(res)

			assert.are.same({'script2', 'script_combined'}, res)

		end)

		it('should return timer scripts', function()
			--on = {'timer'} -- every minute
			local modules = helpers.getTimerHandlers()

			assert.are.same(3, _.size(modules))
			local names = _.pluck(modules, {'name'})
			table.sort(names)

			assert.are.same({
				"script_timer_classic",
				"script_timer_single",
				"script_timer_table" }, names)

		end)

	end)

	describe('Http data', function()
		it('should fetch http data when timer trigger is met', function()
			local requested = false
			helpers.utils.requestDomoticzData = function(ip, port)
				requested = true
			end

			helpers.evalTimeTrigger = function(interval)
				return true
			end

			helpers.fetchHttpDomoticzData('0', '1', 'some trigger')
			assert.is_true(requested)
		end)

		it('should log an error when passing wrong stuff', function()
			local requested = false
			local msg, level
			helpers.utils.requestDomoticzData = function(ip, port)
				requested = true
			end

			helpers.evalTimeTrigger = function(interval)
				return true
			end

			utils.log = function(m, l)
				msg = m
				level = l
			end

			helpers.fetchHttpDomoticzData()
			assert.is_false(requested)
			assert.is_same('Invalid ip for contacting Domoticz', msg)
			assert.is_same(utils.LOG_ERROR, level)

		end)
	end)

	describe('Various', function()
		it('should dump the command array', function()
			local messages = {}

			utils.log = function(m, l)
				table.insert(messages, m)
			end

			local array = {
				{a = 1},
				{b = 2}
			}

			helpers.dumpCommandArray(array)
			assert.is_same({
				"[1] = a: 1",
				"[2] = b: 2",
				"====================================================="
			}, messages)
		end)
	end)

	describe('Events', function()

		it('should call the event handler', function()

			local bindings = helpers.getEventBindings()
			local script1 = bindings['onscript1'][1]

			local res = helpers.callEventHandler(script1,
				{
					name = 'device'
				})
			-- should pass the arguments to the execute function
			-- and catch the results from the function
			assert.is_same('script1: domoticz device device', res)
		end)

		it('should catch errors', function()
			local bindings = helpers.getEventBindings()
			local script2 = bindings['onscript2'][1]


			local err = false
			utils.log = function(msg,level)
				if (level == 1) then
					err = true
				end
			end

			-- script2 does nasty things it shouldn't do
			local res = helpers.callEventHandler(script2)
			assert.is_true(err)
		end)

		it('should handle events', function()
			local bindings = helpers.getEventBindings()
			local script1 = bindings['onscript1']

			local called = {}
			helpers.callEventHandler = function(script)
				table.insert(called, script.name)
			end

			helpers.handleEvents(script1)
			assert.is_same({"script1", "script3", "script_combined"}, called)
		end)

	end)

	describe('Event dispatching', function()

		it('should dispatch all event scripts', function()
			local scripts = {}
			local devices = {}
			local dumped = false

			local devicechanged = {
				['onscript1'] = 10,
				['onscript4'] = 20,
				['wildcard'] = 30,
				['someweirddevice'] = 40,
				['on_script_5_Temperature'] = 50,
				['on_script_5'] = 50, -- should be triggered only once
				['mydevice'] = 60 --script 6 triggers by this device' id
			}

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts, _device)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
				table.insert(devices, _device.name)
			end

			local res = helpers.dispatchDeviceEventsToScripts(devicechanged)

			table.sort(scripts)
			table.sort(devices)

			assert.is_same({
				"script1",
				"script3",
				"script4",
				"script5",
				"script6",
				"script_combined",
				"script_wildcard1",
				"script_wildcard2"
			}, scripts)
			assert.is_same({
				"mydevice",
				"on_script_5",
				"onscript1",
				"onscript4",
				"someweirddevice",
				"wildcard"}, devices)

			assert.is_true(dumped)

		end)

		it('should dispatch all timer events', function()
			local scripts = {}
			local dumped = false
			local fetched = false

			helpers.settings['Enable http fetch'] = true

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
			end

			helpers.fetchHttpDomoticzData = function()
				fetched = true
			end

			local res = helpers.dispatchTimerEventsToScripts()

			table.sort(scripts)

			assert.is_same({
				'script_timer_classic',
				'script_timer_single',
				'script_timer_table'
			}, scripts)

			assert.is_true(dumped)
			assert.is_true(fetched)
		end)

		it('should auto fetch http data', function()
			local fetched = false
			helpers.fetchHttpDomoticzData = function()
				fetched = true
			end

			helpers.settings['Enable http fetch'] = true

			helpers.autoFetchHttpDomoticzData()
			assert.is_true(fetched)

			fetched = false
			helpers.settings['Enable http fetch'] = false

			helpers.autoFetchHttpDomoticzData()
			assert.is_false(fetched) -- should still be false
		end)
	end)
end)