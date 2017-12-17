return {

	baseType = 'device',

	name = 'Voltage device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Voltage')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateVoltage')
		end
		return res
	end,

	process = function (device, data, domoticz, adapterManager)

		-- from date: voltage

		function device.updateVoltage(voltage)
			device.update(0, voltage)
		end
	end

}