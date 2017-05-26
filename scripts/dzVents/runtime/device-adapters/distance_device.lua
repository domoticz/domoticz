return {

	baseType = 'device',

	name = 'Distance device adapter',

	matches = function (device)
		return (device.deviceSubType == 'Distance')
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.distance = tonumber(device.rawData[1])

		function device.updateDistance(distance)
			--[[
			 distance in cm or inches, can be in decimals. For example 12.6
			 ]]
			device.update(0, distance)
		end

	end

}