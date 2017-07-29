return {
	active = false,
	on = {
		security = {
			domoticz.SECURITY_ARMEDAWAY,
		}
	},
	data = {},
	execute = function(domoticz, dummy, info)
		domoticz.log('Security was triggered by ' .. info.trigger, domoticz.LOG_INFO)

		-- see dzVents documentation (security device) for instructions on how to create a
		-- security device.
		domoticz.log(domoticz.devices('My Security Panel').state)

	end
}