local socket = require("socket");

return {
	active = true,
	on = {
		devices = {
			'vdTrigger',
			'vdContact',
			'vdDoorLockInverted'
		},
	},
	data = {
		varContact = {initial = false},
		varLock = {initial = false}
	},
	execute = function(dz, item)

		-- vdTrigger > vdContact > vdDoorLockInverted > vdTrigger

		if (item.name == 'vdContact') then
			dz.data.varContact = true
			dz.devices('vdDoorLockInverted').switchOn()
		end

		if (item.name == 'vdDoorLockInverted') then
			dz.devices('vdTrigger').switchOff()
			dz.data.varLock = true
		end

		if (item.name == 'vdTrigger') then
			if (item.active) then
				dz.devices('vdContact').switchOn()
			else
				if (dz.data.varContact and dz.data.varLock) then
					item.switchOff().silent() -- means success
				else				
					item.switchOn().silent() -- fail, at the end this should be On
				end
			end
		end


	end
}
