return {
	active = true,
	on = {
		['variables'] = {'myVar1'}
	},
	execute = function(domoticz, variable, info)

		variable.set(10)
		return 'script_variable1: ' .. tostring(domoticz.name) .. ' ' .. tostring(variable.name) .. ' ' .. tostring(info['type'])

	end
}