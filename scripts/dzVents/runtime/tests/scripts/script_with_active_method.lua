return {
	active = function(domoticz)
		return (domoticz.devices('device1').name == 'Device 1')
	end,
	on = {
		devices = {'onscript_active'}
	},
	execute = function(domoticz, device)
		return 'script_with_active_method'
	end
}