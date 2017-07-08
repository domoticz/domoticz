return {

	baseType = 'device',

	name = 'Lux device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Lux' and device.deviceSubType == 'Lux')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateLux')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: lux

		-- set the lux value
		device['lux'] = tonumber(device.rawData[1])

		device['updateLux'] = function (lux)
			device.update(0, lux)
		end

	end

}