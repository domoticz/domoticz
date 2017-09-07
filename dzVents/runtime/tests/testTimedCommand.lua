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
			cmd = TimedCommand(domoticz, 'mySwitch', 'On')
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
			assert.is_function(cmd.rpt)
			assert.is_function(cmd.secBetweenRepeat)
			assert.is_function(cmd.minBetweenRepeat)
			assert.is_function(cmd.hourBetweenRepeat)

		end)

		it('should return proper function when called after', function()
			local res = cmd.afterSec(1)
			assert.is_function(res.forSec)
			assert.is_function(res.forMin)
			assert.is_function(res.forHour)
			assert.is_function(res.silent)
			assert.is_function(res.rpt)
			assert.is_function(res.secBetweenRepeat)
			assert.is_function(res.minBetweenRepeat)
			assert.is_function(res.hourBetweenRepeat)

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
			assert.is_function(res.rpt)
			assert.is_function(res.secBetweenRepeat)
			assert.is_function(res.minBetweenRepeat)
			assert.is_function(res.hourBetweenRepeat)

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
			assert.is_function(res.rpt)
			assert.is_function(res.secBetweenRepeat)
			assert.is_function(res.minBetweenRepeat)
			assert.is_function(res.hourBetweenRepeat)

			assert.is_nil(res.silent)
		end)

		it('should return proper functions when called rpt', function()
			local res = cmd.rpt(1)
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
			assert.is_function(res.secBetweenRepeat)
			assert.is_function(res.minBetweenRepeat)
			assert.is_function(res.hourBetweenRepeat)
			assert.is_nil(res.rpt)

		end)

		it('should return proper functions when called between', function()
			local res = cmd.secBetweenRepeat(1)
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
			assert.is_function(res.rpt)

			assert.is_nil(res.secBetweenRepeat)
			assert.is_nil(res.minBetweenRepeat)
			assert.is_nil(res.hourBetweenRepeat)
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
			assert.is_nil(cmd.rpt)
			assert.is_nil(cmd.secBetweenRepeat)
			assert.is_nil(cmd.minBetweenRepeat)
			assert.is_nil(cmd.hourBetweenRepeat)
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
			assert.is_nil(res.rpt)
			assert.is_nil(res.secBetweenRepeat)
			assert.is_nil(res.minBetweenRepeat)
			assert.is_nil(res.hourBetweenRepeat)
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
			assert.is_nil(res.rpt)
			assert.is_nil(res.secBetweenRepeat)
			assert.is_nil(res.minBetweenRepeat)
			assert.is_nil(res.hourBetweenRepeat)
		end)

	end)

	describe('variable', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On', 'updatedevice')
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
			assert.is_nil(cmd.rpt)
			assert.is_nil(cmd.secBetweenRepeat)
			assert.is_nil(cmd.minBetweenRepeat)
			assert.is_nil(cmd.hourBetweenRepeat)
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
			assert.is_nil(res.rpt)
			assert.is_nil(res.secBetweenRepeat)
			assert.is_nil(res.minBetweenRepeat)
			assert.is_nil(res.hourBetweenRepeat)
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
			assert.is_nil(res.rpt)
			assert.is_nil(res.secBetweenRepeat)
			assert.is_nil(res.minBetweenRepeat)
			assert.is_nil(res.hourBetweenRepeat)
		end)
	end)

	describe('commands', function()

		before_each(function()
			cmd = TimedCommand(domoticz, 'mySwitch', 'On')
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
			it('should create a proper silent command', function()
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

		describe('repeat', function()
			it('should create a proper repeat command', function()
				cmd.rpt(2)

				assert.is_same({
					{ ["mySwitch"] = "On REPEAT 3" }
				}, commandArray)
			end)
		end)

		describe('xxxBetweenRepeat', function()

			it('should create a proper secBetweenRepeat command', function()
				cmd.secBetweenRepeat(1)

				assert.is_same({
					{ ["mySwitch"] = "On INTERVAL 1 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper minBetweenRepeat command', function()
				cmd.minBetweenRepeat(1)

				assert.is_same({
					{ ["mySwitch"] = "On INTERVAL 60 SECONDS" }
				}, commandArray)
			end)

			it('should create a proper hourBetweenRepeat command', function()
				cmd.hourBetweenRepeat(1)

				assert.is_same({
					{ ["mySwitch"] = "On INTERVAL 3600 SECONDS" }
				}, commandArray)
			end)

		end)

		describe('combined', function()

			it('should create a combined command', function()

				cmd.afterMin(2).forHour(2).silent().rpt(2).secBetweenRepeat(2)

				assert.is_same({
					{ ["mySwitch"] = 'On AFTER 120 SECONDS FOR 7200 SECONDS NOTRIGGER REPEAT 3 INTERVAL 2 SECONDS' }
				}, commandArray)

			end)

		end)

	end)

end)
