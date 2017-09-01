return {

	baseType = 'device',

	name = '1-phase Ampere device adapter',

	matches = function (device, adapterManager)
		local res = (device.deviceType == 'General' and device.deviceSubType == 'Current')
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		-- current from data.data

		function device.updateCurrent(current)
			return device.update(0, current)
		end

	end

}