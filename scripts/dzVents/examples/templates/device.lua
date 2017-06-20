return {
	active = false,
	on = {
		devices = {
			'myDevice'
		}
	},
	data = {},
	execute = function(domoticz, device)
		domoticz.log('Device ' .. device.name .. ' was changed', domoticz.LOG_INFO)
		-- code
	end
}