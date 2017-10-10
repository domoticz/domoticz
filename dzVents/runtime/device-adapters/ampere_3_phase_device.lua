return {
	baseType = 'device',
	name = '3-phase Ampere device adapter',
	matches = function(device, adapterManager)
		local res = (device.deviceType == 'Current' and device.deviceSubType == 'CM113, Electrisave')
		return res
	end,
	process = function(device, data, domoticz, utils, adapterManager)

		device.current1 = tonumber(device.rawData[1])
		device.current2 = tonumber(device.rawData[2])
		device.current3 = tonumber(device.rawData[3])

		function device.updateCurrent(current1, current2, current3)
			return device.update(0, tostring(current1) .. ';' .. tostring(current2) .. ';' .. tostring(current3) )
		end
	end
}