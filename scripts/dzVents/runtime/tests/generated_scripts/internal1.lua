return {
	active = true,
	on = {
		devices = {'mySwitch'}
	},
	data = {
		x = { initial = 4 }
	},
	execute = function(domoticz, device, triggerInfo)

		if (device.toggleSwitch) then device.toggleSwitch() end

		domoticz.data.x = domoticz.data.x + 10

		return 'script1: ' ..
				tostring(domoticz.name) ..
				' ' ..
				tostring(device.name) ..
				' ' ..
				tostring(triggerInfo['type'])
	end
}