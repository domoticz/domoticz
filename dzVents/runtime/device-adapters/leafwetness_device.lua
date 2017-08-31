return {

	baseType = 'device',

	name = 'Leaf wetness device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Leaf Wetness')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateWetness')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.wetness = data.data._nValue

		function device.updateWetness(wetness)
			return device.update(tostring(wetness), 0)
		end

	end

}