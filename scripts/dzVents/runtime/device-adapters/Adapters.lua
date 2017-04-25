local genericAdapter = require('generic_device')

local deviceAdapters = {
	'lux_device',
	'zone_heating_device',
	'kwh_device'
}
local fallBackDeviceAdapter = genericAdapter

local _utils = require('Utils')

local function DeviceAdapters(utils)

	if (utils == nil) then
		utils = _utils
	end

	local self = {

		name = 'Generic device adapter',

		getDeviceAdapter = function(device)
			-- find a matching adapter
			for i, adapterName in pairs(deviceAdapters) do

				-- do a safe call and catch possible errors
				ok, adapter = pcall(require, adapterName)
				if (not ok) then
					utils.log(adapter, utils.LOG_ERROR)
				else
					local matches = adapter.matches(device)
					if (matches) then
						return adapter
					end

				end
			end

			-- no adapter found
			-- return the fallback
			return genericAdapter
		end,

		deviceAdapters = deviceAdapters,

		genericAdapter = genericAdapter

	}

	return self

end

return DeviceAdapters
