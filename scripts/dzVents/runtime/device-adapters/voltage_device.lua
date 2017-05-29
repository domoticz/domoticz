return {

	baseType = 'device',

	name = 'Voltage device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Voltage')
	end,

	process = function (device, data, domoticz, adapterManager)

		function device.updateVoltage(voltage)
			device.update(0, voltage)
		end
	end

}