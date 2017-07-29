return {
	active = true,
	on = {
		variables = {
			'varString'
		}
	},
	execute = function(dz, variable)

		if (not variable.value == 'Zork is a dork') then
			dz.log('varString: Test variable: FAILED', dz.LOG_ERROR)
			dz.devices('varStringResults').updateText('FAILED')
		else
			dz.log('varString: Test variable: OK')
			dz.devices('varStringResults').updateText('SUCCEEDED')
		end

	end
}