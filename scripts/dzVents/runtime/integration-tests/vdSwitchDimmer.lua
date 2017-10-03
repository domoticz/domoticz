local log
local dz

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('vdSwitchDimmer tests: ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
end

local expectEql = function(attr, test, marker)
	if (attr ~= test) then
		local msg = tostring(attr) .. '~=' .. tostring(test)
		if (marker ~= nil) then
			msg = msg .. ' (' .. tostring(marker) .. ')'
		end
		err(msg)
		print(debug.traceback())
		return false
	end
	return true
end

local checkAttributes = function(dev, attributes)
	local res = true
	for attr, value in pairs(attributes) do
		res = res and expectEql(dev[attr], value, attr)
	end
	return res
end

local testDimmer = function(name)
	local dev = dz.devices(name)

	local res = true
	res = res and checkAttributes(dev, {
		['level'] = 75,
		["lastLevel"] = 33,
	})

	tstMsg('Test dimmer lastLevel', res)
	return res
end


return {
	active = true,
	on = {
		devices = {'vdSwitchDimmer'},
	},
	execute = function(domoticz, dimmer)
		local res = true
		dz = domoticz
		log = dz.log

		log('vdSwitchDimmer 1 tests')

		res = res and testDimmer('vdSwitchDimmer')

		if (not res) then
			log('Results vdSwitchDimmer: FAILED!!!!', dz.LOG_ERROR)
			dz.devices('switchDimmerResults').updateText('FAILED')
		else
			log('Results vdSwitchDimmer: SUCCEEDED')
			dz.devices('switchDimmerResults').updateText('SUCCEEDED')
		end
	end
}