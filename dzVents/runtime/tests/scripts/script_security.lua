return {
	active = true,
	on = {
		['security'] = {domoticz.SECURITY_ARMEDAWAY}
	},
	execute = function(domoticz, security, info)
		domoticz.notify('Me', domoticz.security)
		return 'script_security: ' .. tostring(security.isSecurity) ..' ' .. tostring(info['trigger'])
	end
}
