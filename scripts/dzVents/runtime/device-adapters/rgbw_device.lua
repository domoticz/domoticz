local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'RGB(W) device adapter',

	matches = function (device, adapterManager)
		return false -- this should all be handled by the switch adapter..
--		local res = (device.deviceType == 'Lighting 2' or device.deviceType == 'Lighting Limitless/Applamp')
--		if (not res) then
--			adapterManager.addDummyMethod(device, 'switchOn')
--			adapterManager.addDummyMethod(device, 'switchOff')
--			adapterManager.addDummyMethod(device, 'dimTo')
--			adapterManager.addDummyMethod(device, 'toggleSwitch')
--		end
--		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: maxDimLevel

		function device.toggleSwitch()
			local current, inv
			if (device.state ~= nil) then
				current = adapterManager.states[string.lower(device.state)]
				if (current ~= nil) then
					inv = current.inv
					if (inv ~= nil) then
						return TimedCommand(domoticz, device.name, inv)
					end
				end
			end
			return nil
		end

		if (device.level == nil and data.data._nValue ~= nil) then
			-- rgb devices get their level in _nValue
			device.level = data.data._nValue
		end

		function device.switchOn()
			return TimedCommand(domoticz, device.name, 'On')
		end

		function device.switchOff()
			return TimedCommand(domoticz, device.name, 'Off')
		end

		function device.dimTo(percentage)
			return TimedCommand(domoticz, device.name, 'Set Level ' .. tostring(percentage))
		end

	end

}