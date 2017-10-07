return {

	baseType = 'device',

	name = 'Electric usage device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Usage' and device.deviceSubType == 'Electric')
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['WhActual'] = tonumber(device.rawData[1]  or 0)

		function device.updateEnergy(energy)
			return device.update(0, energy)
		end

	end

}