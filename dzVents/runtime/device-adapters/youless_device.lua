return {

	baseType = 'device',

	name = 'Youless device adapter',

	matches = function(device, adapterManager)
		local res = (device.deviceType == 'YouLess Meter' and device.deviceSubType == 'YouLess counter')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateYouless')
		end
		return res
	end,

	process = function(device, data, domoticz, utils, adapterManager)
		-- from data: current, Kwh.
		device.counterDeliveredTotal = tonumber(device.rawData[1])
		device.powerYield = tonumber(device.rawData[2])
		device.counterDeliveredToday = tonumber((string.gsub(device.counterToday, "kWh", ""))) * 1000

		function device.updateYouless(total, actual)
			local value = tostring(total) .. ';' ..
					tostring(actual)
			return device.update(0, value)
		end
	end
}
