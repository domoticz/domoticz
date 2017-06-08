return {
	active = true,
	on = {
		'wild*'
	},
	execute = function(domoticz, device)
		return 'script_wildcard1: ' .. domoticz.name .. ' ' .. device.name
	end
}