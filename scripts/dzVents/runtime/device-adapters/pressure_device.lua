return {

	baseType = 'device',

	name = 'Pressure device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Pressure')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updatePressure')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: pressure

		function device.updatePressure(pressure)
			device.update(0, pressure)
		end

	end

}