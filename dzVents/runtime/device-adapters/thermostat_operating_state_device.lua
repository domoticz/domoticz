return {

	baseType = 'device',

	name = 'Thermostat operating state device adapter',

	matches = function(device, adapterManager)
		local res = (device.deviceSubType == 'Thermostat Operating State')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateMode')
		end
		return res
	end,

	process = function(device, data, domoticz, utils, adapterManager)

		local _modes = { 'Idle', 'Cooling', 'Heating' }

		device.modes = table.concat(_modes,', ')
		device.modeString = _modes[ ( device.nValue < 3 and device.nValue + 1 ) or 1 ]
		device.active = device.nValue == 1 or device.nValue == 2
		device.mode = device.nValue

		function device.updateMode(newMode)
			if type(newMode) == 'string' then
				for i, mode in pairs(_modes) do
					if ( mode:lower() == newMode:lower() ) then
						return device.update(i - 1 , ( newMode:lower() ))
					end
				end
				utils.log('Unknown mode "' .. modeString .. '"  ; available modes are: ' .. device.modes, utils.LOG_ERROR)
			elseif newmode == 'number' and newMode >=0 and newMode <=2 then
				return device.update( math.floor(newMode) , _modes[ math.floor(newMode) + 1] )
			end
			utils.log('Unknown mode "' .. newMode .. '"  ; available modes are: 0, 1 and 2', utils.LOG_ERROR)
		end

	end
}