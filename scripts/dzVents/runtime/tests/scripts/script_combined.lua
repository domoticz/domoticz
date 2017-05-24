return {
	active = true,
	on = {
		'onscript1',
		'onscript2'
	},
	execute = function(domoticz, device)
		domoticz.notify('Yo')
		return 'script_combined'
	end
}