local _ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'

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
			['script_path'] = scriptPath,
			['currentTime'] = '2017-08-17 12:13:14.123'
		}
		TimedCommand = require('TimedCommand')
	end)

	teardown(function()
		TimedCommand = nil
	end)

	describe('device', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'device')
		end)

		after_each(function()
			commandArray = {}
			cmd = nil
		end)

		it('should return proper functions when nothing is set', function()
			assert.is_function(cmd.afterSec)
			assert.is_function(cmd.afterMin)
			assert.is_function(cmd.afterHour)
			assert.is_function(cmd.forSec)
			assert.is_function(cmd.forMin)
			assert.is_function(cmd.forHour)
			assert.is_function(cmd.withinMin)
			assert.is_function(cmd.withinSec)
			assert.is_function(cmd.withinHour)
			assert.is_function(cmd.silent)
			assert.is_function(cmd.repeatAfterSec)
			assert.is_function(cmd.repeatAfterMin)
			assert.is_function(cmd.repeatAfterHour)
			assert.is_nil(cmd.checkFirst) -- no current state is set

		end)

		it('should return proper function when called after', function()
			local res = cmd.afterSec(1)
			assert.is_function(res.forSec)
			assert.is_function(res.forMin)
			assert.is_function(res.forHour)
			assert.is_function(res.silent)
			assert.is_function(res.repeatAfterSec)
			assert.is_function(res.repeatAfterMin)
			assert.is_function(res.repeatAfterHour)

			assert.is_nil(res.afterSec)
			assert.is_nil(res.afterMin)
			assert.is_nil(res.afterHour)
			assert.is_nil(res.withinSec)
			assert.is_nil(res.withinMin)
			assert.is_nil(res.withinHour)
		end)

		it('should return proper function when called for', function()
			local res = cmd.forSec(1)
			assert.is_function(res.afterSec)
			assert.is_function(res.afterMin)
			assert.is_function(res.afterHour)
			assert.is_function(res.withinMin)
			assert.is_function(res.withinSec)
			assert.is_function(res.withinHour)
			assert.is_function(res.silent)
			assert.is_function(res.repeatAfterSec)
			assert.is_function(res.repeatAfterMin)
			assert.is_function(res.repeatAfterHour)

			assert.is_nil(res.forSec)
			assert.is_nil(res.forMin)
			assert.is_nil(res.forHour)
		end)

		it('should return proper functions called silent', function()
			local res = cmd.silent()
			assert.is_function(res.afterSec)
			assert.is_function(res.afterMin)
			assert.is_function(res.afterHour)
			assert.is_function(res.forSec)
			assert.is_function(res.forMin)
			assert.is_function(res.forHour)
			assert.is_function(res.withinMin)
			assert.is_function(res.withinSec)
			assert.is_function(res.withinHour)
			assert.is_function(res.repeatAfterSec)
			assert.is_function(res.repeatAfterMin)
			assert.is_function(res.repeatAfterHour)

			assert.is_nil(res.silent)
		end)

		it('should return proper functions when called repeatAfterXXX', function()
			local res = cmd.repeatAfterSec(1)
			assert.is_function(res.afterSec)
			assert.is_function(res.afterMin)
			assert.is_function(res.afterHour)
			assert.is_function(res.forSec)
			assert.is_function(res.forMin)
			assert.is_function(res.forHour)
			assert.is_function(res.withinMin)
			assert.is_function(res.withinSec)
			assert.is_function(res.withinHour)
			assert.is_function(res.silent)

			assert.is_nil(res.repeatAfterSec)
			assert.is_nil(res.repeatAfterMin)
			assert.is_nil(res.repeatAfterHour)
		end)

		it('should return checkFirst if a currentState is set', function()
			local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'device', 'On')
			assert.is_function(cmd.checkFirst)
		end)

	end)

	describe('variable', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'variable')
		end)

		after_each(function()
			commandArray = {}
			cmd = nil
		end)


		it('should return proper functions when nothing is set', function()
			assert.is_function(cmd.afterSec)
			assert.is_function(cmd.afterMin)
			assert.is_function(cmd.afterHour)
			assert.is_function(cmd.silent)
			assert.is_function(cmd.withinMin)
			assert.is_function(cmd.withinSec)
			assert.is_function(cmd.withinHour)

			assert.is_nil(cmd.forSec)
			assert.is_nil(cmd.forMin)
			assert.is_nil(cmd.forHour)
			assert.is_nil(cmd.repeatAfterSec)
			assert.is_nil(cmd.repeatAfterMin)
			assert.is_nil(cmd.repeatAfterHour)
			assert.is_nil(cmd.checkFirst)
		end)

		it('should return proper function when called after', function()

			local res = cmd.afterSec(1)

			assert.is_function(res.silent)

			assert.is_nil(res.afterSec)
			assert.is_nil(res.afterMin)
			assert.is_nil(res.afterHour)
			assert.is_nil(res.forSec)
			assert.is_nil(res.forMin)
			assert.is_nil(res.forHour)
			assert.is_nil(res.withinSec)
			assert.is_nil(res.withinMin)
			assert.is_nil(res.withinHour)
			assert.is_nil(res.repeatAfterSec)
			assert.is_nil(res.repeatAfterMin)
			assert.is_nil(res.repeatAfterHour)
			assert.is_nil(cmd.checkFirst)
		end)

		it('should return proper functions called silent', function()

			local res = cmd.silent()

			assert.is_function(res.afterSec)
			assert.is_function(res.afterMin)
			assert.is_function(res.afterHour)
			assert.is_function(res.withinMin)
			assert.is_function(res.withinSec)
			assert.is_function(res.withinHour)

			assert.is_nil(res.silent)
			assert.is_nil(res.forSec)
			assert.is_nil(res.forMin)
			assert.is_nil(res.forHour)
			assert.is_nil(res.repeatAfterSec)
			assert.is_nil(res.repeatAfterMin)
			assert.is_nil(res.repeatAfterHour)
			assert.is_nil(cmd.checkFirst)
		end)

	end)

	describe('updatedevice', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'updatedevice')
		end)

		after_each(function()
			commandArray = {}
			cmd = nil
		end)


		it('should return proper functions when nothing is set', function()
			assert.is_function(cmd.silent)

			assert.is_nil(cmd.afterSec)
			assert.is_nil(cmd.afterMin)
			assert.is_nil(cmd.afterHour)
			assert.is_nil(cmd.withinMin)
			assert.is_nil(cmd.withinSec)
			assert.is_nil(cmd.withinHour)
			assert.is_nil(cmd.forSec)
			assert.is_nil(cmd.forMin)
			assert.is_nil(cmd.forHour)
			assert.is_nil(cmd.repeatAfterSec)
			assert.is_nil(cmd.repeatAfterMin)
			assert.is_nil(cmd.repeatAfterHour)
		end)


		it('should return proper functions called silent', function()

			local res = cmd.silent()

			assert.is_nil(res.afterSec)
			assert.is_nil(res.afterMin)
			assert.is_nil(res.afterHour)
			assert.is_nil(res.withinMin)
			assert.is_nil(res.withinSec)
			assert.is_nil(res.withinHour)

			assert.is_nil(res.silent)
			assert.is_nil(res.forSec)
			assert.is_nil(res.forMin)
			assert.is_nil(res.forHour)
			assert.is_nil(res.repeatAfterSec)
			assert.is_nil(res.repeatAfterMin)
			assert.is_nil(res.repeatAfterHour)
		end)
	end)

	describe('commands', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'device')
		end)

		after_each(function()
			commandArray = {}
			cmd = nil
		end)

		describe('after', function()
			it('should create a proper afterSec command', function()
				cmd.afterSec(1)

				assert.is_same({
					{ ["mySwitch"] = "On AFTER 1 SECONDS" },
				}, commandArray)
			end)

			it('should create a proper afterMin command', function()
				cmd.afterMin(1)

				assert.is_same({
					{ ["mySwitch"] = "On AFTER 60 SECONDS" },
				}, commandArray)
			end)

			it('should create a proper afterHour command', function()
				cmd.afterHour(1)

				assert.is_same({
					{ ["mySwitch"] = "On AFTER 3600 SECONDS" },
				}, commandArray)
			end)
		end)

		describe('for', function()
			it('should create a proper forSec command', function()
				cmd.forSec(1)

				assert.is_same({
					{ ["mySwitch"] = "On FOR 1 SECONDS" },
				}, commandArray)
			end)

			it('should create a proper forMin command', function()
				cmd.forMin(1)

				assert.is_same({
					{ ["mySwitch"] = "On FOR 60 SECONDS" },
				}, commandArray)
			end)

			it('should create a proper forHour command', function()
				cmd.forHour(1)

				assert.is_same({
					{ ["mySwitch"] = "On FOR 3600 SECONDS" },
				}, commandArray)
			end)
		end)

		describe('within', function()
			it('should create a proper withinSec command', function()
				cmd.withinSec(1)

				assert.is_same({
					{ ["mySwitch"] = "On RANDOM 1 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper withinMin command', function()
				cmd.withinMin(1)

				assert.is_same({
					{ ["mySwitch"] = "On RANDOM 60 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper withinHour command', function()
				cmd.withinHour(1)

				assert.is_same({
					{ ["mySwitch"] = "On RANDOM 3600 SECONDS" }
				}, commandArray)
			end)
		end)

		describe('silent', function()
			it('should create a proper silent command for a device', function()
				cmd.silent()

				assert.is_same({
					{ ["mySwitch"] = "On NOTRIGGER" }
				}, commandArray)
			end)

			it('should create a proper silent command for a variable', function()

				commandArray = {}

				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'variable')

				cmd.silent()

				assert.is_same({
					{ ["mySwitch"] = "On" }
				}, commandArray)
			end)

			it('should create a proper silent command for a variable', function()

				commandArray = {}

				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'variable')

				assert.is_same({
					{ ["mySwitch"] = "On TRIGGER" }
				}, commandArray)
			end)

			it('should create a proper silent command for a updatedevice', function()

				commandArray = {}

				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'updatedevice')

				cmd.silent()

				assert.is_same({
					{ ["mySwitch"] = "On" }
				}, commandArray)
			end)

			it('should create a proper silent command for a updatedevice', function()

				commandArray = {}

				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'updatedevice')

				assert.is_same({
					{ ["mySwitch"] = "On TRIGGER" }
				}, commandArray)
			end)
		end)

		describe('repeatAfterXXX', function()

			it('should create a proper repeatAfterSec command', function()
				cmd.repeatAfterSec(1)

				assert.is_same({
					{ ["mySwitch"] = "On REPEAT 2 INTERVAL 1 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper repeatAfterMin command', function()
				cmd.repeatAfterMin(1, 4)

				assert.is_same({
					{ ["mySwitch"] = "On REPEAT 5 INTERVAL 60 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper repeatAfterHour command', function()
				cmd.repeatAfterHour(1, 2)

				assert.is_same({
					{ ["mySwitch"] = "On REPEAT 3 INTERVAL 3600 SECONDS" }
				}, commandArray)
			end)

		end)

		describe('checkState', function()

			it('should not issue a command if the current state is the same as the target state', function()
				commandArray = {}
				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'device', 'On')

				cmd.afterMin(2).checkFirst().forMin(2)

				assert.is_same({
					{  }
				}, commandArray)

			end)

			it('should issue a command if the current state is not the same as the target state', function()
				commandArray = {}
				local cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'device', 'Off')

				cmd.afterMin(2).checkFirst().forMin(2)

				assert.is_same({
					{ ['mySwitch'] = 'On AFTER 120 SECONDS FOR 120 SECONDS' }
				}, commandArray)
			end)


		end)

		describe('combined', function()

			it('should create a combined command', function()

				cmd.afterMin(2).forHour(2).silent().repeatAfterSec(2, 10)

				assert.is_same({
					{ ["mySwitch"] = 'On AFTER 120 SECONDS FOR 7200 SECONDS NOTRIGGER REPEAT 11 INTERVAL 2 SECONDS' }
				}, commandArray)

			end)

		end)

	end)

end)
