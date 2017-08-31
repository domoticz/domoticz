return {

	baseType = 'device',

	name = 'Scale weight device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'Weight')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateWeight')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.weight = tonumber(device.rawData[1])

		function device.updateWeight(weight)
			return device.update(0, tostring(weight))
		end

	end

}