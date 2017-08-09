-- Define all the sensors which needs to be considered for the sunscreen to close
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
            return device.rainRate > 0
        end
    },
    rainExpected = {
        active = false,
        device = 'Rain expected', -- This needs to be a virtual sensor of type 'percentage'
        closeRule = function(device)
            return device.percentage > 15
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

-- Define the name of your sunscreen device
local sunscreenDevice = 'Sunscreen'

-- Enable dry run mode to test the sunscreen script without actually activating the sunscreen
local dryRun = false

-- Define the name of a virtual switch which you can use to disable the sunscreen automation script
-- Set to false to disable this feature
local manualOverrideSwitch = false

-- Minutes to wait after a sunscreen close before opening it again.
local timeBetweenOpens = 10

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
        
        local function switchOn(sunscreen, message)
            if (sunscreen.state == 'Closed') then
                if (not dryRun) then
                    sunscreen.switchOn()
                    domoticz.notify('Sunscreen', message)
                end
                domoticz.log(message, domoticz.LOG_INFO)
            end
        end
    
        local function switchOff(sunscreen, message)
            if (sunscreen.state == 'Open') then
                if (not dryRun) then
                    sunscreen.switchOff()
                    domoticz.notify('Sunscreen', message)
                end
                domoticz.log(message, domoticz.LOG_INFO)
            end
        end
        
        if (manualOverrideSwitch and domoticz.devices(manualOverrideSwitch).state == 'On') then
            domoticz.log('Automatic sunscreen script is manually disabled', domoticz.LOG_DEBUG)
            return
        end
        
        local sunscreen = domoticz.devices(sunscreenDevice)
        
        -- Sunscreen must always be up during nighttime
        if (domoticz.time.isNightTime) then
            switchOff(sunscreen, 'Closing sunscreen, It is night')
            return
        end
        
        -- Check all sensor tresholds and if any exeeded close sunscreen
        for sensorType, sensor in pairs(sensors) do

            if (sensor['active'] == true) then
                
                local device = domoticz.devices(sensor['device'])
                local closeRule = sensor['closeRule']
                
                domoticz.log('Checking sensor: ' .. sensorType, domoticz.LOG_DEBUG)
                
                if (closeRule(device)) then
                    
                    switchOff(sunscreen, sensorType .. ' treshold exceeded, Sunscreen up')
                    
                    domoticz.log(sensorType .. ' treshold exceeded', domoticz.LOG_DEBUG)
                    -- Return early when we exeed any tresholds
                    return
                end
            else
                domoticz.log('Sensor not active skipping: ' .. sensorType, domoticz.LOG_DEBUG)
            end
        end
        
        -- All tresholds OK, sunscreen may be lowered
        if (sunscreen.lastUpdate.minutesAgo > timeBetweenOpens) then
            switchOn(sunscreen, 'Sun is shining, all thresholds OK, lowering sunscreen')
        end
    end
}
