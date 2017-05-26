return {

	baseType = 'device',

	name = 'Custom sensor device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Custom Sensor')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.updateCustomSensor(value)
			device.update(0, value)
		end

	end

}