return {

	baseType = 'device',

	name = 'Percentage device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Percentage')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.percentage = tonumber(device.rawData[1])

		function device.updatePercentage(percentage)
			device.update(0, percentage)
		end
	end

}