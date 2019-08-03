return {

	baseType = 'device',

	name = 'Thermostat setpoint device adapter',

	matches = function (device, adapterManager)
		local res = device.deviceSubType == 'SetPoint' and device.hardwareTypeValue ~= 20 -- rule out opentherm
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSetPoint')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.setPoint = tonumber(device.rawData[1] or 0)

		function device.updateSetPoint(setPoint)
			return device.update(0, setPoint)
		end

	end

}
