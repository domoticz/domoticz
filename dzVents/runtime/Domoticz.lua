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

	local sNow, now, nowTime
	if (_G.TESTMODE and _G.TESTTIME) then
		sNow = 2017 .. '-' .. 6 .. '-' .. 13 .. ' ' .. 12 .. ':' .. 5 .. ':' .. 0
		nowTime = Time(sNow)
	else
		nowTime = Time()
	end

	-- check if the user set a lat/lng
	-- if not, then daytime, nighttime is incorrect
	if (_G.timeofday['SunriseInMinutes'] == 0 and _G.timeofday['SunsetInMinutes'] == 0) then
		utils.log('No information about sunrise and sunset available. Please set lat/lng information in settings.', utils.LOG_ERROR)
	end

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
		['startTime'] = Time(globalvariables['domoticz_start_time']),
		['systemUptime'] = tonumber(globalvariables['systemUptime']),
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
		['LOG_FORCE'] = utils.LOG_FORCE,
		['EVENT_TYPE_TIMER'] = 'timer',
		['EVENT_TYPE_DEVICE'] = 'device',
		['EVENT_TYPE_VARIABLE'] = 'variable',
		['EVENT_TYPE_SECURITY'] = 'security',
		['EVENT_TYPE_SCENE'] = 'scene',
		['EVENT_TYPE_GROUP'] = 'group',
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
		utils.log('OpenURL: ' .. tostring(url), utils.LOG_DEBUG)
		self.sendCommand('OpenURL', url)
	end

	-- send a scene switch command
	function self.setScene(scene, value)
		return TimedCommand(self, 'Scene:' .. scene, value, 'device', scene.state)
	end

	-- send a group switch command
	function self.switchGroup(group, value)
		return TimedCommand(self, 'Group:' .. group, value, 'device', group.state)
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

	function self.toCelsius(f, relative)
		if (relative) then
			return f*(1/1.8)
		end
		return ((f-32) / 1.8)
	end

	function self.urlEncode(s, strSub)
		return utils.urlEncode(s, strSub)
	end

	function self.round(x, n)
		n = math.pow(10, n or 0)
		x = x * n
		if x >= 0 then
			x = math.floor(x + 0.5)
		else
			x = math.ceil(x - 0.5)
		end
		return x / n
	end

	-- doesn't seem to work well for some weird reasone
	function self.logDevice(device)
		dumpTable(device, '> ')
	end

	self.__devices = {}
	self.__scenes = {}
	self.__groups = {}
	self.__variables = {}

	function getItemFromData(baseType, id)

		local res

		for index, item in pairs(_G.domoticzData) do
			if (item.baseType == baseType) then
				if (item.id == id or item.name == id) then
					if (res == nil) then
						res = item
					else
						utils.log('Multiple items found for ' .. tostring(id) .. ' (' .. tostring(baseType) .. '). Please make sure your names are unique or use ids instead.', utils.LOG_ERROR)
					end
				end
			end
		end

		return res
	end

	function getObject(baseType, id, data)
		local cache
		local constructor

		if (baseType == 'device') then
			cache = self.__devices
			constructor = Device
		elseif (baseType == 'group') then
			cache = self.__groups
			constructor = Device
		elseif (baseType == 'scene') then
			cache = self.__scenes
			constructor = Device
		elseif (baseType == 'uservariable') then
			cache = self.__variables
			constructor = Variable
		else
			-- ehhhh
		end

		local item = cache[id]

		if (item ~= nil) then
			return item
		end

		if (data == nil) then
			data = getItemFromData(baseType, id)
		end

		if (data ~= nil) then
			local newItem = constructor(self, data)
			cache[data.id] = newItem
			cache[data.name] = newItem

			return newItem
		end

		-- special case for scenes and groups
		-- as they may not be in the collection if Domoticz wasn't restarted after creating the scene or group.
		if (baseType == 'scene' or baseType == 'group') then
			utils.log('There is no group or scene with that name or id: ' ..
				tostring(id) ..
				'. If you just created the scene or group you may have to restart Domoticz to make it become visible to dzVents.', utils.LOG_ERROR)
		else
			utils.log('There is no ' .. baseType .. ' with that name or id: ' .. tostring(id), utils.LOG_ERROR)
		end

	end


	local function setIterators(collection, initial, baseType, filterForChanged)

		local _collection

		if (initial) then
			_collection = _G.domoticzData
		else
			_collection = collection
		end

		collection['forEach'] = function(func)
			local res
			for i, item in pairs(_collection) do

				local _item

				if (initial) then
					if (item.baseType == baseType) and (filterForChanged == true and item.changed == true or filterForChanged == false) then
						_item = getObject(baseType, item.id, item) -- create the device object or get it from the cache
					end
				else
					_item = item
				end


				if (_item and type(_item) ~= 'function' and ((initial == true and type(i) == 'number') or (initial == false and type(i) ~= number))) then
					res = func(_item)
					if (res == false) then -- abort
						return
					end
				end
			end
		end

		collection['find'] = function(func)
			local res
			local ret
			for i, item in pairs(_collection) do

				local _item

				if (initial) then
					if (item.baseType == baseType) and (filterForChanged == true and item.changed == true or filterForChanged == false) then
						_item = getObject(baseType, item.id, item) -- create the device object or get it from the cache
					end
				else
					_item = item
				end


				if (_item and type(_item) ~= 'function' and ((initial == true and type(i) == 'number') or (initial == false and type(i) ~= number))) then
					ret = func(_item)
					if (ret == true) then
						return _item
					end
				end
			end
		end

		collection['reduce'] = function(func, accumulator)
			for i, item in pairs(_collection) do

				local _item

				if (initial) then
					if (item.baseType == baseType) and (filterForChanged == true and item.changed == true or filterForChanged == false) then
						_item = getObject(baseType, item.id, item) -- create the device object or get it from the cache
					end
				else
					_item = item
				end

				if (_item and type(_item) ~= 'function' and ((initial == true and type(i) == 'number') or (initial == false and type(i) ~= number))) then
					accumulator = func(accumulator, _item)
				end
			end
			return accumulator
		end

		collection['filter'] = function(filter)
			local res = {}
			for i, item in pairs(_collection) do

				local _item

				if (initial) then
					if (item.baseType == baseType) and (filterForChanged == true and item.changed == true or filterForChanged == false) then
						_item = getObject(baseType, item.id, item) -- create the device object or get it from the cache
					end
				else
					_item = item
				end

				if (_item and type(_item) ~= 'function' and ( (initial == true and type(i) == 'number') or (initial == false and type(i) ~= number))) then
					if (filter(_item)) then
						res[i] = _item
					end
				end
			end
			setIterators(res, false, baseType)
			return res
		end

		return collection
	end


	function self.devices(id)
		if (id ~= nil) then
			return getObject('device', id)
		else
			return setIterators({}, true, 'device', false)
		end
	end

	function self.groups(id)
		if (id ~= nil) then
			return getObject('group', id)
		else
			return setIterators({}, true, 'group', false)
		end
	end

	function self.scenes(id)
		if (id ~= nil) then
			return getObject('scene', id)
		else
			return setIterators({}, true, 'scene', false)
		end
	end

	function self.variables(id)
		if (id ~= nil) then
			return getObject('uservariable', id)
		else
			return setIterators({}, true, 'uservariable', false)
		end
	end

	function self.changedDevices(id)
		if (id ~= nil) then
			return getObject('device', id)
		else
			return setIterators({}, true, 'device', true)
		end
	end

	function self.changedScenes(id)
		if (id ~= nil) then
			return getObject('scene', id)
		else
			return setIterators({}, true, 'scene', true)
		end
	end

	function self.changedGroups(id)
		if (id ~= nil) then
			return getObject('group', id)
		else
			return setIterators({}, true, 'group', true)
		end
	end

	function self.changedVariables(id)
		if (id ~= nil) then
			return getObject('uservariable', id)
		else
			return setIterators({}, true, 'uservariable', true)
		end
	end

	return self
end

return Domoticz
