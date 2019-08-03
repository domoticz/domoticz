--[[
    This script can be used to get notification message when plant sensor exceed specified tresholds.
    It is specifically build to be used with the Xiaomi Mi Flora (https://www.domoticz.com/wiki/Mi_Flora_Bluetooth_LE), but can also be used with similar sensors

    For the script to work correctly it is recommended your device names have the following convention:

    Mi Flora - #0 Moisture
    Mi Flora - #0 Conductivity
    Mi Flora - #1 Moisture
    etc. etc.

    This is the default device naming when Mi Flora plugin creates devices.

    If you have another naming you need to adjust settings below.
]]--

local configuration = {
    -- Define the different sensorTypes you want to get notified of
    sensorTypes = {
        moisture = {
            name = 'Moisture', -- Specify substring of name to match ie. "Mi Flora - #1 Moisture"
            property = 'percentage' -- property of dzVents device object to use
        },
        fertility = {
            name = 'Conductivity',
            property = 'percentage'
        }
    },

    -- Define the plant names and the tresholds (min, max) per sensor below
    sensor0 = {
        plant = "Calamondin",
        tresholds = {
            moisture = {30, 60},
            fertility = {350, 2000}
        }
    },
    sensor1 = {
        plant = "Red pepper",
        tresholds = {
            moisture = {15, 60},
            fertility = {350, 2000}
        }
    },
    sensor2 = {
        plant = "Strawberries",
        tresholds = {
            moisture = {15, 60},
            fertility = {350, 2000}
        }
    },
}

return {
    active = true,
    on = {
        devices = {
            'Mi Flora*'
        }
    },
    logging = {
        level = domoticz.LOG_DEBUG
    },
    execute = function(domoticz, device)

        local sensorNumber = string.match(device.name, "#(%d+)")
        local configKey = 'sensor' .. sensorNumber

        if (configuration[configKey] == nil) then
            domoticz.log('No configuration defined for sensor #' .. sensorNumber, domoticz.LOG_INFO)
            return
        end

        local sensorConfig = configuration[configKey]
        local tresholds = sensorConfig.tresholds
        local plantName = sensorConfig.plant

        local function checkSensorTresholds(sensorType, notification)
            local sensorTypeConfig = configuration.sensorTypes[sensorType]
            if (tresholds[sensorType] == nil or not string.match(device.name, sensorTypeConfig.name)) then
                domoticz.log(string.format('No tresholds configured for sensor #%d or name does not match' , sensorNumber), domoticz.LOG_DEBUG)
                return
            end

            local value = device[sensorTypeConfig.property]
            if (value < tresholds[sensorType][1] or value > tresholds[sensorType][2]) then
                notification = string.format(notification, plantName)
                notification = notification .. ' | ' .. string.format('%s = %d', sensorType, value)
                domoticz.notify('Plants', notification)
                domoticz.log(string.format('#%d %s exceeded', sensorNumber, sensorType), domoticz.LOG_DEBUG)
            else
                domoticz.log(string.format('#%d %s ok', sensorNumber, sensorType), domoticz.LOG_DEBUG)
            end
        end

        checkSensorTresholds('moisture', '%s needs watering')
        checkSensorTresholds('fertility', '%s needs fertilization')

    end
}
