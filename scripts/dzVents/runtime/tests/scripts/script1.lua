return {
	active = true,
	on = {
		'onscript1'
	},
	execute = function(domoticz, device, triggerInfo)

		if (device.toggleSwitch) then device.toggleSwitch() end

		return 'script1: ' ..
				tostring(domoticz.name) ..
				' ' ..
				tostring(device.name) ..
				' ' ..
				tostring(triggerInfo['type'])
	end
}