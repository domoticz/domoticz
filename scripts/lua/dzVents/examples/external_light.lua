-- External light
--
-- start on event only on night 
-- stop in 5 to 10 minutes after last event

return {
    active = true,
    on = {
        -- switches name's witch turn light on 
        'PIR Indoor',
        'Kitchen Door Switch',
        'Door Entry Switch',
        'Garage Switch',
        'Gate Switch',
        -- periodical check if we must switch light off
        timer = 'every 5 minutes'
    },
    execute = function(domoticz, switch, triggerInfo)
    	-- external light switch name
    	local external_light = domoticz.devices['Eclairage Extérieur']
    	
    	-- timed event : to switch off light
        if (triggerInfo.type == domoticz.EVENT_TYPE_TIMER) then
			if (external_light.lastUpdate.minutesAgo > 5 ) then
				external_light.switchOff()
			end
    	else
    	-- all other events : turn light on, but only on night !
           	if (domoticz.time.isNightTime) then
	           	external_light.switchOn()
			end
        end   
    end
}
