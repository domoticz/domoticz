local _ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'

local testData = require('tstData')

describe('timed commands', function()
	local TimedCommand
	local commandArray = {}
	local cmd

	local domoticz = {
		sendCommand = function(command, value)
			table.insert(commandArray, {[command] = value})
			return commandArray[#commandArray], command, value
		end
	}

	setup(function()
		_G.logLevel = 0
		_G.globalvariables = {
			Security = 'sec',
			['radix_separator'] = '.',
			['script_reason'] = 'device',
			['domoticz_listening_port'] = '8080',
			['script_path'] = scriptPath
		}
		TimedCommand = require('TimedCommand')
	end)

	teardown(function()
		TimedCommand = nil
	end)

	before_each(function()
		cmd = TimedCommand(domoticz, 'mySwitch', 'On')
	end)

	after_each(function()
		commandArray = {}
		cmd = nil
	end)

	it('should instantiate', function()
		assert.is_not_nil(cmd)
	end)

	it('should have a handle to command', function()
		latest = cmd._latest
		assert.is_same({["mySwitch"]="On"}, latest)
	end)

	it('should have an after_sec method', function()
		assert.is_function(cmd.afterSec)
		local res = cmd.afterSec(5)
		assert.is_same({["mySwitch"]="On AFTER 5"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.forMin)
		res.forMin(10)
		assert.is_same({["mySwitch"]="On AFTER 5 FOR 10"}, cmd._latest)

		assert.is_same({{["mySwitch"]="On AFTER 5 FOR 10"}}, commandArray)
	end)

	it('should have an after_min method', function()
		assert.is_function(cmd.afterMin)
		local res = cmd.afterMin(5)
		assert.is_same({["mySwitch"]="On AFTER 300"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.forMin)
		res.forMin(10)
		assert.is_same({["mySwitch"]="On AFTER 300 FOR 10"}, cmd._latest)
		assert.is_same({{["mySwitch"]="On AFTER 300 FOR 10"}}, commandArray)
	end)

	it('should have an for_min method', function()
		assert.is_function(cmd.forMin)
		local res = cmd.forMin(5)
		assert.is_same({["mySwitch"]="On FOR 5"}, cmd._latest)

		-- and it should have an after_sec
		assert.is_function(res.afterSec)
		res.afterSec(10)
		assert.is_same({["mySwitch"]="On AFTER 10 FOR 5"}, cmd._latest)

		-- and it should have an after_min
		assert.is_function(res.afterMin)
		res.afterMin(10)
		assert.is_same({["mySwitch"]="On AFTER 600 FOR 5"}, cmd._latest)

		assert.is_same({{["mySwitch"]="On AFTER 600 FOR 5"}}, commandArray)
	end)

	it('should have an within_min method', function()
		assert.is_function(cmd.withinMin)
		local res = cmd.withinMin(5)
		assert.is_same({["mySwitch"]="On RANDOM 5"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.forMin)
		res.forMin(10)
		assert.is_same({["mySwitch"]="On RANDOM 5 FOR 10"}, cmd._latest)
		assert.is_same({{["mySwitch"]="On RANDOM 5 FOR 10"}}, commandArray)
	end)

end)
