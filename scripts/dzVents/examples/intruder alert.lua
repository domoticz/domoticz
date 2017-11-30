return {

	active = true,

	on = {
		devices = {
			'PIR_*' -- all my motion detectors' name start with PIR_
		}
	},

	execute = function(domoticz, detector)

		if (detector.state == 'Motion' and domoticz.security ~= domoticz.SECURITY_DISARMED) then
			-- o dear

			domoticz.setScene('intruder alert', 'On')

			-- send notification
			domoticz.notify('Security breach', '',
				domoticz.PRIORITY_EMERGENCY,
				domoticz.SOUND_SIREN)
		end

	end
}