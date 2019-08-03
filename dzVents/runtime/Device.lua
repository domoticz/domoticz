local utils = require('Utils')
local Adapters = require('Adapters')
local TimedCommand = require('TimedCommand')
local TIMED_OPTIONS = require('TimedCommandOptions')

local function Device(domoticz, data, dummyLogger)

	local self = {}
	local state
	local adapterManager = Adapters(dummyLogger)


	function self.update(nValue, sValue, protected)
		local params = {
			idx = self.id,
			nValue = (nValue ~= nil and nValue ~= '')  and nValue or nil,
			sValue = (sValue ~= nil and sValue ~= '')  and tostring(sValue) or nil,
			_trigger = true,
			protected = protected ~= nil and protected or nil
		}
		return TimedCommand(domoticz, 'UpdateDevice', params, 'updatedevice')
	end

	function self.dump()
		domoticz.logDevice(self)
	end

	self['name'] = data.name
	self['id'] = data.id -- actually, this is the idx
	self['idx'] = data.id -- for completeness
	self['_data'] = data
	self['baseType'] = data.baseType

	if (_G.TESTMODE) then
		function self.getAdapterManager()
			return adapterManager
		end
	end

	-- process generic first
	adapterManager.genericAdapter.process(self, data, domoticz, utils, adapterManager)

	-- get a specific adapter for the current device
	local adapters = adapterManager.getDeviceAdapters(self)

	self._adapters = {}

	for i, adapter in pairs(adapters) do
		utils.log('Processing device-adapter for ' .. self.name .. ': ' .. adapter.name, utils.LOG_DEBUG)
		adapter.process(self, data, domoticz, utils, adapterManager)
		table.insert(self._adapters, adapter.name)
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

	return self
end

return Device
