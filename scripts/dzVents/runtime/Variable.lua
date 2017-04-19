local Time = require('Time')

--local function Variable(domoticz, name, value)

local function Variable(domoticz, data)

	local self = {
		['nValue'] = tonumber(data.data.value),
		['value'] = data.data.value,
		['lastUpdate'] = Time(data.lastUpdate)
	}

	-- send an update to domoticz
	function self.set(value)
		domoticz.sendCommand('Variable:' .. data.name, tostring(value))
	end

	return self
end

return Variable