local TimedCommand = require('TimedCommand')

return {

    baseType = 'device',

    name = 'Onkyo device',

    matches = function (device, adapterManager)
        local res = (device.hardwareType == 'Onkyo AV Receiver (LAN)')
        if (not res) then
            adapterManager.addDummyMethod(device, 'onkyoEISCPCommand')
        end
        return res
    end,

    process = function (device, data, domoticz, utils, adapterManager)

        function device.onkyoEISCPCommand(cmd)
            local url = domoticz.settings['Domoticz url'] ..
                    '/json.htm?param=onkyoeiscpcommand&type=command&idx=' .. device.id .. '&action=' .. tostring (cmd)
            return domoticz.openURL(url)
        end
    end
}
