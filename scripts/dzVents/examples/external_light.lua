-- External light
-- turn lights on at sunset and back off at sunrise

return {
    active = true,
    on = {
        timer = {
	        'at sunset',
	        'at sunrise'
        }
    },
    execute = function(domoticz, timer)
    	-- external light switch name
    	local external_light = domoticz.devices('External light')

        if (timer.trigger == 'at sunset') then
	        external_light.switchOn()
        else
	        external_light.switchOff()
        end
    end
}
