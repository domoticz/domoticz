local Time = require('Time')
local TimedCommand = require('TimedCommand')

local function Variable(domoticz, data)

	local self = {
		['value'] = data.data.value,
		['name'] = data.name,
		['type'] = data.variableType,
		['changed'] = data.changed,
		['id'] = data.id,
		['lastUpdate'] = Time(data.lastUpdate),
		['baseType'] = domoticz.BASETYPE_VARIABLE,
		isVariable = true,
		isHTTPResponse = false,
	    isDevice = false,
	    isScene = false,
	    isGroup = false,
	    isTimer = false,
		isSecurity = false
	}

	if (data.variableType == 'float' or data.variableType == 'integer') then
		-- actually this isn't needed as value is already in the
		-- proper type
		-- just for backward compatibility
		self['nValue'] = data.data.value
	end

	if (data.variableType == 'date') then
		local d, mon, y = string.match(data.data.value, "(%d+)%/(%d+)%/(%d+)")
		local date = y .. '-' .. mon .. '-' .. d .. ' 00:00:00'
		self['date'] = Time(date)
	end

	if (data.variableType == 'time') then
		local now = os.date('*t')
		local time = tostring(now.year) ..
				'-' .. tostring(now.month) ..
				'-' .. tostring(now.day) ..
				' ' .. data.data.value ..
				':00'
		self['time'] = Time(time)
	end


	-- send an update to domoticz
	function self.set(value)
		if (value == nil) then value = '' end

		-- return TimedCommand(domoticz, 'Variable:' .. data.name, tostring(value), 'variable')
		return TimedCommand(domoticz, 'Variable', {
			idx = data.id,
			_trigger = true,
			value = tostring(value)
		}, 'variable')
	end

	function self.cancelQueuedCommands()
		domoticz.sendCommand('Cancel', {
			type = 'variable',
			idx = data.id
		})
	end

	return self
end

return Variable
