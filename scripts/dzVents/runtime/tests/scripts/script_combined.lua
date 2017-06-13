return {
	active = true,
	on = {
		'onscript1',
		'onscript2'
	},
	execute = function(domoticz, device)
		device.updateLux(123)
		return 'script_combined'
	end
}