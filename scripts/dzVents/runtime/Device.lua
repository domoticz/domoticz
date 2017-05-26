local utils = require('Utils')
local Time = require('Time')
local Adapters = require('Adapters')

local function Device(domoticz, data)

	local changedAttributes = {} -- storage for changed attributes

	-- extract dimming levels for dimming devices
	local level
	local self = {}


	function self.update(...)
		-- generic update method for non-switching devices
		-- each part of the update data can be passed as a separate argument e.g.
		-- device.update(12,34,54) will result in a command like
		-- ['UpdateDevice'] = '<id>|12|34|54'
		local command = self.id
		for i, v in ipairs({ ... }) do
			command = command .. '|' .. tostring(v)
		end

		domoticz.sendCommand('UpdateDevice', command)
	end

	-- todo get rid of this (there's an example using it)
	function self.attributeChanged(attribute)
		-- returns true if an attribute is marked as changed
		return (changedAttributes[attribute] == true)
	end

	local state

	self['name'] = data.name
	self['id'] = data.id
	self['_data'] = data
	self['baseType'] = data.baseType

	local adapterManager = Adapters()

	-- process generic first
	adapterManager.genericAdapter.process(self, data, domoticz, utils, adapterManager)

	-- get a specific adapter for the current device
	local adapters = adapterManager.getDeviceAdapters(self)
	for i, adapter in pairs(adapters) do
		utils.log('Processing adapter for ' .. self.name .. ': ' .. adapter.name, utils.LOG_DEBUG)
		adapter.process(self, data, domoticz, utils, adapterManager)
	end


	-- tbd
	if (data.changedAttribute) then
		changedAttributes[data.changedAttribute] = true
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end


	return self
end

return Device