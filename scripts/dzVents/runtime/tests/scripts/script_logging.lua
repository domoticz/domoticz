return {
	active = true,
	logging = {
		level = domoticz.LOG_DEBUG,
		marker = "Hey you"
	},
	on = {
		devices = {'loggingstuff'}
	},
	execute = function(domoticz, device, triggerInfo)

		return tostring(_G.logLevel) .. ' ' .. tostring(_G.logMarker)
	end
}