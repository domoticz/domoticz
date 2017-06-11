local scriptPath = globalvariables['script_path']
package.path = package.path .. ';' .. scriptPath .. '?.lua'
local Device = require('Device')
local Variable = require('Variable')
local Time = require('Time')
local TimedCommand = require('TimedCommand')
local utils = require('Utils')

-- simple string splitting method
-- coz crappy LUA doesn't have this natively... *sigh*
function string:split(sep)
	local sep, fields = sep or ":", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields + 1] = c end)
	return fields
end

-- main class
local function Domoticz(settings)

	local sNow, now
	if (_G.TESTMODE and _G.TESTTIME) then
		sNow = 2017 .. '-' .. 6 .. '-' .. 13 .. ' ' .. 12 .. ':' .. 5 .. ':' .. 0
	else
		now = os.date('*t')
		sNow = now.year .. '-' .. now.month .. '-' .. now.day .. ' ' .. now.hour .. ':' .. now.min .. ':' .. now.sec
	end
	local nowTime = Time(sNow)
	nowTime['isDayTime'] = timeofday['Daytime']
	nowTime['isNightTime'] = timeofday['Nighttime']
	nowTime['sunriseInMinutes'] = timeofday['SunriseInMinutes']
	nowTime['sunsetInMinutes'] = timeofday['SunsetInMinutes']

	-- the new instance
	local self = {
		['settings'] = settings,
		['commandArray'] = {},
		['devices'] = {},
		['scenes'] = {},
		['groups'] = {},
		['changedDevices'] = {},
		['changedVariables'] = {},
		['security'] = globalvariables['Security'],
		['radixSeparator'] = globalvariables['radix_separator'],
		['time'] = nowTime,
		['variables'] = {},
		['PRIORITY_LOW'] = -2,
		['PRIORITY_MODERATE'] = -1,
		['PRIORITY_NORMAL'] = 0,
		['PRIORITY_HIGH'] = 1,
		['PRIORITY_EMERGENCY'] = 2,
		['SOUND_DEFAULT'] = 'pushover',
		['SOUND_BIKE'] = 'bike',
		['SOUND_BUGLE'] = 'bugle',
		['SOUND_CASH_REGISTER'] = 'cashregister',
		['SOUND_CLASSICAL'] = 'classical',
		['SOUND_COSMIC'] = 'cosmic',
		['SOUND_FALLING'] = 'falling',
		['SOUND_GAMELAN'] = 'gamelan',
		['SOUND_INCOMING'] = 'incoming',
		['SOUND_INTERMISSION'] = 'intermission',
		['SOUND_MAGIC'] = 'magic',
		['SOUND_MECHANICAL'] = 'mechanical',
		['SOUND_PIANOBAR'] = 'pianobar',
		['SOUND_SIREN'] = 'siren',
		['SOUND_SPACEALARM'] = 'spacealarm',
		['SOUND_TUGBOAT'] = 'tugboat',
		['SOUND_ALIEN'] = 'alien',
		['SOUND_CLIMB'] = 'climb',
		['SOUND_PERSISTENT'] = 'persistent',
		['SOUND_ECHO'] = 'echo',
		['SOUND_UPDOWN'] = 'updown',
		['SOUND_NONE'] = 'none',
		['HUM_NORMAL'] = 0,
		['HUM_COMFORTABLE'] = 1,
		['HUM_DRY'] = 2,
		['HUM_WET'] = 3,
		-- true mapping to numbers is done in the device adapters for
		-- baro and temphumbaro devices
		['BARO_STABLE'] = 'stable',
		['BARO_SUNNY'] = 'sunny',
		['BARO_CLOUDY'] = 'cloudy',
		['BARO_UNSTABLE'] = 'unstable',
		['BARO_THUNDERSTORM'] = 'thunderstorm',
		['BARO_NOINFO'] = 'noinfo',
		['BARO_PARTLYCLOUDY'] = 'partlycloudy',
		['BARO_RAIN'] = 'rain',
		['ALERTLEVEL_GREY'] = 0,
		['ALERTLEVEL_GREEN'] = 1,
		['ALERTLEVEL_YELLOW'] = 2,
		['ALERTLEVEL_ORANGE'] = 3,
		['ALERTLEVEL_RED'] = 4,
		['SECURITY_DISARMED'] = 'Disarmed',
		['SECURITY_ARMEDAWAY'] = 'Armed Away',
		['SECURITY_ARMEDHOME'] = 'Armed Home',
		['LOG_INFO'] = utils.LOG_INFO,
		['LOG_MODULE_EXEC_INFO'] = utils.LOG_MODULE_EXEC_INFO,
		['LOG_DEBUG'] = utils.LOG_DEBUG,
		['LOG_ERROR'] = utils.LOG_ERROR,
		['EVENT_TYPE_TIMER'] = 'timer',
		['EVENT_TYPE_DEVICE'] = 'device',
		['EVENT_TYPE_VARIABLE'] = 'variable',
		['EVENT_TYPE_SECURITY'] = 'security',
		['EVOHOME_MODE_AUTO'] = 'Auto',
		['EVOHOME_MODE_TEMPORARY_OVERRIDE'] = 'TemporaryOverride',
		['EVOHOME_MODE_PERMANENT_OVERRIDE'] = 'PermanentOverride',
		['INTEGER'] = 'integer',
		['FLOAT'] = 'float',
		['STRING'] = 'string',
		['DATE'] = 'date',
		['TIME'] = 'time',
		['NSS_GOOGLE_CLOUD_MESSAGING'] = 'gcm',
		['NSS_HTTP'] = 'http',
		['NSS_KODI'] = 'kodi',
		['NSS_LOGITECH_MEDIASERVER'] = 'lms',
		['NSS_NMA'] = 'nma',
		['NSS_PROWL'] = 'prowl',
		['NSS_PUSHALOT'] = 'pushalot',
		['NSS_PUSHBULLET'] = 'pushbullet',
		['NSS_PUSHOVER'] = 'pushover',
		['NSS_PUSHSAFER'] = 'pushsafer'
	}

	local function setIterators(collection)

		collection['forEach'] = function(func)
			local res
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					res = func(item, i, collection)
					if (res == false) then -- abort
					return
					end
				end
			end
		end

		collection['reduce'] = function(func, accumulator)
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					accumulator = func(accumulator, item, i, collection)
				end
			end
			return accumulator
		end

		collection['filter'] = function(filter)
			local res = {}
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					if (filter(item)) then
						res[i] = item
					end
				end
			end
			setIterators(res)
			return res
		end
	end

	-- add domoticz commands to the commandArray
	function self.sendCommand(command, value)
		table.insert(self.commandArray, { [command] = value })

		-- return a reference to the newly added item
		return self.commandArray[#self.commandArray], command, value
	end

	-- have domoticz send a push notification
	function self.notify(subject, message, priority, sound, extra, subSystems)
		-- set defaults
		if (priority == nil) then priority = self.PRIORITY_NORMAL end
		if (message == nil) then message = '' end
		if (sound == nil) then sound = self.SOUND_DEFAULT end
		if (extra == nil) then extra = '' end

		local _subSystem

		if (subSystems == nil) then
			_subSystem = ''
		else
			-- combine
			if (type(subSystems) == 'table') then
				_subSystem = table.concat(subSystems, ";")
			elseif (type(subSystems) == 'string') then
				_subSystem = subSystems
			else
				_subSystem = ''
			end
		end
		self.sendCommand('SendNotification', subject
				.. '#' .. message
				.. '#' .. tostring(priority)
				.. '#' .. tostring(sound)
				.. '#' .. tostring(extra)
				.. '#' .. tostring(_subSystem))
	end

	-- have domoticz send an email
	function self.email(subject, message, mailTo)
		if (mailTo == nil) then
			utils.log('No mail-to is provided', utils.LOG_ERROR)
		else
			if (subject == nil) then subject = '' end
			if (message == nil) then message = '' end
			self.sendCommand('SendEmail', subject .. '#' .. message .. '#' .. mailTo)
		end
	end

	-- have domoticz send an sms
	function self.sms(message)
		self.sendCommand('SendSMS', message)
	end

	-- have domoticz open a url
	function self.openURL(url)
		self.sendCommand('OpenURL', url)
	end

	-- send a scene switch command
	function self.setScene(scene, value)
		return TimedCommand(self, 'Scene:' .. scene, value)
	end

	-- send a group switch command
	function self.switchGroup(group, value)
		return TimedCommand(self, 'Group:' .. group, value)
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

	function self.log(message, level)
		utils.log(message, level)
	end

	local function dumpTable(t, level)
		for attr, value in pairs(t) do
			if (type(value) ~= 'function') then
				if (type(value) == 'table') then
					print(level .. attr .. ':')
					dumpTable(value, level .. '    ')
				else
					print(level .. attr .. ': ' .. tostring(value))
				end
			else
				print(level .. attr .. '()')
			end
		end
	end

	-- doesn't seem to work well for some weird reasone
	function self.logDevice(device)
		dumpTable(device, '> ')
	end

	local function setIterators2(collection)

		collection['forEach'] = function(func)
			local res
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					res = func(item, i, collection)
					if (res == false) then -- abort
						return
					end
				end
			end
		end

		collection['reduce'] = function(func, accumulator)
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					accumulator = func(accumulator, item, i, collection)
				end
			end
			return accumulator
		end

		collection['filter'] = function(filter)
			local res = {}
			for i, item in pairs(collection) do
				if (type(item) ~= 'function' and type(i) ~= 'number') then
					if (filter(item)) then
						res[i] = item
					end
				end
			end
			setIterators(res)
			return res
		end
	end


	local _devices

	function getItemFromData(baseType, id)

		for index, item in pairs(domoticzData) do
			if (item.baseType == baseType and (item.id == id or item.name == id)) then
				return item
			end
		end

		return nil

	end

	function self._devices(id)

		if (id ~= nil) then

			local device = _devices[id]

			if (device ~=nil) then
				return device
			end

			local data = getDeviceFromData('device', id)
			if (data ~= nil) then
				local newDevice = Device(self, data)
				_devices[data.id] = newDevice
				_devices[data.name] = newDevice

				return newDevice
			end
		else



		end

	end

	local function bootstrap()


		for index, item in pairs(domoticzData) do

			if (item.baseType == 'device') then

				local newDevice = Device(self, item)
				if (item.changed) then
					self.changedDevices[item.name] = newDevice
					self.changedDevices[item.id] = newDevice-- id lookup
				end

				if (self.devices[item.name] == nil) then
					self.devices[item.name] = newDevice
					self.devices[item.id] = newDevice
				else
					utils.log('Device found with a duplicate name. This device will be ignored. Please rename: ' .. item.name, utils.LOG_DEBUG)
				end

			elseif (item.baseType == 'scene') then

				local newScene = Device(self, item)

				if (self.scenes[item.name] == nil) then
					self.scenes[item.name] = newScene
					self.scenes[item.id] = newScene
				else
					utils.log('Scene found with a duplicate name. This scene will be ignored. Please rename: ' .. item.name, utils.LOG_DEBUG)
				end

			elseif (item.baseType == 'group') then
				local newGroup = Device(self, item)

				if (self.groups[item.name] == nil) then
					self.groups[item.name] = newGroup
					self.groups[item.id] = newGroup
				else
					utils.log('Group found with a duplicate name. This group will be ignored. Please rename: ' .. item.name, utils.LOG_DEBUG)
				end

			elseif (item.baseType == 'uservariable') then
				local var = Variable(self, item)
				self.variables[item.name] = var


				if (item.changed) then
					self.changedVariables[item.name] = var
				end
			end

		end

	end

	bootstrap()

	setIterators(self.devices)
	setIterators(self.changedDevices)
	setIterators(self.variables)
	setIterators(self.changedVariables)
	setIterators(self.scenes)
	setIterators(self.groups)


	return self
end

return Domoticz
