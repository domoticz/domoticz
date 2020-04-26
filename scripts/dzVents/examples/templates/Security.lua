return {
	on = {
		security = {
			domoticz.SECURITY_ARMEDAWAY,
		}
	},
	execute = function(domoticz, security)
		domoticz.log('Security was triggered by ' .. security.trigger, domoticz.LOG_INFO)
	end
}
