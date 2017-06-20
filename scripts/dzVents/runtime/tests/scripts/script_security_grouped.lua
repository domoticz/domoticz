return {
	active = true,
	on = {
		['security'] = {
			domoticz.SECURITY_ARMEDHOME,
			domoticz.SECURITY_DISARMED,
			domoticz.SECURITY_ARMEDAWAY,
		}
	},
	execute = function(domoticz, security, info)
		return 'script_security: ' .. tostring(security) .. ' ' .. tostring(info['trigger'])
	end
}