return 
{
	on = 
	{
		devices = 
		{
			'vdHTTPSwitch' 
		},
	
		httpResponses = 
		{
			'*HTTP*',
		}
	},

	logging = 
	{
		level = domoticz.LOG_DEBUG,
		marker = 'integrationTest httpResponse',
	},

	execute = function(dz, item)

		local expectEql = function(attr, test, marker)
			if attr == test then 
				return true
			else
				dz.log(tostring(attr) .. '~=' .. tostring(test), dz.LOG_ERROR)
				return false
			end
		end

		local function sendURL(method, url, trigger)
			dz.openURL(
			{
				url = ( url or 'http://localhost:4000/test' ) .. method:lower() .. '?p=1',
				method = method,
				callback = 'HTTPTrigger' .. ( trigger or method ) ,
				postData = { p = method },
			})
		end

		if item.isHTTPResponse then

			if item.callback == 'HTTPTriggerGET' and expectEql(item.json.p, '1', 'p == 1') and expectEql(item.statusCode, 200, 'statusCode') then 
					dz.globalData.httpTrigger[item.callback] = "OK"  
					sendURL( 'POST')
			elseif item.callback == 'HTTPTriggerPOST' and expectEql(item.statusCode, 200, 'statusCode') and expectEql(item.json.p, 'POST', 'p = "POST"') then
					dz.globalData.httpTrigger[item.callback] = "OK" 
					sendURL('PUT')
			elseif item.callback == 'HTTPTriggerPUT' and expectEql(item.statusCode, 200, 'statusCode') and expectEql(item.json.p, 'PUT', 'p = "PUT"') then 
					dz.globalData.httpTrigger[item.callback] = "OK"  
					sendURL('DELETE')
			elseif item.callback == 'HTTPTriggerDELETE' and expectEql(item.statusCode, 200, 'statusCode') and expectEql(item.json.p, 'DELETE', 'p = "DELETE"') then
					dz.globalData.httpTrigger[item.callback] = "OK"  
					sendURL('GET', 'http://noplaceToGo:4000/test', 'FAIL')
			elseif item.callback == 'HTTPTriggerFAIL' and expectEql(item.statusCode, 6, 'statusCode') then 
				dz.globalData.httpTrigger[item.callback] = "OK"  
dz.utils.dumpTable(dz.globalData)
			elseif item.callback == 'vdHTTPSwitch' and expectEql(item.statusCode, 200, 'statusCode') then 
				dz.globalData.httpTrigger[item.callback] = "OK"  
			end
	
		elseif item.isDevice then

			dz.globalData.httpTrigger = {}
			dz.triggerHTTPResponse(item.name, 4, item.state )
			sendURL('GET')

		end

	end
}
