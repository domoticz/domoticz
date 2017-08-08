return {
    active = true,
    on = {
        timer = {'every minute'}
    },
    execute = function(domoticz)
        -- Define your devices
        local sunscreen = domoticz.devices('Zonwering')
        local temperature = domoticz.devices('Wunderground - Temperatuur')
        local wind = domoticz.devices('Wunderground - Wind')
        local rain = domoticz.devices('Neerslag')
        local uv = domoticz.devices('Wunderground - UV')
        
        -- Tresholds
        local minTemperature = 15
        local maxWindgust = 150
        local maxWindspeed = 50
        local minUv = 3
        
        -- Sunscreen must be closed at nighttime
        if (domoticz.time.isNightTime) then
            if (sunscreen.state == 'Open') then
                domoticz.log('Sunscreen#Sunscreen up --> It is night')
                sunscreen.switchOff()
            end
            return
        end
        
        -- Check rain sensor
        if (rain.rain > 0) then
            if (sunscreen.state == 'Open') then
                domoticz.log('Sunscreen#Sunscreen up --> It is raining')
                sunscreen.switchOff()
            end
            return
        end
        
        -- Check UV sensor
        if (uv.uv <= minUv) then
            if (sunscreen.state == 'Open') then
                domoticz.log('Sunscreen#Sunscreen up --> Sunshine not too bright')
                sunscreen.switchOff()
            end
            return
        end
        
        -- Check temerature sensor
        if (temperature.temperature <= minTemperature) then
            if (sunscreen.state == 'Open') then
                domoticz.log('Sunscreen#Sunscreen up --> It is too cold')
                sunscreen.switchOff()
            end
            return
        end
        
        -- Check wind sensor
        if (wind.speed >= maxWindspeed or wind.gust >= maxWindgust) then
            if (sunscreen.state == 'Open') then
                domoticz.log('Sunscreen#Sunscreen up --> Windspeed too high')
                sunscreen.switchOff()
            end
            return
        end
        
        -- All thresholds OK, lower sunscreen
        if (sunscreen.state == 'Closed') then
            domoticz.log('Sunscreen#Sun is shining, all thresholds OK, lowering sunscreen')
            sunscreen.switchOn()
        else
            domoticz.log('Sunscreen#Sunscreen OK --> No action')
        end
    end
}
