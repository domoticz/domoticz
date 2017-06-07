return {

	baseType = 'device',

	name = 'Custom sensor device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Custom Sensor')

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateCustomSensor')
		end

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: sensorType, sensorUnit

		function device.updateCustomSensor(value)
			device.update(0, value)
		end

	end

}