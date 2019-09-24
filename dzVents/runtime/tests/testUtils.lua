local _ = require 'lodash'

--package.path = package.path .. ";../?.lua"

local scriptPath = ''
package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;../../../scripts/lua/?.lua;'


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
			assert.is_same('Error: (' .. utils.DZVERSION .. ') abc', printed)
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

	it('should right pad a string', function()
		assert.is_same(utils.rightPad('string',7),'string ')
		assert.is_same(utils.rightPad('string',7,'@'),'string@')
		assert.is_same(utils.rightPad('string',2),'string')
	end)
    
	it('should left pad a string', function()
		assert.is_same(utils.leftPad('string',7),' string')
		assert.is_same(utils.leftPad('string',7,'@'),'@string')
		assert.is_same(utils.leftPad('string',2),'string')
	end)
    
    
	it('should center and pad a string', function()
		assert.is_same(utils.centerPad('string',8),' string ')
		assert.is_same(utils.centerPad('string',8,'@'),'@string@')
		assert.is_same(utils.centerPad('string',2),'string')
	end)
    
        
	it('should pad a number with leading zeros', function()
		assert.is_same(utils.leadingZeros(99,3),'099')
		assert.is_same(utils.leadingZeros(999,2),'999')
	end)

	it('should return nil for os.execute (echo)', function()
		assert.is_nil(utils.osExecute('echo test > testfile.out'))
	end)

	it('should return nil for os.execute (rm)', function()
		assert.is_nil(utils.osExecute('rm testfile.out'))
	end)

	it('should return false if a file does not exist', function()
		assert.is_false(utils.fileExists('blatestfile'))
	end)

	it('should convert a json to a table', function()
		local json = '{ "a": 1 }'
		local t = utils.fromJSON(json)
		assert.is_same(1, t['a'])
	end)

	it('should convert a json to a table or fallback to fallback', function()
		local json = '{ "a": 1 }'
		local t = utils.fromJSON(json, fallback)
		assert.is_same(1, t['a'])

		json = nil
		local fallback  = { a=1 }
		local t = utils.fromJSON(json, fallback)
		assert.is_same(1, t['a'])

		json = nil
		fallback  = nil
		local t = utils.fromJSON(json, fallback)
		assert.is_nil(t)

	end)

	it('should convert a table to json', function()
		local t = { a= 1 }
		local res = utils.toJSON(t)
		assert.is_same('{"a":1}', res)
	end)
	
	it('should dump a table to log', function()
		local t = { a=1,b=2,c={d=3,e=4, "test"} }
		local res = utils.dumpTable(t,"> ")
		assert.is_nil(res)
	end)

	it('should split a string ', function()
		assert.is_same(utils.stringSplit("A-B-C", "-")[2],"B")
		assert.is_same(utils.stringSplit("I forgot to include this in Domoticz.lua")[7],"Domoticz.lua")
	end)

	it('should handle inTable ', function()
		assert.is_same(utils.inTable({ testVersion = "2.5" }, 2.5 ), "value")
		assert.is_same(utils.inTable({ testVersion = "2.5" }, "testVersion"), "key")
		assert.is_false(utils.inTable({ testVersion = "2.5" }, 2.4 ), false)
	end)

end)
