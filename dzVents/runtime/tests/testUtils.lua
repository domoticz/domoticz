local _ = require 'lodash'

--package.path = package.path .. ";../?.lua"

local scriptPath = ''
package.path = ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;../../../scripts/lua/?.lua;' .. package.path

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

		_G.domoticzData =
		{
			 [1] = { ["id"] = 1, ["baseType"] = "device", ["name"] = "ExistingDevice" },
			 [2] = { ["id"] = 2, ["baseType"] = "group", ["name"] = "ExistingGroup" },
			 [3] = { ["id"] = 3, ["baseType"] = "scene", ["name"] = "ExistingScene" },
			 [4] = { ["id"] = 4, ["baseType"] = "uservariable", ["name"] = "ExistingVariable" },
			 [5] = { ["id"] = 5, ["baseType"] = "camera", ["name"] = "ExistingCamera" },
			 [6] = { ["id"] = 6, ["baseType"] = "hardware", ["name"] = "ExistingHardware" },
		}

		utils = require('Utils')
	end)

	teardown(function()
		utils = nil
		 _G.domoticzData = nil
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

		it('should log INFO by default', function()
			local printed

			utils.print = function(msg)
				printed = msg
			end

			_G.logLevel = utils.LOG_INFO

			utils.log('something')

			assert.is_same('Info: something', printed)
		end)

		it('should log print with requested marker', function()
			local printed

			utils.print = function(msg)
				printed = msg
			end

			_G.logLevel = utils.LOG_INFO
			utils.log('something')
			assert.is_same('Info: something', printed)

			_G.moduleLabel = 'testUtils'
			utils.setLogMarker()
			utils.log('something')
			assert.is_same('Info: testUtils: something', printed)

			utils.setLogMarker('Busted')
			utils.log('something')
			assert.is_same('Info: Busted: something', printed)
		end)

		it('should not log above level', function()
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
			utils.log('error', utils.LOG_WARNING)
			assert.is_nil(printed)

			_G.logLevel = 0
			utils.log('error', utils.LOG_ERROR)
			assert.is_nil(printed)
		end)
	end)

	describe("various utils", function()

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

		it('should return nil for osexecute (echo)', function()
			assert.is_nil(utils.osExecute('echo test > testfile.out'))
		end)

		it('should return nil for os.execute (rm)', function()
			assert.is_nil(utils.osExecute('rm testfile.out'))
		end)

		it('should return nil for osCommand (echo)', function()
			local res, rc = utils.osCommand('echo test > testfile.out')
			assert.is_same(rc, 0)
			assert.is_same(res, '')
		end)

		it('should return nil for osCommand (rm)', function()
			local res, rc = utils.osCommand('rm -fv nofile.nofile ')
			assert.is_same(rc, 0)
			assert.is_same(res, '')
			local res, rc = utils.osCommand('rm -v testfile.out')
			assert.is_same(rc, 0)
			assert.is_same(res:sub(1,4), "remo")

		end)

		it('should return false if a file does not exist', function()
			assert.is_false(utils.fileExists('blatestfile'))
		end)

		it('should return false if a device does not exist and id or name when it does', function()
			local device = { id = 1}
			local noDevice = { id = 2}
			assert.is_false(utils.deviceExists('noDevice'))
			assert.is_false(utils.deviceExists(noDevice))
			assert.is_false(utils.deviceExists(2))
			assert.is_true(utils.deviceExists('ExistingDevice') == 1 )
			assert.is_true(utils.deviceExists(1) == 'ExistingDevice' )
			assert.is_true(utils.deviceExists(device) == 1)
		end)

		it('should return false if a group does not exist and id or name when it does', function()
			local group = { id = 2}
			local noGroup = { id = 3}
			assert.is_false(utils.groupExists('noGroup'))
			assert.is_false(utils.groupExists(noGroup))
			assert.is_false(utils.groupExists(3))
			assert.is_true(utils.groupExists('ExistingGroup') == 2 )
			assert.is_true(utils.groupExists(2) == 'ExistingGroup' )
			assert.is_true(utils.groupExists(group) == 2)
		end)

		it('should return false if a scene does not exist and id or name when it does', function()
			local scene = { id = 3}
			local noScene = { id = 4}
			assert.is_false(utils.sceneExists('noScene'))
			assert.is_false(utils.sceneExists(noScene))
			assert.is_false(utils.sceneExists(4))
			assert.is_true(utils.sceneExists('ExistingScene') == 3 )
			assert.is_true(utils.sceneExists(3) == 'ExistingScene' )
			assert.is_true(utils.sceneExists(scene) == 3)
		end)

		it('should return false if a variable does not exist and id or name when it does', function()
			local variable = { id = 4}
			local noVariable = { id = 5}
			assert.is_false(utils.variableExists('noVariable'))
			assert.is_false(utils.variableExists(noVariable))
			assert.is_false(utils.variableExists(5))
			assert.is_true(utils.variableExists('ExistingVariable') == 4 )
			assert.is_true(utils.variableExists(4) == 'ExistingVariable' )
			assert.is_true(utils.variableExists(variable) == 4)
		end)

		it('should return false if a camera does not exist and id or name when it does', function()
			local camera = { id = 5}
			local noCamera = { id = 6}
			assert.is_false(utils.cameraExists('noCamera'))
			assert.is_false(utils.cameraExists(noCamera))
			assert.is_false(utils.cameraExists(6))
			assert.is_true(utils.cameraExists('ExistingCamera') == 5 )
			assert.is_true(utils.cameraExists(5) == 'ExistingCamera' )
			assert.is_true(utils.cameraExists(camera) == 5)
		end)

		it('should return false if hardware does not exist and id or name when it does', function()
			assert.is_false(utils.hardwareExists('noHardware'))
			assert.is_false(utils.hardwareExists(7))
			assert.is_true(utils.hardwareExists('ExistingHardware') == 6 )
			assert.is_true(utils.hardwareExists(6) == 'ExistingHardware' )
		end)

		it('should convert a json to a table', function()
			local json = '{ "a": 1 }'
			local t = utils.fromJSON(json)
			assert.is_same(1, t['a'])
		end)

		it('should recognize an xml string', function()
			local xml = '<testXML>What a nice feature!</testXML>'
			assert.is_true(utils.isXML(xml))

			local xml = nil
			assert.is_nil(utils.isXML(xml))

			local xml = '<testXML>What a bad XML!</testXML> xml'
			assert.is_nil(utils.isXML(xml))

			local xml = '{ wrong XML }'
			local content = 'application/xml'
			fallback = nil
			assert.is_true(utils.isXML(xml, content))

		end)

		it('should recognize a json string', function()
			local json = '[{ "test": 12 }]'
			assert.is_true(utils.isJSON(json))

			local json = '{ "test": 12 }'
			assert.is_true(utils.isJSON(json))

			local json = nil
			assert.is_false(utils.isJSON(json))

			local json = '< wrong XML >'
			local content = 'application/json'
			fallback = nil
			assert.is_true(utils.isJSON(json, content))

		end)

		it('should convert a json string to a table or fallback to fallback', function()
			local json = '{ "a": 1 }'
			local t = utils.fromJSON(json, fallback)
			assert.is_same(1, t['a'])

			local json = '[{"obj":"Switch","act":"On" }]'
			local t = utils.fromJSON(json, fallback)
			assert.is_same('Switch', t[1].obj)

			json = nil
			local fallback = { a=1 }
			local t = utils.fromJSON(json, fallback)
			assert.is_same(1, t['a'])

			json = nil
			fallback = nil
			local t = utils.fromJSON(json, fallback)
			assert.is_nil(t)

		end)

		it('should convert an xml string to a table or fallback to fallback', function()
			local xml = '<testXML>What a nice feature!</testXML>'
			local t = utils.fromXML(xml, fallback)
			assert.is_same('What a nice feature!', t.testXML)

			local xml = nil
			local fallback = { a=1 }
			local t = utils.fromXML(xml, fallback)
			assert.is_same(1, t['a'])

			local xml = nil
			fallback = nil
			local t = utils.fromXML(xml, fallback)
			assert.is_nil(t)

		end)

		it('should convert a table to json', function()
			local t = {
				a = 1,
				b = function()
					print('This should do nothing')
				end
			}
			local res = utils.toJSON(t)
			assert.is_same('{"a":1,"b":"Function"}', res)
		end)

		it('should convert a table to xml', function()
			local t = { a= 1 }
			local res = utils.toXML(t, 'busted')
			assert.is_same('<busted>\n<a>1</a>\n</busted>\n', res)
		end)

		it('should convert a string or number to base64Code', function()
			local res = utils.toBase64('Busted in base64')
			assert.is_same('QnVzdGVkIGluIGJhc2U2NA==', res)

			local res = utils.toBase64(123.45)
			assert.is_same('MTIzLjQ1', res)

			local res = utils.toBase64(1234567890)
			assert.is_same('MTIzNDU2Nzg5MA==', res)

		end)

		it('should send errormessage when sending table to toBase64', function()

			utils.log = function(msg)
				printed = msg
			end

			local t = { a= 1 }

			local res = utils.toBase64(t)
			assert.is_same('toBase64: parm should be a number or a string; you supplied a table', printed)

		end)

		it('should decode a base64 encoded string', function()
			local res = utils.fromBase64('QnVzdGVkIGluIGJhc2U2NA==')
			assert.is_same('Busted in base64', res)
		end)

		it('should send errormessage when sending table to fromBase64', function()

			utils.log = function(msg)
				printed = msg
			end

			local t = { a= 1 }

			local res = utils.fromBase64(t)
			assert.is_same('fromBase64: parm should be a string; you supplied a table', printed)

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

		it('should fuzzy match  a string ', function()
			assert.is_same(utils.fuzzyLookup('HtpRepsonse','httpResponse'),3)
			assert.is_same(utils.fuzzyLookup('httpResponse','httpResponse'),0)
			local validEventTypes = 'devices,timer,security,customEvents,system,httpResponses,scenes,groups,variables,devices'
			assert.is_same(utils.fuzzyLookup('CutsomeEvent',utils.stringSplit(validEventTypes,',')),'customEvents')
		end)

		it('should match a string with Lua magic chars', function()
			assert.is_same(string.sMatch("testing (A-B-C) testing","(A-B-C)"), "(A-B-C)")
			assert.is_not(string.match("testing (A-B-C) testing", "(A-B-C)"), "(A-B-C)")
		end)

		it('should handle inTable ', function()
			assert.is_same(utils.inTable({ testVersion = "2.5" }, 2.5 ), "value")
			assert.is_same(utils.inTable({ testVersion = "2.5" }, "testVersion"), "key")
			assert.is_not(utils.inTable({ testVersion = "2.5" }, 2.4 ), false)
		end)
	end)
end)
