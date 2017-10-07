return {

	baseType = 'device',

	name = 'Temperature device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Temp')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateTemperature')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: temperature

		function device.updateTemperature(temperature)
			return device.update(0, temperature)
		end
	end

}