return {
	active = true,
	on = {
		devices = {
			'vdTrigger',
			'vdContact',
			'vdDoorContact',
			'vdDoorLock',
			'vdDoorLockInverted'
		},
	},
	data = {
		varContact = {initial = false},
		varDoorContact = {initial = false},
		varDoorLockInverted = {initial = false},
		varDoorLock = {initial = false}
	},
	execute = function(dz, item)

		-- vdTrigger > vdDoorContact > vdDoorLock > vdDoorLockInverted > vdContact > vdTrigger

		if (item.name == 'vdDoorContact') then
			dz.data.varDoorContact = true
			dz.devices('vdDoorLock').switchOn() -- activate > locked
		end

		if (item.name == 'vdDoorLock') then
			dz.data.varDoorLock = true
			dz.devices('vdDoorLockInverted').switchOff()  -- inverted > de-activate > locked
		end

		if (item.name == 'vdDoorLockInverted') then
			dz.data.varDoorLockInverted = true
			dz.devices('vdContact').open()
		end

		if (item.name == 'vdContact') then
			dz.data.varContact = true
			dz.devices('vdTrigger').switchOff()
		end

		if (item.name == 'vdTrigger') then
			if (item.active) then
				dz.devices('vdDoorContact').open()
			else
				if (dz.data.varContact and dz.data.varDoorContact and dz.data.varDoorLockInverted) then
					item.switchOff().silent() -- means success
				else
					item.switchOn().silent() -- fail, at the end this should be On
				end
			end
		end

		dz.utils._.print(dz.data)
	end
}
