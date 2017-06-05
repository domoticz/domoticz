return {
	active = true,
	on = {
		'onscript1'
	},
	execute = function(domoticz, device, triggerInfo)

		return 'internal onscript1: ' ..
				tostring(domoticz.name) ..
				' ' ..
				tostring(device.name) ..
				' ' ..
				tostring(triggerInfo['type'])
	end
}