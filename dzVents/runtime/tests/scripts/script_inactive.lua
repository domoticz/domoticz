return {
	active = false,
	on = {
		devices = {'onscript1'}
	},
	execute = function(domoticz, device)
		return 'script_inactive'
	end
}