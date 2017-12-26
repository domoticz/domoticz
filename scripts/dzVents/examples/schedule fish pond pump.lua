return {
	active = true,
	on = {
		['timer'] = {
			'at 8:30 on mon,tue,wed,thu,fri',
			'at 17:30 on mon,tue,wed,thu,fri',

			-- weeekends are different
			'at sunset on sat, sun',
			'at sunrise on sat, sun'
		}
	},

	execute = function(domoticz)

		local pump = domoticz.devices('Pond')
		pump.toggleSwitch()

	end
}
