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
        httpResponses = {'trigger1', 'trigger2'}
    },
    execute = function(domoticz, triggerItem, info)

		dz = domoticz
		log = dz.log
        local res = true

        if (info.type == domoticz.EVENT_TYPE_HTTPRESPONSE) then

            if (info.trigger == 'trigger1') then

                res = res and expectEql(triggerItem.json.p, '1', 'p == 1')

                if (res) then
                    domoticz.globalData.httpTrigger = 'trigger1'
                    domoticz.openURL({
                       url = 'http://localhost:3000/testpost',
                       method = 'POST',
                       callback = 'trigger2',
                       body = {
                           p = 2
                       }
                   })

                end

            elseif (info.trigger == 'trigger2') then
                res = res and expectEql(triggerItem.json.p, '2', 'p == 2')
                domoticz.globalData.httpTrigger = 'trigger2'
            end

        else
            domoticz.openURL({
                url = 'http://localhost:3000/testget?p=1',
                method = 'GET',
                callback = 'trigger1',
            })
        end


    end
}
