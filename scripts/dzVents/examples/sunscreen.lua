local sensors = {
    temperature = {
        active = true,
        device = 'Temperature',
        closeRule = function(device)
            return device.temperature <= 15
        end
    },
    wind = {
        active = true,
        device = 'Wind',
        closeRule = function(device)
            return device.speed >= 50 or device.gust >= 150
        end
    },
    rain = {
        active = true,
        device = 'Rain',
        closeRule = function(device)
            return device.rain > 0
        end
    },
    uv = {
        active = true,
        device = 'UV',
        closeRule = function(device)
            return device.uv <= 3
        end
    },
    lux = {
        active = false,
        device = 'Lux',
        closeRule = function(device)
            return device.lux <= 500
        end
    }
}

local sunscreenDevice = 'Sunscreen'
local dryRun = true
-- local manualOverrideSwitch = 'Disable auto sunscreen'

return {
    active = true,
    on = {
        timer = {'every minute'}
    },
    logging = {
        level = domoticz.LOG_DEBUG,
        marker = 'Sunscreen'
    },
    execute = function(domoticz)

        if (manualOverrideSwitch ~= nil and domoticz.devices(manualOverrideSwitch).state == 'On') then
            return
        end

        local sunscreen = domoticz.devices(sunscreenDevice)

        -- Sunscreen must always be up during nighttime
        if (domoticz.time.isNightTime) then
            if (sunscreen.state == 'Open') then
                if (not dryRun) then
                    sunscreen.switchOff()
                end
                domoticz.log('Closing sunscreen, It is night', domoticz.LOG_INFO)
            end
            return
        end

        -- Check all sensor tresholds and if any exeeded close sunscreen
        for sensorType, sensor in pairs(sensors) do

            if (sensor['active'] == true) then

                local device = domoticz.devices(sensor['device'])
                local closeRule = sensor['closeRule']

                domoticz.log('Checking sensor: ' .. sensorType, domoticz.LOG_DEBUG)

                if (closeRule(device) and sunscreen.state == 'Open') then
                    if (not dryRun) then
                        sunscreen.switchOff()
                    end
                    domoticz.log(sensorType .. ' treshold exceeded, closing sunscreen', domoticz.LOG_INFO)
                    -- Return early when we exeed any tresholds
                    return
                end
            else
                domoticz.log('Sensor not active skipping: ' .. sensorType, domoticz.LOG_DEBUG)
            end
        end

        -- All tresholds OK, sunscreen may be lowered
        if (sunscreen.state == 'Closed') then
            domoticz.log('Sun is shining, all thresholds OK, lowering sunscreen')
            if (not dryRun) then
                sunscreen.switchOn()
            end
        end
    end
}
