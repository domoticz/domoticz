package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local TestTools = require('domoticzTestTools')('8080', true)
local socket = require("socket")

local _ = require 'lodash'

local fsScripts = {'scriptSelectorSwitch.lua'}

describe('Test if selector switch afterSec() works', function ()

	local vdScriptStartIdx, vdScriptEndIdx, vdScriptOK, switchIdx

	setup(function()
		local ok = TestTools.reset()
		assert.is_true(ok)
		ok, dummyIdx = TestTools.createDummyHardware('dummy')

		ok, switchIdx = TestTools.createVirtualDevice(dummyIdx, 'vdSelectorSwitch', 1002)

		ok, vdScriptStartIdx = TestTools.createVirtualDevice(dummyIdx, 'vdScriptStart', 6)
		TestTools.createVirtualDevice(dummyIdx, 'vdScriptEnd', 6)
		ok, vdScriptOKIdx = TestTools.createVirtualDevice(dummyIdx, 'vdScriptOK', 6)

		TestTools.switchSelector(switchIdx, 10)
		socket.sleep(2)

		TestTools.installFSScripts(fsScripts)


	end)

	teardown(function()
		TestTools.cleanupFSScripts(fsScripts)
	end)

	before_each(function()
	end)

	after_each(function() end)

	local dummyIdx

	describe('Start the tests', function()

		it('Should all just work fine', function()
			socket.sleep(2) -- make sure the first lastUpdate check is at least 2 seconds ago
			local ok = TestTools.switch(vdScriptStartIdx, 'On')
			assert.is_true(ok)
		end)

		it('Should have succeeded', function()

			socket.sleep(5) -- the trigger for stage 2 has a delay set to 4 seconds (afterSec(4))

			local ok = false
			local vdOKDevice

			ok, vdOKDevice = TestTools.getDevice(vdScriptOKIdx)
			assert.is_true(ok)
			assert.is_same('On', vdOKDevice['Status'])

		end)

	end)


end);
