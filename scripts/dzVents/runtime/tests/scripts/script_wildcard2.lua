return {
	active = true,
	on = {
		'some*device'
	},
	execute = function(domoticz, device)
		return 'script_wildcard2: ' .. domoticz.name .. ' ' .. device.name
	end
}