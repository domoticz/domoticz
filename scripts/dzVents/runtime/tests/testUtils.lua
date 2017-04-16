local _ = require 'lodash'

package.path = package.path .. ";../?.lua"

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

describe('event helpers', function()
	local utils

	setup(function()
		_G.logLevel = 1
		_G.log = function()	end
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
			assert.is_same('abc', printed)
		end)

		it('shoud log INFO by default', function()
			local printed

			utils.print = function(msg)
				printed = msg
			end

			_G.logLevel = utils.LOG_INFO

			utils.log('something')

			assert.is_same('something', printed)
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

	it('should return the devices.lua path', function()
		assert.is_same( '../../devices.lua', utils.getDevicesPath())
	end)

	it('should fetch the http data', function()

		local cmd

		utils.osExecute = function(c)
			cmd = c
		end

		utils.requestDomoticzData('0.0.0.0', '8080')

		local expected = "{ echo 'return ' ; curl 'http://0.0.0.0:8080/json.htm?type=devices&displayhidden=1&filter=all&used=true' -s ; }  | sed 's/],/},/' | sed 's/   \"/   [\"/' | sed 's/         \"/         [\"/' | sed 's/\" :/\"]=/' | sed 's/: \\[/: {/' | sed 's/= \\[/= {/' > ../../devices.lua.tmp 2>/dev/null &"

		assert.is_same(expected, cmd)
	end)

end)
