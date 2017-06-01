local _ = require 'lodash'
_G._ = require 'lodash'

local scriptPath = ''

--package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

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
		['EVENT_TYPE_VARIABLE'] = 'variable',
		['EVENT_TYPE_SECURITY'] = 'security',
		['SECURITY_DISARMED'] = 'Disarmed',
		['SECURITY_ARMEDAWAY'] = 'Armed Away',
		['SECURITY_ARMEDHOME'] = 'Armed Home',
		['settings'] = {},
		['radixSeparator'] = '.',
		['security'] = 'Armed Away',
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
			['Log level'] = 1,

		}

		_G.TESTMODE = true

		_G.globalvariables = {
			Security = 'sec',
			['radix_separator'] = '.',
			['script_reason'] = 'device',
			['script_path'] = scriptPath,
			['Security'] = 'Armed Away',
			['dzVents_log_level'] = 1,
			['domoticz_listening_port'] = '8181'
		}

		EventHelpers = require('EventHelpers')
	end)

	teardown(function()
		helpers = nil
	end)

	before_each(function()
		helpers = EventHelpers(domoticz)
		utils = helpers._getUtilsInstance()
		utils.print = function() end
		utils.activateDevicesFile = function() end
		_G.scripts = {
			['switchScript'] = [[
				return {
					active = true,
					on = {
						'mySwitch'
					},
					execute = function(domoticz, device, triggerInfo)

						if (device.toggleSwitch) then device.toggleSwitch() end

						return 'script1: ' ..
								tostring(domoticz.name) ..
								' ' ..
								tostring(device.name) ..
								' ' ..
								tostring(triggerInfo['type'])
					end
				}
			]],
			['faultyscript'] = [[
				return {
					active = true,
					on = {
			]],
			['internalOnScript1'] = [[
				return {
					active = true,
					on = {
						'onscript1'
					},
					execute = function(domoticz, device, triggerInfo)

						return 'internal onscript1: ' ..
								tostring(domoticz.name) ..
								' ' ..
								tostring(device.name) ..
								' ' ..
								tostring(triggerInfo['type'])
					end
				}
			]],
		}
	end)

	after_each(function()
		helpers = nil
		utils = nil
	end)

	describe('Loading modules', function()
		it('should get a list of files in a folder', function()
			local files = helpers.scandir('scandir')

			local f = {'f1','f2','f3', 'lua' }
			assert.are.same(f, _.pluck(files, {'name'}))
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

			assert.is_true(helpers.evalTimeTrigger('on mon', { ['hour'] = 2, ['min'] = 4, ['day'] = 2 }))
			assert.is_false(helpers.evalTimeTrigger('on mon', { ['hour'] = 2, ['min'] = 4, ['day'] = 3 }))

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

			assert.are.same({'internalOnScript1', 'script1', 'script3', 'script_combined'}, res)

		end)

		it('should find scripts for changed items', function()

			local modules = {
				['aaa'] = { 'a1', 'a2', 'a3' },
				['bbb'] = { 'b1', 'b2', 'b3' },
				['aa*'] = { 'c1', 'c2', 'c3' },
				['a*'] =  { 'd1', 'd2', 'd3' },
				['aaa*'] = { 'e1', 'e2', 'e3' }
			}

			local scripts = helpers.findScriptForChangedItem('aaa', modules)

			assert.are.same({ 'a1', 'a2', 'a3', 'c1', 'c2', 'c3', 'd1', 'd2', 'd3', 'e1', 'e2', 'e3' }, values(scripts))

		end)

		it('should return scripts for all triggers', function()
			local modules = helpers.getEventBindings()
			assert.are.same({
				8,
				'deviceGork',
				'loggingstuff',
				'mySwitch',
				'on_script_5',
				'onscript1',
				'onscript2',
				'onscript4',
				'onscript7',
				'onscript7b',
				'some*device',
				'somedevice',
				'wild*' }, keys(modules))
			assert.are.same(13, _.size(modules))
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
			assert.are.same(5, _.size(errModules))
			assert.are.same({
				'faultyscript',
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
			assert.are.same(5, _.size(errModules))
			assert.are.same({
				'faultyscript',
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
			assert.are.same(5, _.size(errModules))
			assert.are.same({
				'faultyscript',
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

		it('should return variable script', function()
			local modules = helpers.getEventBindings('variable')
			local myVar1 = modules['myVar1']

			assert.are.same('table', type(myVar1))

			local res = {}
			for i, mod in pairs(myVar1) do
				table.insert(res, mod.name)
			end
			table.sort(res)

			assert.are.same({ 'script_variable1', 'script_variable2',  }, res)
		end)

		it('should return an array of internal scripts for the same trigger', function()

			local modules = helpers.getEventBindings()
			local script1 = modules['mySwitch']
			assert.are.same('table', type(script1))

			local res = {}
			for i,mod in pairs(script1) do
				table.insert(res, mod.name)
			end
			table.sort(res)

			assert.are.same({'switchScript'}, res)
		end)

		it('should return internal and external scripts for all triggers', function()
			local modules = helpers.getEventBindings()
			assert.are.same({
				8,
				'deviceGork',
				'loggingstuff',
				'mySwitch',
				'on_script_5',
				'onscript1',
				'onscript2',
				'onscript4',
				'onscript7',
				'onscript7b',
				'some*device',
				'somedevice',
				'wild*' }, keys(modules))
			assert.are.same(13, _.size(modules))
		end)

		it('should return scripts for a device that has time-constrained triggers', function()
			local modules = helpers.getEventBindings('device', { ['hour'] = 13, ['min'] = 5, ['day'] = 2 })
			local scripts = modules['deviceZork']

			local res = {}
			for i, mod in pairs(scripts) do
				table.insert(res, mod.name)
			end

			table.sort(res)
			assert.are.same({ 'script_with_time-contrained_device' }, res)

			modules = helpers.getEventBindings('device', { ['hour'] = 2, ['min'] = 1, ['day'] = 5  })
			scripts = modules['deviceZork']
			assert.is_nil(scripts)


			modules = helpers.getEventBindings('device', { ['hour'] = 2, ['min'] = 1, ['day'] = 1 })
			scripts = modules['deviceDork']

			local res = {}
			for i, mod in pairs(scripts) do
				table.insert(res, mod.name)
			end

			table.sort(res)
			assert.are.same({ 'script_with_time-contrained_device' }, res)

			scripts = modules['deviceGork']

			local res = {}
			for i, mod in pairs(scripts) do
				table.insert(res, mod.name)
			end
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

		it('should have proper settings', function()
			assert.are.same('http://127.0.0.1:8181', helpers.settings['Domoticz url'])

		end)

		it('should add global helpers to the domoticz object', function()

			local bindings = helpers.getEventBindings()

			assert.is_same('abc', domoticz.helpers.myHelper('a', 'b', 'c')) -- method is defined in global_data.lua

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
			assert.is_same('internal onscript1: domoticz device device', res)

			script1 = bindings['onscript1'][2]

			res = helpers.callEventHandler(script1,
				{
					name = 'device'
				})
			-- should pass the arguments to the execute function
			-- and catch the results from the function
			assert.is_same('script1: domoticz device device', res)
		end)

		it('should call the event handler for variables', function()

			local bindings = helpers.getEventBindings('variable')
			local myVar1 = bindings['myVar1'][1]

			local res = helpers.callEventHandler(myVar1,
				nil,
				{
					name = 'myVar1',
					set = function()
					end
				})
			-- should pass the arguments to the execute function
			-- and catch the results from the function
			assert.is_same('script_variable1: domoticz myVar1 variable', res)
		end)

		it('should call the event handler for security events', function()

			local bindings = helpers.getEventBindings('security')

			local modulesFound = _.pluck(bindings, {'name'})

			local scriptSecurity = _.find(bindings, function(mod)
				return mod.name == 'script_security'
			end)

			local scriptSecurityGrouped = _.find(bindings, function(mod)
				return mod.name == 'script_security_grouped'
			end)

			assert.are.same({
				"script_security",
				"script_security_grouped"
			}, modulesFound)


			local res = helpers.callEventHandler(scriptSecurity,
				nil,
				nil,
				'Armed Away')
			-- should pass the arguments to the execute function
			-- and catch the results from the function
			assert.is_same('script_security: nil Armed Away', res)

			local res = helpers.callEventHandler(scriptSecurityGrouped,
				nil,
				nil,
				'Armed Home')
			-- should pass the arguments to the execute function
			-- and catch the results from the function
			assert.is_same('script_security: nil Armed Away', res)

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
			assert.is_same({'internalOnScript1', "script1", "script3", "script_combined"}, called)
		end)

		it('should have custom logging settings', function()
			local bindings = helpers.getEventBindings('device')

			local loggingstuff = bindings['loggingstuff']

			local moduleLevel, moduleMarker
			helpers.callEventHandler = function(mod, dev, var, sec)
				moduleLevel = _G.logLevel
				moduleMarker = _G.logMarker
			end

			helpers.handleEvents(loggingstuff)
			assert.is_same(4, moduleLevel)
			assert.is_same('Hey you', moduleMarker)
			assert.is_same(1, _G.logLevel)
			assert.is_same(nil, _G.logMarker)
		end)

	end)

	describe('Event dispatching', function()

		it('should dispatch all device event scripts', function()
			local scripts = {}
			local devices = {}
			local dumped = false

			local function getDummy(id, name, state, value)
				return {
					['id'] = id,
					['name'] = name,
					['state'] = state,
					['value'] = value
				}
			end

			local devicechanged = {
				['onscript1'] = getDummy(1, 'onscript1', 'state1', 10),
				['onscript4'] = getDummy(2, 'onscript4', 'state2', 20),
				['wildcard'] = getDummy(3, 'wildcard', 'state3', 30),
				['someweirddevice'] = getDummy(4, 'someweirddevice', 'state4', 40),
				--['8device'] = getDummy(8, '8device', 'state64', 404),
				-- ['on_script_5_Temperature'] = 50,
				--['on_script_5'] = 50, -- should be triggered only once
				['mydevice'] = getDummy(8, 'mydevice', 'state5', 60), --script 6 triggers by this device' id,
			}

			devicechanged['forEach'] = function(func)
				local res
				for i, item in pairs(devicechanged) do
					if (type(item) ~= 'function' and type(i) ~= 'number') then
						res = func(item, i, devicechanged)
						if (res == false) then -- abort
							return
						end
					end
				end
			end

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts, _device)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
				table.insert(devices, _device.name)
			end
			local res = helpers.dispatchDeviceEventsToScripts({['changedDevices'] = devicechanged})

			table.sort(scripts)
			table.sort(devices)
			assert.is_same({
				'internalOnScript1',
				"script1",
				"script3",
				"script4",
				"script6",
				"script_combined",
				"script_wildcard1",
				"script_wildcard2"
			}, scripts)


			assert.is_same({
				"mydevice",
				"onscript1",
				"onscript4",
				"someweirddevice",
				"wildcard"}, devices)

			assert.is_true(dumped)

		end)

		it('should dispatch all timer events', function()
			local scripts = {}
			local dumped = false

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
			end

			local res = helpers.dispatchTimerEventsToScripts()

			table.sort(scripts)

			assert.is_same({
				'script_timer_classic',
				'script_timer_single',
				'script_timer_table'
			}, scripts)

			assert.is_true(dumped)
		end)

		it('should dispatch all security events', function()
			local scripts = {}
			local dumped = false

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
			end

			local res = helpers.dispatchSecurityEventsToScripts()

			table.sort(scripts)

			assert.is_same({
				'script_security',
				'script_security_grouped'

			}, scripts)

			assert.is_true(dumped)
		end)


		it('should dispatch all variable event scripts', function()
			local scripts = {}
			local variables = {}
			local dumped = false

			local function getDummy(id, name, value)
				return {
					['id'] = id,
					['name'] = name,
					['value'] = value
				}
			end

			local varchanged = {
				['myVar1'] = getDummy(1, 'myVar1', 10),
				['myVar2'] = getDummy(2, 'myVar2', 20),
				['myVar3'] = getDummy(3, 'myVar3', 30),
			}

			varchanged['forEach'] = function(func)
				local res
				for i, item in pairs(varchanged) do
					if (type(item) ~= 'function' and type(i) ~= 'number') then
						res = func(item, i, varchanged)
						if (res == false) then -- abort
							return
						end
					end
				end
			end

			helpers.dumpCommandArray = function()
				dumped = true
			end

			helpers.handleEvents = function(_scripts, _device, _variable)
				_.forEach(_scripts, function(s)
					table.insert(scripts, s.name)
				end)
				table.insert(variables, _variable.name)
			end
			local res = helpers.dispatchVariableEventsToScripts({ ['changedVariables'] = varchanged })

			table.sort(scripts)
			table.sort(variables)
			assert.is_same({
				'script_variable1',
				'script_variable2',
				'script_variable2',
				'script_variable3'
			}, scripts)


			assert.is_same({
				"myVar1",
				"myVar2",
				"myVar3",
			}, variables)

			assert.is_true(dumped)
		end)

	end)
end)