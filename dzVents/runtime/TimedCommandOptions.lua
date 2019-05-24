local TIMED_OPTIONS = {
	device = {
		_for = true,
		_after = true,
		_within = true,
		_silent = true,
		_triggerMode = 'NOTRIGGER',
		_repeat = true,
		_checkState = true
	},
	variable = {
		_for = false,
		_after = true,
		_within = true,
		_silent = true,
		_triggerMode = 'TRIGGER',
		_repeat = false,
		_checkState = false
	},
	updatedevice = {
		_for = false,
		_after = true,
		_within = true,
		_silent = true,
		_triggerMode = 'TRIGGER',
		_repeat = false,
		_checkState = false
	},
	setpoint =  {
		_silent = true,
		_after = true,
		_within = true,
		_triggerMode = 'NOTRIGGER'
	},
	camera =  {
		_silent = false,
		_after = true,
		_within = true,
		_triggerMode = ''
	},
	triggerIFTTT =  {
		_silent = false,
		_after = true,
		_within = false,
		_triggerMode = ''
	},
}

return TIMED_OPTIONS
