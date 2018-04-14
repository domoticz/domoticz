local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Evohome device adapter',

	matches = function (device, adapterManager)
		local res = (
			device.hardwareTypeValue == 39 or
			device.hardwareTypeValue == 40 or
			device.hardwareTypeValue == 106 or
			device.hardwareTypeValue == 75
		)

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSetPoint')
		end

		return res

	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.setPoint = tonumber(device.rawData[1] or 0)

		function device.updateSetPoint(setPoint, mode, untilDate)
			return TimedCommand(domoticz,
				'SetSetPoint:' ..
				tostring(device.id),
			   tostring(setPoint) ..
			   '#' .. tostring(mode) ..
			   '#' .. tostring(untilDate) , 'setpoint')
		end

	end

}
