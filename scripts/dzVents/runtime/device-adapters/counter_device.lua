return {

	baseType = 'device',

	name = 'Counter device adapter',

	matches = function (device)
		return (device.deviceSubType == 'RFXMeter counter' or device.deviceSubType == 'Counter Incremental')
	end,

	process = function (device, data, domoticz, utils, adapterManager)


		local valueFormatted = device.counterToday or ''
		local info = adapterManager.parseFormatted(valueFormatted, domoticz['radixSeparator'])
		device['counterToday'] = info['value']

		valueFormatted = device.counter or ''
		info = adapterManager.parseFormatted(valueFormatted, domoticz['radixSeparator'])
		device['counter'] = info['value']


		function device.updateCounter(value)
			device.update(0, value)
		end

	end

}