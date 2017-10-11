return {

	baseType = 'device',

	name = 'Text device',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Text')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateText')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device['text'] = device.rawData[1] or ''

		device['updateText'] = function (text)

			return device.update(0, tostring(text))

		end

	end

}