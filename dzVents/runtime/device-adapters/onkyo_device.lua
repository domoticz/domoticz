local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Onkyo device',

	matches = function (device, adapterManager)
		local res = (device.hardwareType == 'Onkyo AV Receiver (LAN)')
		if (not res) then
			adapterManager.addDummyMethod(device, 'onkyoEISCPCommand')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.onkyoEISCPCommand(cmd)
			return TimedCommand(domoticz, 'CustomCommand:' .. device.id, tostring(cmd), 'device')
		end
	end
}
