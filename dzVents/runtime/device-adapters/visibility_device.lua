return {

	baseType = 'device',

	name = 'Visibility device',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == "Visibility")
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateVisibility')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.visbility = utils.round(data.data.visibility, 1)

		device['updateVisibility'] = function (visibility)
			return device.update(0, visibility)
		end

	end

}