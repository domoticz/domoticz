local _ = require 'lodash'

package.path = package.path .. ";../?.lua"

describe('timed commands', function()
	local TimeCommand
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
		assert.is_function(cmd.after_sec)
		local res = cmd.after_sec(5)
		assert.is_same({["mySwitch"]="On AFTER 5"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.for_min)
		res.for_min(10)
		assert.is_same({["mySwitch"]="On AFTER 5 FOR 10"}, cmd._latest)

		assert.is_same({{["mySwitch"]="On AFTER 5 FOR 10"}}, commandArray)
	end)

	it('should have an after_min method', function()
		assert.is_function(cmd.after_min)
		local res = cmd.after_min(5)
		assert.is_same({["mySwitch"]="On AFTER 300"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.for_min)
		res.for_min(10)
		assert.is_same({["mySwitch"]="On AFTER 300 FOR 10"}, cmd._latest)
		assert.is_same({{["mySwitch"]="On AFTER 300 FOR 10"}}, commandArray)
	end)

	it('should have an for_min method', function()
		assert.is_function(cmd.for_min)
		local res = cmd.for_min(5)
		assert.is_same({["mySwitch"]="On FOR 5"}, cmd._latest)

		-- and it should have an after_sec
		assert.is_function(res.after_sec)
		res.after_sec(10)
		assert.is_same({["mySwitch"]="On AFTER 10 FOR 5"}, cmd._latest)

		-- and it should have an after_min
		assert.is_function(res.after_min)
		res.after_min(10)
		assert.is_same({["mySwitch"]="On AFTER 600 FOR 5"}, cmd._latest)

		assert.is_same({{["mySwitch"]="On AFTER 600 FOR 5"}}, commandArray)
	end)

	it('should have an within_min method', function()
		assert.is_function(cmd.within_min)
		local res = cmd.within_min(5)
		assert.is_same({["mySwitch"]="On RANDOM 5"}, cmd._latest)

		-- and it should have a for_min
		assert.is_function(res.for_min)
		res.for_min(10)
		assert.is_same({["mySwitch"]="On RANDOM 5 FOR 10"}, cmd._latest)
		assert.is_same({{["mySwitch"]="On RANDOM 5 FOR 10"}}, commandArray)
	end)

end)
