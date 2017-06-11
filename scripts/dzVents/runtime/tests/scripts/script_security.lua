return {
	active = true,
	on = {
		['security'] = {domoticz.SECURITY_ARMEDAWAY}
	},
	execute = function(domoticz, dummy, info)
		return 'script_security: ' .. tostring(dummy) ..' ' .. tostring(info['trigger'])
	end
}