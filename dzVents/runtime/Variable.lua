local Time = require('Time')
local TimedCommand = require('TimedCommand')
local evenItemIdentifier = require('eventItemIdentifier')

local function Variable(domoticz, data)
	local self = {
		['value'] = data.data.value,
		['name'] = data.name,
		['type'] = data.variableType,
		['changed'] = data.changed,
		['id'] = data.id,
		['idx'] = data.id,
		['lastUpdate'] = Time(data.lastUpdate),
	}

	evenItemIdentifier.setType(self, 'isVariable', domoticz.BASETYPE_VARIABLE, data.name)

	if (data.variableType == 'float' or data.variableType == 'integer') then
		-- actually this isn't needed as value is already in the
		-- proper type
		-- just for backward compatibility
		self['nValue'] = data.data.value
	end

	function self.dump( filename )
		domoticz.logObject(self, filename, 'variable')
	end

	function self.dumpSelection( selection )
		domoticz.utils.dumpSelection(self, ( selection or 'attributes' ))
	end

	if (data.variableType == 'date') then
		local d, mon, y = string.match(data.data.value, "(%d+)[%p](%d+)[%p](%d+)")
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

		if self.type == 'integer'  then
			value = math.floor(value)
		elseif value == nil then
			value = ''
		end

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

	function self.rename(newName)
		local newValue = domoticz.utils.urlEncode(self.value)

		if self.type == 'integer' then
			newValue = math.floor(self.value)
		end

		local url = domoticz.settings['Domoticz url'] .. '/json.htm?type=command&param=updateuservariable' ..
			'&idx=' .. data.id ..
			'&vname=' .. domoticz.utils.urlEncode(newName) ..
			'&vtype=' ..  self.type ..
			'&vvalue=' .. newValue
		domoticz.openURL(url)
	end

	return self
end

return Variable
