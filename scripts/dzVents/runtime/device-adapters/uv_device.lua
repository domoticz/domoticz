return {

	baseType = 'device',

	name = 'UV device adapter',

	matches = function (device)
		return (device.deviceType == 'UV')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.uv = tonumber(device.rawData[1])

		function device.updateUV(uv)
			local value = tostring(uv) .. ';0'
			device.update(0, value)
		end
	end

}