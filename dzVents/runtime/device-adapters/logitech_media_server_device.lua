local TimedCommand = require('TimedCommand')

return {
	baseType = 'device',
	name = 'Logitech Media Server device',

	matches = function (device, adapterManager)
		local res = (device.hardwareType == 'Logitech Media Server')

		if (not res) then
			adapterManager.addDummyMethod(device, 'switchOff')
			adapterManager.addDummyMethod(device, 'stop')
			adapterManager.addDummyMethod(device, 'play')
			adapterManager.addDummyMethod(device, 'pause')
			adapterManager.addDummyMethod(device, 'setVolume')
			adapterManager.addDummyMethod(device, 'startPlaylist')
			adapterManager.addDummyMethod(device, 'playFavorites')
			adapterManager.addDummyMethod(device, 'volumeUp')
			adapterManager.addDummyMethod(device, 'volumeDown')
		end
		return res
	end,

	process = function (device, data, domoticz, utils, adapterManager)

		device.playlistID = data.data.levelVal

		function device.switchOff()
			return TimedCommand(domoticz, device.name, 'Off', 'device', device.state)
		end

		function device.stop()
			return TimedCommand(domoticz, device.name, 'Stop', 'device', device.state)
		end

		function device.play()
			return TimedCommand(domoticz, device.name, 'Play', 'device', device.state)
		end

		function device.pause()
			return TimedCommand(domoticz, device.name, 'Pause', 'device', device.state)
		end

		function device.setVolume(value)
			value = tonumber(value)

			if (value < 0 or value > 100) then
				utils.log('Volume must be between 0 and 100. Value = ' .. tostring(value), utils.LOG_ERROR)
			else
				return TimedCommand(domoticz, device.name, 'Set Volume ' .. tostring(value), 'device', device.state)
			end
		end

		function device.startPlaylist(name)
			return TimedCommand(domoticz, device.name, 'Play Playlist ' .. tostring(name), 'updatedevice', device.state)
		end

		function device.playFavorites(position)
			position = tonumber(position)

			if (position == nil) then
				position = 0
			end

			return TimedCommand(domoticz, device.name, 'Play Favorites ' .. tostring(position), 'updatedevice', device.state)
		end

		function device.volumeDown()
			local url = domoticz.settings['Domoticz url'] ..  "/json.htm?type=command&param=lmsmediacommand&action=VolumeDown" ..
					   '&idx=' .. device.id
			return domoticz.openURL(url)
		end

		function device.volumeUp()
			local url = domoticz.settings['Domoticz url'] ..  "/json.htm?type=command&param=lmsmediacommand&action=VolumeUp" .. '&idx=' .. device.id
			return domoticz.openURL(url)
		end

	end

}
