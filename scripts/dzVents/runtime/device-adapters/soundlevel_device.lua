return {

	baseType = 'device',

	name = 'Sound level device',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == "Sound Level")
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSoundLevel')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.level = tonumber(device.state)

		device['updateSoundLevel'] = function (level)
			device.update(0, level)
		end

	end

}