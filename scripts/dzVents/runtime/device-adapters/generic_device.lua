local _states = {
	on = { b = true, inv = 'Off' },
	open = { b = true, inv = 'Closed' },
	['group on'] = { b = true },
	panic = { b = true, inv = 'Off' },
	normal = { b = true, inv = 'Alarm' },
	alarm = { b = true, inv = 'Normal' },
	chime = { b = true },
	video = { b = true },
	audio = { b = true },
	photo = { b = true },
	playing = { b = true, inv = 'Pause' },
	motion = { b = true },
	off = { b = false, inv = 'On' },
	closed = { b = false, inv = 'Open' },
	['group off'] = { b = false },
	['panic end'] = { b = false },
	['no motion'] = { b = false, inv = 'Off' },
	stop = { b = false, inv = 'Open' },
	stopped = { b = false },
	paused = { b = false, inv = 'Play' },
	['all on'] = { b = true, inv = 'All Off' },
	['all off'] = { b = false, inv = 'All On' },
}

-- some states will be 'booleanized'
local function stateToBool(state)
	state = string.lower(state)
	local info = _states[state]
	local b
	if (info) then
		b = _states[state]['b']
	end

	if (b == nil) then b = false end
	return b
end

local function setStateAttribute(state, device)
	local level;
	if (state and string.find(state, 'Set Level')) then
		level = string.match(state, "%d+") -- extract dimming value
		state = 'On' -- consider the device to be on
	end

	if (level) then
		device.addAttribute('level', tonumber(level))
	end


	if (state ~= nil) then -- not all devices have a state like sensors
		if (type(state) == 'string') then -- just to be sure
			device.addAttribute('state', state)
			device.addAttribute('bState', stateToBool(state))
		else
			device.addAttribute('state', state)
		end
	end

	return device
end

return {

	baseType = 'device',

	match = function (device)
		return true -- generic always matches
	end,

	process = function (device)

		local domoticzData = device._data

		local data = domoticzData.data

		setStateAttribute(data._state, device)

		for attribute, value in pairs(data) do
			device.addAttribute(attribute, value)
		end

		return device

	end

}