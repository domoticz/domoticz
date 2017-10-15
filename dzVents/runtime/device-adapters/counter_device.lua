return {

	baseType = 'device',

	name = 'Counter device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'RFXMeter counter' or device.deviceSubType == 'Counter Incremental')

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateCounter')
		end

		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data: counter, counterToday, valueQuantity, valueUnits

		local valueFormatted = device.counterToday or ''
		local info = adapterManager.parseFormatted(valueFormatted, domoticz['radixSeparator'])
		device['counterToday'] = info['value']

		valueFormatted = device.counter or ''
		info = adapterManager.parseFormatted(valueFormatted, domoticz['radixSeparator'])
		device['counter'] = info['value']


		function device.updateCounter(value)
			return device.update(0, value)
		end

	end

}