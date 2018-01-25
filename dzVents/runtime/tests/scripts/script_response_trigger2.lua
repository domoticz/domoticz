return {
	active = true,
	on = {
		httpResponses = {'trigger2'}
	},
	execute = function(domoticz, response)
		return response.callback
	end
}
