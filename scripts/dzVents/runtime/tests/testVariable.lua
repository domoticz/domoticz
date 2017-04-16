local _ = require 'lodash'

package.path = package.path .. ";../?.lua"

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

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

		_G['uservariables_lastupdate'] = {
			['myVar'] = '2016-03-20 12:23:00'
		}
		commandArray = {}
		Variable = require('Variable')

	end)

	teardown(function()
		Variable = nil
	end)

	it('should instantiate', function()
		local var = Variable(domoticz, 'myVar', 10)
		assert.is_not_nil(var)
	end)

	it('should have properties', function()
		local var = Variable(domoticz, 'myVar', '10')
		assert.is_same(10, var.nValue)
		assert.is_same('10', var.value)
		assert.is_same('2016-03-20 12:23:00', var.lastUpdate.raw)
	end)

	it('should have cast to number', function()
		local var = Variable(domoticz, 'myVar', '10')
		assert.is_same('number', type(var.nValue))
	end)

	it('should set a new value', function()
		local var = Variable(domoticz, 'myVar', '10')
		var.set('dzVents rocks')
		assert.is_same(1, _.size(commandArray))
		assert.is_same({ ['Variable:myVar'] = 'dzVents rocks' }, commandArray[1])
	end)

	it('should not fail when trying to cast a string', function()
		local var = Variable(domoticz, 'myVar', 'some value')
		assert.is_nil(var.nValue)
		assert.is_same('some value', var.value)
	end)

end)
