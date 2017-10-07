return {

	baseType = 'device',

	name = 'Waterflow device',

	matches = function (device, adapterManager)
		local res = (device.deviceSubType == "Waterflow")
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateWaterflow')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.flow = tonumber(device.rawData[1])

		device['updateWaterflow'] = function (flow) -- l/m
			return device.update(0, flow)
		end

	end

}