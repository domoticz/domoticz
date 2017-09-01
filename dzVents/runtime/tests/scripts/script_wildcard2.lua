return {
	active = true,
	on = {
		devices = {'some*device'}
	},
	execute = function(domoticz, device)
		return 'script_wildcard2: ' .. domoticz.name .. ' ' .. device.name
	end
}