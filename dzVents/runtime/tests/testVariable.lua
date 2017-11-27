local _ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'

local testData = require('tstData')

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

local xVar = 11
local yVar = 12
local zVar = 13
local aVar = 14
local bVar = 15
local spacedVar = 16

describe('variables', function()
	local Variable
	local commandArray = {}

	local domoticz = {
		settings = { ['Domoticz url'] = 'url' },
		sendCommand = function(command, value)
			table.insert(commandArray, { [command] = value })
			return commandArray[#commandArray], command, value
		end,
	}

	setup(function()
		_G.logLevel = 1

		_G.domoticzData = testData.domoticzData

		_G.globalvariables = {
			Security = 'sec',
			['script_path'] = scriptPath,
			['currentTime'] = '2017-08-17 12:13:14.123'
		}

		TimedCommand = require('TimedCommand')

		commandArray = {}
		Variable = require('Variable')

	end)

	before_each(function()
		commandArray = {}
	end)

	teardown(function()
		Variable = nil
	end)

	it('should instantiate', function()
		local var = Variable(domoticz, testData.domoticzData[xVar])
		assert.is_not_nil(var)
		assert.is_false(var.isHTTPResponse)
		assert.is_true(var.isVariable)
		assert.is_false(var.isTimer)
		assert.is_false(var.isScene)
		assert.is_false(var.isDevice)
		assert.is_false(var.isGroup)
		assert.is_false(var.isSecurity)
	end)

	it('should have properties', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		assert.is_same(2.3, var.nValue)
		assert.is_same(2.3, var.value)
		assert.is_same('2017-04-18 20:16:23', var.lastUpdate.raw)
		assert.is_same('y', var.name)
	end)

	it('should have cast to number', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		assert.is_same('number', type(var.nValue))
	end)


	it('should set a new float value', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		var.set(12.5)
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 2, value='12.5', _trigger=true } }, commandArray[1])
	end)

	it('should set a new integer value', function()
		local var = Variable(domoticz, testData.domoticzData[xVar])
		var.set(12)
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 1, value='12', _trigger=true } }, commandArray[1])
	end)

	it('should set a new string value', function()
		local var = Variable(domoticz, testData.domoticzData[zVar])
		var.set('dzVents')
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 3, value='dzVents', _trigger=true } }, commandArray[1])
	end)

	it('should set a new date value', function()
		local var = Variable(domoticz, testData.domoticzData[aVar])
		var.set('12/12/2012')
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 4, value='12/12/2012', _trigger=true } }, commandArray[1])
	end)

	it('should set a new time value', function()
		local var = Variable(domoticz, testData.domoticzData[bVar])
		var.set('12:34')
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 5, value='12:34', _trigger=true } }, commandArray[1])
	end)

	it('should not fail when trying to cast a string', function()
		local var = Variable(domoticz, testData.domoticzData[zVar])
		assert.is_nil(var.nValue)
		assert.is_same('some value', var.value)
	end)

	it('should have different types', function()
		local var = Variable(domoticz, testData.domoticzData[zVar])
		assert.is_same('string', var.type)
		assert.is_same('string', type(var.value))
	end)

	it('should have a date type', function()
		local var = Variable(domoticz, testData.domoticzData[aVar])
		assert.is_same('date', var.type)
		assert.is_same(2017, var.date.year)
		assert.is_same(3, var.date.day)
		assert.is_same(12, var.date.month)
	end)

	it('should have a time type', function()
		local var = Variable(domoticz, testData.domoticzData[bVar])
		assert.is_same('time', var.type)
		assert.is_same(19, var.time.hour)
		assert.is_same(34, var.time.min)
	end)

	it('should set a make a timed command', function()
		local var = Variable(domoticz, testData.domoticzData[zVar])
		var.set('dzVents').afterSec(5)
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable'] = { idx = 3, value='dzVents', _trigger = true, _after = 5 } }, commandArray[1])
	end)

	it('should have a cancelQueuedCommands method', function()
		local var = Variable(domoticz, testData.domoticzData[zVar])

		var.cancelQueuedCommands()
		assert.is_same({
			{ ['Cancel'] = { type = 'variable', idx = 3 } }
		}, commandArray)
	end)

end)
