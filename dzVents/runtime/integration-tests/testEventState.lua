package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local TestTools = require('domoticzTestTools')('8080', true)
local socket = require("socket")

local _ = require 'lodash'

local fsScripts = {'scriptTestEventState.lua'}

describe('Integration test', function ()

	local vdScriptStartIdx, vdScriptEndIdx, vdScriptOK

	setup(function()
		local ok = TestTools.reset()
		assert.is_true(ok)
		ok, dummyIdx = TestTools.createDummyHardware('dummy')
		ok, idx = TestTools.createVirtualDevice(dummyIdx, 'vdRepeatSwitch', 6)
		ok, vdScriptStartIdx = TestTools.createVirtualDevice(dummyIdx, 'vdScriptStart', 6)
		ok, vdScriptEndIdx = TestTools.createVirtualDevice(dummyIdx, 'vdScriptEnd', 6)
		ok, vdScriptOKIdx = TestTools.createVirtualDevice(dummyIdx, 'vdScriptOK', 6)

		TestTools.installFSScripts(fsScripts)

		socket.sleep(1)
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
			local ok = TestTools.switch(vdScriptStartIdx, 'On')
			assert.is_true(ok)
		end)

		it('Should have succeeded', function()

			socket.sleep(18) -- the trigger for stage 2 has a delay set to 4 seconds (afterSec(4))

			local ok = false
			local vdOKDevice

			ok, vdOKDevice = TestTools.getDevice(vdScriptOKIdx)
			assert.is_true(ok)
			assert.is_same('On', vdOKDevice['Status'])

		end)

	end)


end);
