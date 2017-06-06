return {

	baseType = 'device',

	name = 'Percentage device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Percentage')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updatePercentage')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.percentage = tonumber(device.rawData[1])

		function device.updatePercentage(percentage)
			device.update(0, percentage)
		end
	end

}