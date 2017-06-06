return {

	baseType = 'device',

	name = 'P1 smart meter energy device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'P1 Smart Meter' and device.deviceSubType == 'Energy')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateP1')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['WhActual'] = tonumber(device.rawData[5]) or 0

		device['usage1'] = tonumber(device.rawData[1])
		device['usage2'] = tonumber(device.rawData[2])
		device['return1'] = tonumber(device.rawData[3])
		device['return2'] = tonumber(device.rawData[4])
		device['usage'] = tonumber(device.rawData[5])
		device['usageDelivered'] = tonumber(device.rawData[6])

		local formatted = device.counterDeliveredToday or ''
		local info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counterDeliveredToday'] = info['value']

		formatted = device.counterToday or ''
		info = adapterManager.parseFormatted(formatted, domoticz['radixSeparator'])
		device['counterToday'] = info['value']

		function device.updateP1(usage1, usage2, return1, return2, cons, prod)
			--[[
				USAGE1= energy usage meter tariff 1
				USAGE2= energy usage meter tariff 2
				RETURN1= energy return meter tariff 1
				RETURN2= energy return meter tariff 2
				CONS= actual usage power (Watt)
				PROD= actual return power (Watt)
				USAGE and RETURN are counters (they should only count up).
				For USAGE and RETURN supply the data in total Wh with no decimal point.
				(So if your meter displays f.i. USAGE1= 523,66 KWh you need to send 523660)
			 ]]
			local value = tostring(usage1) .. ';' ..
					tostring(usage2) .. ';' ..
					tostring(return1) .. ';' ..
					tostring(return2) .. ';' ..
					tostring(cons) .. ';' ..
					tostring(prod)
			device.update(0, value)
		end
	end

}