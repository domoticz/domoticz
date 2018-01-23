-- this script can be used in conjunction with the System-alive checker plug-in.
-- the plugin pings a list of devices and creates switches for these devices
-- the reason for this script is to not treat devices as dead immediately after they
-- do not respond. More often than not, the second ping atempt does work. So only
-- if the device has been not responding for a while AFTER it is been presumed dead
-- then this script will notify you

-- put the names of these switches in the devicesToCheck list
-- you may have to tweak the THRESHOLD depending on the check interval




local THRESHOLD = 5 -- minutes
local devicesToCheck = {'ESP8266 CV', 'ESP8266 ManCave', 'ESP8266 Watermeter'}

return {
	active = true,
	on = {
		devices = {
			devicesToCheck
		},
		timer = {
			'every 5 minutes'
		}
	},
	data = {
		notified = { initial = {} }
	},
	execute = function(domoticz, item, triggerInfo)

		if (item.isTimer) then

			-- check all devices that are off and have not been updated in the past 5 minutes and have not been notified for
			for index, deviceName in pairs(devicesToCheck) do

				local device = domoticz.devices(deviceName)

				if (device.state == 'Off' and
						device.lastUpdate.minutesAgo >= THRESHOLD and
						domoticz.data.notified[deviceName] == false) then

					domoticz.notify(deviceName .. ' is not responding anymore.',' ',domoticz.PRIORITY_HIGH)

					-- make sure we only notify once for this device in this case
					domoticz.data.notified[deviceName] = true
				end
			end


		else
			-- it is the device that was triggered
			domoticz.data.notified[item.name] = false

		end

	end
}
