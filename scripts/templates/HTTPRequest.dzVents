return {
	on = {
		timer = {
			'every 5 minutes' -- just an example to trigger the request
		},
		httpResponses = {
			'trigger' -- must match with the callback passed to the openURL command
		}
	},
	execute = function(domoticz, item)

		if (item.isTimer) then
			domoticz.openURL({
				url = 'http://somedomain/someAPI?param=1',
				method = 'GET',
				callback = 'trigger', -- see httpResponses above.
			})
		end

		if (item.isHTTPResponse) then

			if (item.statusCode == 200) then
				if (item.isJSON) then

					local someValue = item.json.someValue  -- just an example

					-- update some device in Domoticz
					domoticz.devices('myTextDevice').updateText(someValue)
				end
			else
				domoticz.log('There was a problem handling the request', domoticz.LOG_ERROR)
				domoticz.log(item, domoticz.LOG_ERROR)
			end

		end

	end
}
