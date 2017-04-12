--[[
	Log level = 1: Errors
	Log level = 1.5: Errors + info about the execution of individual scripts and a dump of the commands sent back to Domoticz
	Log level = 2: Errors + info
	Log level = 3: Debug info + Errors + Info (can be a lot!!!)
	Log level = 0: As silent as possible

	About the fetch interval: there's no need to have a short interval in normal
	situations. dzVents will only use it for battery level and some other information
	that doesn't change very often.
]]--
return {
	['Domoticz ip'] = '127.0.0.1',
	['Domoticz port'] = '8080',
	['Fetch interval'] = 'every 30 minutes', -- see readme for timer settings
	['Enable http fetch'] = true, -- only works on linux systems
	['Log level'] = 2
}
