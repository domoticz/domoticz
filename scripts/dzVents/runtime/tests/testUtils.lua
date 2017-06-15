local _ = require 'lodash'

--package.path = package.path .. ";../?.lua"

local scriptPath = ''
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'


local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

describe('event helpers', function()
	local utils

	setup(function()
		_G.logLevel = 1
		_G.log = function() end
		_G.globalvariables = {
			Security = 'sec',
			['radix_separator'] = '.',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080'
		}
		utils = require('Utils')
	end)

	teardown(function()
		utils = nil
	end)

	describe("Logging", function()

		it('should print using the global log function', function()
			local printed
			utils.print = function(msg)
				printed = msg
			end

			utils.log('abc', utils.LOG_ERROR)
			assert.is_same('Error: abc', printed)
		end)

		it('shoud log INFO by default', function()
			local printed

			utils.print = function(msg)
				printed = msg
			end

			_G.logLevel = utils.LOG_INFO

			utils.log('something')

			assert.is_same('Info:  something', printed)
		end)

		it('shoud not log above level', function()
			local printed
			utils.print = function(msg)
				printed = msg
			end

			_G.logLevel = utils.LOG_INFO

			utils.log('something', utils.LOG_DEBUG)
			assert.is_nil(printed)

			_G.logLevel = utils.LOG_ERROR
			utils.log('error', utils.LOG_INFO)
			assert.is_nil(printed)

			_G.logLevel = 0
			utils.log('error', utils.LOG_ERROR)
			assert.is_nil(printed)
		end)
	end)


	it('should return true if a file exists', function()
		assert.is_true(utils.fileExists('testfile'))
	end)

	it('should return false if a file does not exist', function()
		assert.is_false(utils.fileExists('blatestfile'))
	end)

end)
