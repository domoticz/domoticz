return {
	active = true,
	on = {
		httpResponses = {'trigger1'}
	},
	execute = function(domoticz, response)
		domoticz.openURL('test')
		return response.callback
	end
}
