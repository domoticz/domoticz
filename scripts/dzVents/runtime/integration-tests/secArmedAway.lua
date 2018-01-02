return {
	active = true,
	on = {
		security = {
			domoticz.SECURITY_ARMEDAWAY,
		}
	},
	data = {},
	execute = function(dz, dummy, info)

		if (dz.devices('secPanel').state ~= dz.SECURITY_ARMEDAWAY) then
			dz.log('secArmedAway: Test security: FAILED', dz.LOG_ERROR)
			dz.devices('secArmedAwayResults').updateText('FAILED')
		else
			dz.log('secArmedAway: Test security: OK')
			dz.devices('secArmedAwayResults').updateText('SUCCEEDED')
		end

	end
}