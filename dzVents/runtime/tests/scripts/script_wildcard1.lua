return {
	active = true,
	on = {
		devices = {'wild*'}
	},
	execute = function(domoticz, device)
		return 'script_wildcard1: ' .. domoticz.name .. ' ' .. device.name
	end
}