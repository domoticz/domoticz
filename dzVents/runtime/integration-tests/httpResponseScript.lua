local log
local dz

local err = function(msg)
	log(msg, dz.LOG_ERROR)
end

local tstMsg = function(msg, res)
	print('HTTP trigger, ' .. msg .. ': ' .. tostring(res and 'OK' or 'FAILED'))
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

return {
	on = {
		devices = { 'vdHTTPSwitch' },
		httpResponses = {'trigger1', 'trigger2','trigger3'}
	},
	execute = function(domoticz, item)

		dz = domoticz
		log = dz.log
		local res = true

		if item.isHTTPResponse then

			if item.callback == 'trigger1' then

				res = res and expectEql(item.json.p, '1', 'p == 1')
				res = res and expectEql(item.statusCode, 200, 'statusCode')

				if (res) then
					domoticz.globalData.httpTrigger = 'OK'
					domoticz.openURL({
						url = 'http://localhost:4000/testpost',
						method = 'POST',
						callback = 'trigger2',
						postData = {
							p = 2
						}
					})
				end
		
			elseif (item.callback == 'trigger2') then
				res = res and expectEql(item.statusCode, 200, 'statusCode')
				res = res and expectEql(item.json.p, 2, 'p == 2')
				if (res) then domoticz.globalData.httpTrigger = domoticz.globalData.httpTrigger .. "OK"  end
				
			elseif (item.callback == 'trigger3') then
				res = res and expectEql(item.statusCode, 6, 'statusCode')
				if (res) then domoticz.globalData.httpTrigger = domoticz.globalData.httpTrigger .. "OK"  end
		
			end
		
		elseif item.isDevice then
			domoticz.openURL({
				url = 'http://localhost:4000/testget?p=1',
				method = 'GET',
				callback = 'trigger1',
			})
			domoticz.openURL({
				url = 'http://noplaceToGo:4000/testget?p=1',
				method = 'GET',
				callback = 'trigger3',
			}).afterSec(7)
			
			
		end
	end
}