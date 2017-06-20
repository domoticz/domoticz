local Time = require('Time')

local function Variable(domoticz, name, value)
	local self = {
		['nValue'] = tonumber(value),
		['value'] = value,
		['lastUpdate'] = Time(uservariables_lastupdate[name])
	}

	-- send an update to domoticz
	function self.set(value)
		domoticz.sendCommand('Variable:' .. name, tostring(value))
	end

	return self
end

return Variable