local _ = require 'lodash'

local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'

local testData = require('tstData')

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

local xVar = 9
local yVar = 10
local zVar = 11
local aVar = 12
local bVar = 13

describe('variables', function()
	local Variable
	local commandArray = {}

	local domoticz = {
		sendCommand = function(key, value)
			table.insert(commandArray, { [key] = tostring(value) })
		end
	}

	setup(function()
		_G.logLevel = 1

		_G.domoticzData = testData.domoticzData

		commandArray = {}
		Variable = require('Variable')

	end)

	teardown(function()
		Variable = nil
	end)

	it('should instantiate', function()
		local var = Variable(domoticz, testData.domoticzData[xVar])
		assert.is_not_nil(var)
	end)

	it('should have properties', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		assert.is_same(2.3, var.nValue)
		assert.is_same(2.3, var.value)
		assert.is_same('2017-04-18 20:16:23', var.lastUpdate.raw)
	end)

	it('should have cast to number', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		assert.is_same('number', type(var.nValue))
	end)

	it('should set a new value', function()
		local var = Variable(domoticz, testData.domoticzData[yVar])
		var.set('dzVents rocks')
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable:y'] = 'dzVents rocks' }, commandArray[1])
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

end)
