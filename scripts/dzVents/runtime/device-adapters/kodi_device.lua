return {

	baseType = 'device',

	name = 'Kodi device',

	matches = function (device, adapterManager)
		local res = (device.hardwareType == 'Kodi Media Server')
		if (not res) then
			adapterManager.addDummyMethod(device, 'kodiSwitchOff')
			adapterManager.addDummyMethod(device, 'kodiStop')
			adapterManager.addDummyMethod(device, 'kodiPlay')
			adapterManager.addDummyMethod(device, 'kodiPause')
			adapterManager.addDummyMethod(device, 'kodiSetVolume')
			adapterManager.addDummyMethod(device, 'kodiPlayPlaylist')
			adapterManager.addDummyMethod(device, 'kodiPlayFavorites')
			adapterManager.addDummyMethod(device, 'kodiExecuteAddOn')

		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		function device.kodiSwitchOff()
			--return TimedCommand(domoticz, device.name, 'Play')
			domoticz.sendCommand(device.name, 'Off')
		end

		function device.kodiStop()
			--return TimedCommand(domoticz, device.name, 'Stop')
			domoticz.sendCommand(device.name, 'Stop')
		end

		function device.kodiPlay()
			--return TimedCommand(domoticz, device.name, 'Play')
			domoticz.sendCommand(device.name, 'Play')
		end

		function device.kodiPause()
			--return TimedCommand(domoticz, device.name, 'Pause')
			domoticz.sendCommand(device.name, 'Pause')
		end

		function device.kodiSetVolume(value)

			if (value < 0 or value > 100) then

				utils.log('Volume must be between 0 and 100. Value = ' .. tostring(value), utils.LOG_ERROR)

			else
				--return TimedCommand(domoticz, device.name, 'Pause')
				domoticz.sendCommand(device.name, 'Set Volume ' .. tostring(value))
			end
		end

		function device.kodiPlayPlaylist(name, position)
			if (position == nil) then
				position = 0
			end
			domoticz.sendCommand(device.name, 'Play Playlist ' .. tostring(name) .. ' ' .. tostring(position))
		end

		function device.kodiPlayFavorites(position)
			if (position == nil) then
				position = 0
			end
			domoticz.sendCommand(device.name, 'Play Favorites ' .. tostring(position))
		end

		function device.kodiExecuteAddOn(addonId)
			domoticz.sendCommand(device.name, 'Execute ' .. tostring(addonId))
		end
	end

}