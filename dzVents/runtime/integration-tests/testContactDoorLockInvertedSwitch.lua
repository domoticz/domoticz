package.path = package.path ..
		";../?.lua;../device-adapters/?.lua;./data/?.lua;../../../scripts/dzVents/generated_scripts/?.lua;" ..
		"../../../scripts/lua/?.lua"

local TestTools = require('domoticzTestTools')('8080', true)
local socket = require("socket")

local _ = require 'lodash'

local fsScripts = {'scriptContactDoorLockInvertedSwitch.lua'}

describe('Test if contact and door lock inverted are triggered by event system', function ()

	local contactIdx, doorLockIdx, vdScriptOK, vdTriggerIdx

	setup(function()
		local ok = TestTools.reset()
		assert.is_true(ok)
		ok, dummyIdx = TestTools.createDummyHardware('dummy')

		ok, contactIdx = TestTools.createVirtualDevice(dummyIdx, 'vdContact', 6)
		ok = TestTools.updateSwitch(contactIdx, 'vdContact', '', 2)

		ok, doorLockIdx = TestTools.createVirtualDevice(dummyIdx, 'vdDoorLockInverted', 6)
		ok = TestTools.updateSwitch(doorLockIdx, 'vdDoorLockInverted', '', 20)

		ok, vdTriggerIdx = TestTools.createVirtualDevice(dummyIdx, 'vdTrigger', 6)
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
			local ok = TestTools.switch(vdTriggerIdx, 'On')
			assert.is_true(ok)
		end)

		it('Should have succeeded', function()

			socket.sleep(3)

			local ok = false
			local vdOKDevice

			ok, vdTrigger = TestTools.getDevice(vdTriggerIdx)
			assert.is_true(ok)
			assert.is_same('Off', vdTrigger['Status'])

		end)

	end)


end);
