local scriptPath = globalvariables['script_path']
package.path = scriptPath .. '?.lua' .. ';' .. package.path

local Camera = require('Camera')
local Device = require('Device')
local Variable = require('Variable')
local Time = require('Time')
local TimedCommand = require('TimedCommand')
local utils = require('Utils')
local _ = require('lodash')
local constants = require('constants');

local function merge(toTable, fromTable)
	for k, v in pairs(fromTable) do
		toTable[k] = v
	end
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
	if not(globalvariables.locationSet) then
		utils.log('No information about longitude / latitude available. Please set lat/lng information in settings.', utils.LOG_ERROR)
	end

	nowTime['isDayTime'] = timeofday['Daytime']
	nowTime['isCivilDayTime'] = timeofday['Civildaytime']
	nowTime['isCivilNightTime'] = timeofday['Civilnighttime']
	nowTime['isNightTime'] = timeofday['Nighttime']
	nowTime['sunriseInMinutes'] = timeofday['SunriseInMinutes']
	nowTime['sunsetInMinutes'] = timeofday['SunsetInMinutes']
	nowTime['solarnoonInMinutes'] = timeofday['SunAtSouthInMinutes']
	nowTime['civTwilightStartInMinutes'] = timeofday['CivTwilightStartInMinutes']
	nowTime['civTwilightEndInMinutes'] = timeofday['CivTwilightEndInMinutes']

	-- the new instance
	local self = {
		['settings'] = settings,
		['commandArray'] = {},
		['devices'] = {},
		['scenes'] = {},
		['groups'] = {},
		['hardware'] = {},
		['changedDevices'] = {},
		['changedVariables'] = {},
		['security'] = globalvariables['Security'],
		['radixSeparator'] = globalvariables['radix_separator'],
		['time'] = nowTime,
		['startTime'] = Time(globalvariables['domoticz_start_time']),
		['systemUptime'] = tonumber(globalvariables['systemUptime']),
		['variables'] = {},
		['LOG_INFO'] = utils.LOG_INFO,
		['LOG_MODULE_EXEC_INFO'] = utils.LOG_MODULE_EXEC_INFO,
		['LOG_DEBUG'] = utils.LOG_DEBUG,
		['LOG_ERROR'] = utils.LOG_ERROR,
		['LOG_FORCE'] = utils.LOG_FORCE,
		utils = {
			_ = _
		}
	}

	merge(self, constants)
	merge(self.utils, utils)

	-- add domoticz commands to the commandArray or delay
	function self.sendCommand(command, value, delay)
		if delay and tonumber(delay) then
			self.emitEvent('___' .. command .. '__' , value ).afterSec(delay)
		else
			table.insert(self.commandArray, { [command] = value })
		end
		-- return a reference to the newly added item
		return self.commandArray[#self.commandArray], command, value, delay
	end

	-- have domoticz send a push notification
	function self.notify(subject, message, priority, sound, extra, subSystems, delay)
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

		if _subSystem:find('gcm') then
			utils.log('Notification subsystem Google Cloud Messaging (gcm) has been deprecated by Google. Switch to Firebase now!', utils.LOG_ERROR)
			_subSystem = _subSystem:gsub('gcm','fcm')
		end

		 local function strip(str)
			local stripped = tostring(str):gsub('#','')
			return stripped
		end

		local data = strip(subject)
				.. '#' .. strip(message)
				.. '#' .. strip(priority)
				.. '#' .. strip(sound)
				.. '#' .. strip(extra)
				.. '#' .. strip(_subSystem)
				self.sendCommand('SendNotification', data, delay)

	end

	-- have domoticz send an email
	function self.email(subject, message, mailTo, delay)
		if (mailTo == nil) then
			utils.log('No mail-to is provided', utils.LOG_ERROR)
		else
			if (subject == nil) then subject = '' end
			if (message == nil) then message = '' end
			self.sendCommand('SendEmail', subject .. '#' .. message .. '#' .. mailTo, delay)
		end
	end

	function self.triggershellCommandResponse(shellCommandResponse, delay, message)
		local shellCommandResponse = shellCommandResponse or _G.moduleLabel
		local delay = delay or 0
		local message = 'triggerShellCommandResponse: ' .. (message or shellCommandResponse)
		local command = 'echo '..message
		self.executeShellCommand
		(
			{
				command = command,
				callback = shellCommandResponse,
			}
		).afterSec(delay)
	end

	function self.triggerHTTPResponse(httpResponse, delay, message)
		local httpResponse = httpResponse or _G.moduleLabel
		local delay = delay or 0
		local message = 'triggerHTTPResponse: ' .. (message or httpResponse)
		local url = self.settings['Domoticz url'] .. '/json.htm?type=command&param=addlogmessage&message=' .. self.utils.urlEncode(message)
		self.openURL
		(
			{
				url = url,
				callback = httpResponse,
			}
		).afterSec(delay)
	end

	-- have domoticz send snapshot
	function self.snapshot(cameraIDS, subject)
		local subject = subject or 'domoticz'
		local cameraIDS = cameraIDS or 1

		local cameraIDS = ( type(cameraIDS) == 'table' and cameraIDS ) or utils.stringSplit(cameraIDS,'%p')

		for index, camera in ipairs(cameraIDS) do
			if tostring(camera):match('%a') then
				cameraIDS[index] = self.cameras(camera).id
			end
		end

		if not( cameraIDS[2] ) then
			local snapshotCommand = "SendCamera:" .. cameraIDS[1]
			return TimedCommand(self, snapshotCommand , subject, 'camera')
		else
			cameraIDS = table.concat(cameraIDS, ';')
			subject = self.utils.urlEncode(subject)
			return TimedCommand(self, 'OpenURL', self.settings['Domoticz url'] .. "/json.htm?type=command&param=emailcamerasnapshot&camidx=" .. cameraIDS .. '&subject=' .. subject , 'camera')
		end
	end

	-- have domoticz send an sms
	function self.sms(message, delay)
		self.sendCommand('SendSMS', message, delay)
	end

	function self.emitEvent(eventname, data)

		if (type(data) == 'table') then
			data = utils.toJSON(data)
		else
			data = tostring(data)
		end

		local eventinfo = {
			name = eventname,
			data = data
		}
		return TimedCommand(self, 'CustomEvent', eventinfo, 'emitEvent')
	end

	-- have domoticz execute a script
	function self.executeShellCommand(options)
		if (type(options)== 'string') then
			options = {
				command = options,
			}
		end
		if (type(options)=='table') then
			local command = options.command
			local callback = options.callback
			local sep = '/'
			local dataFolderPath = _G.dataFolderPath or ""  -- or needed for testing
			if dataFolderPath:find(string.char(92)) then sep = string.char(92) end -- Windows \
			local path = dataFolderPath .. sep

			local request = {
				command = options.command,
				callback = options.callback,
				timeout = options.timeout,
				path = path
			}

			utils.log('ExecuteShellCommand: command = ' .. _.str(request.command), utils.LOG_DEBUG)
			utils.log('ExecuteShellCommand: callback = ' .. _.str(request.callback), utils.LOG_DEBUG)
			utils.log('ExecuteShellCommand: timeout = ' .. _.str(request.timeout), utils.LOG_DEBUG)
			utils.log('ExecuteShellcommand: path = ' .. _.str(request.path),utils.LOG_DEBUG)
			return TimedCommand(self, 'ExecuteShellCommand', request, 'updatedevice')
		else
			utils.log('executeShellCommand: Invalid arguments, use either a string or a table with options', utils.LOG_ERROR)
		end

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
			local postData, headers

			-- process body data
			if (method ~= 'GET') then
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

			-- process headers
			if options.headers then
				headers = ''
				for key, value in pairs(options.headers) do
					headers = headers .. '!#' .. key .. ': ' .. tostring(value)
				end
			end

			local request = {
				URL = url,
				method = method,
				headers = headers,
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

	-- get information from hardware
	function self.hardwareInfo( id )
		local hardware, deviceNames, deviceIds = {}, {}, {}

		local hardwareDevices = self.devices().filter(function(dv)
			return dv.hardwareId == id or dv.hardwareName == id
		end).forEach(function(sdv)
			table.insert(deviceNames, sdv.name)
			table.insert(deviceIds, sdv.id)
		end)

		if #deviceNames > 0 then
			local aDevice = self.devices(deviceNames[1])

			hardware.name = aDevice.hardwareName
			hardware.id = aDevice.hardwareID
			hardware.type = aDevice.hardwareType
			hardware.typeValue = aDevice.TypeValue
			hardware.deviceNames = deviceNames
			hardware.deviceIds = deviceIds
		end

		return hardware
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

	function self.dump( file )
		self.utils.dumpTable(settings, '> ', file)
	end

	function self.logObject(object, file, objectType )
		self.utils.dumpTable(object, objectType .. '> ', file)
	end

	self.__cameras = {}
	self.__devices = {}
	self.__scenes = {}
	self.__groups = {}
	self.__hardware = {}
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
		elseif (baseType == 'hardware') then
			cache = self.__hardware
			constructor = Device
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

		local noObjectMessage = 'There is no ' .. baseType .. ' with that name or id: ' .. tostring(id)

		if (baseType == 'scene' or baseType == 'group' or baseType == 'hardware') then
			-- special case for hardware, scenes and groups
			-- as they may not be in the collection if Domoticz wasn't restarted after creating the hardware, scene or group.
			noObjectMessage = noObjectMessage .. '. If you just created the '.. baseType .. ' you may have to restart Domoticz to make it become visible to dzVents.'
		end
		utils.log(noObjectMessage, utils.LOG_ERROR)
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

	function self.hardware(id)
		if (id ~= nil) then
			return self._getObject('hardware', id)
		else
			return self._setIterators({}, true, 'hardware', false)
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
