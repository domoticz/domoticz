return {

	baseType = 'device',

	name = 'Solar radiation device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == 'Solar Radiation')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateRadiation')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- from data radiation


		function device.updateRadiation(radiation)
			device.update(0, radiation)
		end
	end

}