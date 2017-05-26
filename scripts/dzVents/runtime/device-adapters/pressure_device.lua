return {

	baseType = 'device',

	name = 'Pressure device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Pressure')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.updatePressure(pressure)
			device.update(0, pressure)
		end

	end

}