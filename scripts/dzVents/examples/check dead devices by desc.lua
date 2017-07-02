--Check dead device using their description
--This allow to configure device to be checked directly in domoticz GUI by accessing to device details and add "CDA:<delayInMinute>"
--You have to active http fetching !
return {
	active = true,
	on = {
        ['timer'] = {
            'every 60 minutes'
        }
	},
	execute = function(domoticz)
        local message=""

        domoticz.devices().forEach(function(device)
            if (device.description ~= nil) then
                _, _, threshold = string.find(device.description,"CDA:(%d+)")
                if (threshold ~= nil) then
                    local name = device.name
                    local minutes = domoticz.devices(name).lastUpdate.minutesAgo
                    if ( minutes > tonumber(threshold)) then
                            message = message .. 'Device ' ..
                            name .. ' seems to be dead. No heartbeat for at least ' ..
                            minutes .. ' minutes.\r'
                    end
                end

            end
        end)

        if (message ~= "") then
                domoticz.notify('Dead devices', message, domoticz.PRIORITY_HIGH)
                domoticz.log('Dead devices found: ' .. message, domoticz.LOG_ERROR)
        end

	end
}
