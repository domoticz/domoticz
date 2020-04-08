return {
	on = {
		variables = {
			'myUserVariable'
		}
	},
	execute = function(domoticz, variable)
		domoticz.log('Variable ' .. variable.name .. ' was changed', domoticz.LOG_INFO)
		-- code
	end
}