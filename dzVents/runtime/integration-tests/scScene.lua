local log
local dz

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('Scene trigger, ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
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

local testLastUpdate = function(trigger)
	-- check if trigger.lastUpdate is older than the current time
	local now = dz.time.secondsSinceMidnight

	local results = (trigger.lastUpdate.secondsSinceMidnight < now)

	if (not results) then
		print('Error: Now: ' .. tostring(now) .. ' lastUpdate: ' .. tostring(trigger.lastUpdate.secondsSinceMidnight) .. ' should be different.')
	end

	expectEql(true, results, trigger.name .. '.lastUpdate should be in the past')

	tstMsg('Test scene lastUpdate', results)
	return results
end


return {
	active = true,
	on = {
		scenes = {
			'scScene'
		}
	},
	execute = function(domoticz, scene)

		local res = true
		dz = domoticz

		log = dz.log

		res = res and testLastUpdate(scene)

		if (not scene.name == 'sceneSwitch1' or not res) then
			dz.log('scScene: Test scene event: FAILED', dz.LOG_ERROR)
			dz.devices('scSceneResults').updateText('FAILED')
		else
			dz.log('scScene: Test scene event: OK')
			dz.devices('scSceneResults').updateText('SUCCEEDED')
		end

	end
}