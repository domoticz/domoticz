return {

	baseType = 'hardware',

	name = 'Hardware device adapter',

	matches = function (device, adapterManager)
		local res = (device.baseType == 'hardware')
		return res
	end,

	process = function (hardware, data, domoticz, utils, adapterManager)

		hardware.isHardware = true
		hardware.isPythonPlugin = hardware._data.isPythonPlugin
		hardware.typeValue = hardware._data.typeValue
		hardware.type = hardware._data.typeName

		if hardware.isPythonPlugin then
			hardware.type = domoticz.hardwareInfo(hardware.id).type
		end

		function hardware.devices()
			local subData = {}
			local ids = domoticz.hardwareInfo(hardware.id).deviceIds

			for i, id in ipairs(ids or {}) do
				subData[i] = domoticz._getItemFromData('device', id)
			end

			return domoticz._setIterators({}, true, 'device', false , subData)
		end

	end

}
