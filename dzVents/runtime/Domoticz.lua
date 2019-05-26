local scriptPath = globalvariables['script_path']
package.path = package.path .. ';' .. scriptPath .. '?.lua'

local Camera = require('Camera')
local Device = require('Device')
local Variable = require('Variable')
local Time = require('Time')
local TimedCommand = require('TimedCommand')
local utils = require('Utils')
local _ = require('lodash')

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
	nowTime['isCivilDayTime'] = timeofday['Civildaytime']
	nowTime['isCivilNightTime'] = timeofday['Civilnighttime']
	nowTime['isNightTime'] = timeofday['Nighttime']
	nowTime['sunriseInMinutes'] = timeofday['SunriseInMinutes']
	nowTime['sunsetInMinutes'] = timeofday['SunsetInMinutes']
	nowTime['civTwilightStartInMinutes'] = timeofday['CivTwilightStartInMinutes']
	nowTime['civTwilightEndInMinutes'] = timeofday['CivTwilightEndInMinutes']

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
		['EVENT_TYPE_HTTPRESPONSE'] = 'httpResponse',
		['EVOHOME_MODE_AUTO'] = 'Auto',
		['EVOHOME_MODE_TEMPORARY_OVERRIDE'] = 'TemporaryOverride',
		['EVOHOME_MODE_PERMANENT_OVERRIDE'] = 'PermanentOverride',
		['EVOHOME_MODE_FOLLOW_SCHEDULE'] = 'FollowSchedule',
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
		['NSS_PUSHSAFER'] = 'pushsafer',
		['NSS_TELEGRAM'] = 'telegram',
		['BASETYPE_DEVICE'] = 'device',
		['BASETYPE_SCENE'] = 'scene',
		['BASETYPE_GROUP'] = 'group',
		['BASETYPE_VARIABLE'] = 'variable',
		['BASETYPE_SECURITY'] = 'security',
		['BASETYPE_TIMER'] = 'timer',
		['BASETYPE_HTTP_RESPONSE'] = 'httpResponse',


		utils = {
			_ = _,

			toCelsius = function(f, relative)
				return utils.toCelsius(f, relative)
			end,

			urlEncode = function(s, strSub)
				return utils.urlEncode(s, strSub)
			end,

			urlDecode = function(s)
				return utils.urlDecode(s)
			end,

			round = function(x,n)
				return utils.round(x,n)
			end,

			osExecute = function(cmd)
				utils.osExecute(cmd)
			end,

			fileExists = function(path)
				return utils.fileExists(path)
			end,

			fromJSON = function(json, fallback)
				return utils.fromJSON(json, fallback)
			end,

			toJSON = function(luaTable)
				return utils.toJSON(luaTable)
			end,

			rgbToHSB = function(r, g, b)
				return utils.rgbToHSB(r,g,b)
			end,

			hsbToRGB = function(h, s, b)
				return utils.hsbToRGB(h,s,b)
			end,
			
			dumpTable = function(t, level)
				return utils.dumpTable(t, level)
			end,

			stringSplit = function(text, sep)
				return utils.stringSplit(text, sep)
			end,

			inTable = function(t, searchItem)
				return utils.inTable(t, searchItem)
			end,
		}
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
		local data = subject
				.. '#' .. message
				.. '#' .. tostring(priority)
				.. '#' .. tostring(sound)
				.. '#' .. tostring(extra)
				.. '#' .. tostring(_subSystem)
		self.sendCommand('SendNotification', data)
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

	-- have domoticz send snapshot
	function self.snapshot(cameraID, subject)
		if tostring(cameraID):match("%a") then
			cameraID = self.cameras(cameraID).id
		end
		local snapshotCommand = "SendCamera:" .. cameraID
		return TimedCommand(self, snapshotCommand , subject, 'camera') -- works with afterXXX
	end

	-- have domoticz send an sms
	function self.sms(message)
		self.sendCommand('SendSMS', message)
	end

	-- have domoticz open a url
	function self.openURL(options)

		if (type(options) == 'string') then
			options = {
				url = options,
				method = 'GET'
			}
		end

		if (type(options) == 'table') then

			local url = options.url
			local method = string.upper(options.method or 'GET')
			local callback = options.callback
			local postData

			-- process body data
			if (method == 'POST') then
				postData = ''
				if (options.postData ~= nil) then
					if (type(options.postData) == 'table') then
						postData = utils.toJSON(options.postData)

						if (options.headers == nil) then
							options.headers = { ['Content-Type'] = 'application/json' }
						end
					else
						postData = options.postData
					end
				end
			end

			local request = {
				URL = url,
				method = method,
				headers = options.headers,
				postdata = postData,
				_trigger = callback,
			}

			utils.log('OpenURL: url = ' .. _.str(request.URL), utils.LOG_DEBUG)
			utils.log('OpenURL: method = ' .. _.str(request.method), utils.LOG_DEBUG)
			utils.log('OpenURL: post data = ' .. _.str(request.postdata), utils.LOG_DEBUG)
			utils.log('OpenURL: headers = ' .. _.str(request.headers), utils.LOG_DEBUG)
			utils.log('OpenURL: callback = ' .. _.str(request._trigger), utils.LOG_DEBUG)

			return TimedCommand(self, 'OpenURL', request, 'updatedevice')

		else
			utils.log('OpenURL: Invalid arguments, use either a string or a table with options', utils.LOG_ERROR)
		end

	end

	-- have domoticz trigger an IFTTTT maker event
	function self.triggerIFTTT(sID, ...)
		if sID then
			local luaTable = {}
			luaTable.sID = sID
			for i,value in ipairs({...}) do
				luaTable["sValue" .. i] = tostring(value)
			end
			utils.log('IFFTT Maker Event = ' .. sID, utils.LOG_DEBUG)
			if luaTable.sValue1 then
				utils.log('IFFTT extra value(s) = ' .. luaTable.sValue1 .. " " .. (luaTable.sValue2 or "") .. " " .. (luaTable.sValue3 or ""), utils.LOG_DEBUG)
			end
			return TimedCommand(self, 'TriggerIFTTT', luaTable, 'triggerIFTTT') -- works with afterXXX
		else
			utils.log('No maker event sID is provided', utils.LOG_ERROR)
		end
	end

	-- send a scene switch command
	function self.setScene(scene, value)
		utils.log('setScene is deprecated. Please use the scene object directly.', utils.LOG_INFO)
		return TimedCommand(self, 'Scene:' .. scene, value, 'device', scene.state)
	end

	-- send a group switch command
	function self.switchGroup(group, value)
		utils.log('switchGroup is deprecated. Please use the group object directly.', utils.LOG_INFO)
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

	function self.toCelsius(f, relative)
		utils.log('domoticz.toCelsius deprecated. Please use domoticz.utils.toCelsius.', utils.LOG_INFO)
		return self.utils.toCelsius(f, relative)
	end

	function self.urlEncode(s, strSub)
		utils.log('domoticz.urlEncode deprecated. Please use domoticz.utils.urlEncode.', utils.LOG_INFO)
		return self.utils.urlEncode(s, strSub)
	end

	function self.round(x, n)
		utils.log('domoticz.round deprecated. Please use domoticz.utils.round.', utils.LOG_INFO)
		return self.utils.round(x, n)
	 end

	function self.dump()
		self.utils.dumpTable(settings, '> ')
	end

	function self.logDevice(device)
		self.utils.dumpTable(device, '> ')
	end

	function self.logCamera(camera)
		self.utils.dumpTable(camera, '> ')
	end

	self.__cameras = {}
	self.__devices = {}
	self.__scenes = {}
	self.__groups = {}
	self.__variables = {}

	function self._getItemFromData(baseType, id)

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

	function self._getObject(baseType, id, data)
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
		elseif (baseType == 'camera') then
			cache = self.__cameras
			constructor = Camera
		else
			-- ehhhh
		end

		local item = cache[id]

		if (item ~= nil) then
			return item
		end

		if (data == nil) then
			data = self._getItemFromData(baseType, id)
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

	function self._setIterators(collection, initial, baseType, filterForChanged, initalCollection)

		local _collection

		if (initial) then
			if (initalCollection == nil) then
				_collection = _G.domoticzData
			else
				_collection = initalCollection
			end
		else
			_collection = collection
		end

		collection['forEach'] = function(func)
			local res
			for i, item in pairs(_collection) do

				local _item

				if (initial) then
					if (item.baseType == baseType) and (filterForChanged == true and item.changed == true or filterForChanged == false) then
						_item = self._getObject(baseType, item.id, item) -- create the device object or get it from the cache
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
						_item = self._getObject(baseType, item.id, item) -- create the device object or get it from the cache
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
						_item = self._getObject(baseType, item.id, item) -- create the device object or get it from the cache
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
						_item = self._getObject(baseType, item.id, item) -- create the device object or get it from the cache
					end
				else
					_item = item
				end

				if (_item and type(_item) ~= 'function' and ( (initial == true and type(i) == 'number') or (initial == false and type(i) ~= number))) then
					if (type(filter) == 'function') then
						if (filter(_item)) then
							res[i] = _item
						end
					elseif (type(filter) == 'table') then
						-- assume a list of names

						for i, filterItem in pairs(filter) do
							if (_item[ (type(filterItem) == 'number') and 'id' or 'name' ] == filterItem) then
								res[i] = _item
								break
							end
						end
					else
						--unsupported
						utils.log('Please provide either a function or a table with names/ids to the filter.', utils.LOG_ERROR)
					end
				end
			end
			self._setIterators(res, false, baseType)
			return res
		end

		return collection
	end

	function self.devices(id)
		if (id ~= nil) then
			return self._getObject('device', id)
		else
			return self._setIterators({}, true, 'device', false)
		end
	end

	function self.groups(id)
		if (id ~= nil) then
			return self._getObject('group', id)
		else
			return self._setIterators({}, true, 'group', false)
		end
	end

	function self.scenes(id)
		if (id ~= nil) then
			return self._getObject('scene', id)
		else
			return self._setIterators({}, true, 'scene', false)
		end
	end

	function self.variables(id)
		if (id ~= nil) then
			return self._getObject('uservariable', id)
		else
			return self._setIterators({}, true, 'uservariable', false)
		end
	end

	function self.cameras(id)
		if (id ~= nil) then
			return self._getObject('camera', id)
		else
			return self._setIterators({}, true, 'camera', false)
		end
	end

	function self.changedDevices(id)
		if (id ~= nil) then
			return self._getObject('device', id)
		else
			return self._setIterators({}, true, 'device', true)
		end
	end

	function self.changedScenes(id)
		if (id ~= nil) then
			return self._getObject('scene', id)
		else
			return self._setIterators({}, true, 'scene', true)
		end
	end

	function self.changedGroups(id)
		if (id ~= nil) then
			return self._getObject('group', id)
		else
			return self._setIterators({}, true, 'group', true)
		end
	end

	function self.changedVariables(id)
		if (id ~= nil) then
			return self._getObject('uservariable', id)
		else
			return self._setIterators({}, true, 'uservariable', true)
		end
	end

	return self
end

return Domoticz
