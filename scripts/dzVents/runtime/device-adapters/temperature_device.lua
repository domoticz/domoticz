return {

	baseType = 'device',

	name = 'Temperature device adapter',

	matches = function (device)
		return (device.deviceType == 'Temp')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.updateTemperature(temperature)
			device.update(0, temperature)
		end

	end


}