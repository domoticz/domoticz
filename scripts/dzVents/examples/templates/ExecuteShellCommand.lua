return {
	on = {
		timer = {
			'every 5 minutes' -- just an example to trigger the request
		},
		shellCommandResponses = {
			'trigger' -- must match with the callback passed to the executeShellCommand
		}
	},
	execute = function(domoticz, item)

		if (item.isTimer) then
			domoticz.executeShellCommand({
				command = 'ls',
				callback = 'trigger', -- see shellCommandResponses above.
				timeout = 20, -- max execution time in seconds. Defaults to 10, specify 0 for no limit on execution time
			})
		end

		if (item.isShellCommandResponse) then

			if (item.statusCode==0) then
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

