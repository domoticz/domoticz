return {

	baseType = 'device',

	name = 'Z-Wave Thermostat mode device adapter',

	matches = function(device, adapterManager)
		local res = (device.deviceSubType == 'Thermostat Mode')
		if (not res) then
			adapterManager.addDummyMethod(device, 'updateMode')
		end
		return res
	end,

	process = function(device, data, domoticz, utils, adapterManager)

		-- modes from data: ["modes"] = "0;Off;1;Heat;2;Heat Econ;";
		-- mode from data


		local _modes = device.modes and string.split(device.modes, ';') or {}
		-- we have to combine tupels into one
		local modesLookup = {}
		local modes = {}

		for i, j in pairs(_modes) do

			if ( (i/2) ~= math.floor(i/2) ) then -- only the odds which are the ids
				local label = _modes[i + 1]
				if (label ~= nil) then
					modesLookup[tonumber(_modes[i])] = label
				end
			else
				table.insert(modes, _modes[i])
			end

		end

		device.modes = modes
		device.modeString = modesLookup[device.mode]

		function device.updateMode(modeString)
			for i, mode in pairs(modesLookup) do
				if (mode == modeString) then
					return device.update(i, i)
				end
			end
		end

	end
}