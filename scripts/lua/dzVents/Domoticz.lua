local scriptPath = globalvariables['script_path']
package.path = package.path .. ';' .. scriptPath .. '?.lua'

local EventHelpers = require('EventHelpers')
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

	local now = os.date('*t')
	local sNow = now.year .. '-' .. now.month .. '-' .. now.day .. ' ' .. now.hour .. ':' .. now.min .. ':' .. now.sec
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
		['security'] = globalvariables["Security"],
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
		['BARO_STABLE'] = 0,
		['BARO_SUNNY'] = 1,
		['BARO_CLOUDY'] = 2,
		['BARO_UNSTABLE'] = 3,
		['BARO_THUNDERSTORM'] = 4,
		['BARO_UNKNOWN'] = 5,
		['BARO_CLOUDY_RAIN'] = 6,
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
		['EVOHOME_MODE_AUTO'] = 'Auto',
		['EVOHOME_MODE_TEMPORARY_OVERRIDE'] = 'TemporaryOverride',
		['EVOHOME_MODE_PERMANENT_OVERRIDE'] = 'PermanentOverride'
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
	function self.notify(subject, message, priority, sound)
		-- set defaults
		if (priority == nil) then priority = self.PRIORITY_NORMAL end
		if (message == nil) then message = '' end
		if (sound == nil) then sound = self.SOUND_DEFAULT end

		self.sendCommand('SendNotification', subject .. '#' .. message .. '#' .. tostring(priority) .. '#' .. tostring(sound))
	end

	-- have domoticz send an email
	function self.email(subject, message, mailTo)
		if (mailTo == nil) then
			utils.log('No mail to is provide', utils.LOG_DEBUG)
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

	function self.fetchHttpDomoticzData()
		utils.requestDomoticzData(self.settings['Domoticz ip'],
			self.settings['Domoticz port'])
	end

	function self.log(message, level)
		utils.log(message, level)
	end

	-- bootstrap the variables section
	local function createVariables()
		for name, value in pairs(uservariables) do
			local var = Variable(self, name, value)
			self.variables[name] = var
		end
	end

	-- process a otherdevices table for a given attribute and
	-- set the attribute on the appropriate device object
	local function setDeviceAttribute(otherdevicesTable, attribute, tableName)
		for name, value in pairs(otherdevicesTable) do
			-- utils.log('otherdevices table :' .. name .. ' value: ' .. value, utils.LOG_DEBUG)
			if (name ~= nil and name ~= '') then -- sometimes domoticz seems to do this!! ignore...

			-- get the device
			local device = self.devices[name]

			if (device == nil) then
				utils.log('Cannot find the device. Skipping:  ' .. name .. ' ' .. value, utils.LOG_ERROR)
			else
				if (attribute == 'lastUpdate') then
					device.addAttribute(attribute, Time(value))
				elseif (attribute == 'rawData') then
					device._sValues = value
					device.addAttribute(attribute, string.split(value, ';'))
				elseif (attribute == 'id') then
					device.addAttribute(attribute, value)

					-- create lookup by id
					self.devices[value] = device

					-- create the changedDevices entry when changed
					-- we do it at this moment because at this stage
					-- the device just got his id
					if (device.changed) then
						self.changedDevices[device.name] = device
						self.changedDevices[value] = device -- id lookup
					end
				else
					device.addAttribute(attribute, value)
				end

				if (tableName ~= nil) then
					local deviceAttributeName = name .. '_' ..
							string.upper(string.sub(tableName, 1, 1)) ..
							string.sub(tableName, 2)

					-- now we have to transfer the changed information for attributes
					-- if that is availabel
					if (devicechanged and devicechanged[deviceAttributeName] ~= nil) then
						device.setAttributeChanged(attribute)
					end
				end
			end
			end
		end
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
			end
		end
	end

	-- doesn't seem to work well for some weird reasone
	function self.logDevice(device)
		dumpTable(device, '> ')
	end

	local function readHttpDomoticzData()
		local httpData = {
			['result'] = {}
		}

		-- figure out what os this is
		local sep = string.sub(package.config, 1, 1)
		if (sep ~= '/') then return httpData end -- only on linux

		if utils.fileExists(utils.getDevicesPath()) then
			local ok, module

			ok, module = pcall(require, 'devices')
			if (ok) then
				if (type(module) == 'table') then
					httpData = module
				end
			else
				-- cannot be loaded
				utils.log('devices.lua cannot be loaded', utils.LOG_ERROR)
				utils.log(module, utils.LOG_ERROR)
			end
		else
			if (self.settings['Enable http fetch']) then
				self.fetchHttpDomoticzData()
			end
		end
		return httpData
	end

	if (_G.TESTMODE) then
		self._readHttpDomoticzData = readHttpDomoticzData
	end

	local function createDevices()
		-- first create the device objects
		for name, state in pairs(otherdevices) do
			local wasChanged = (devicechanged ~= nil and devicechanged[name] ~= nil)
			self.devices[name] = Device(self, name, state, wasChanged)
		end

		-- then fill them with attributes from the
		-- global tables handed over by Domoticz
		for tableName, tableData in pairs(_G) do

			-- only deal with global <otherdevices_*> tables
			if (string.find(tableName, 'otherdevices_') ~= nil and
				string.find(tableName, 'otherdevices_scenesgroups') == nil) then
				utils.log('Found ' .. tableName .. ' adding this as a possible attribute', utils.LOG_DEBUG)
				-- extract the part after 'otherdevices_'
				-- That is the unprocesses attribute name
				local oriAttribute = string.sub(tableName, 14)
				local attribute = oriAttribute

				-- now process some specials
				if (attribute) == 'idx' then
					attribute = 'id'
				end
				if (attribute == 'lastupdate') then
					attribute = 'lastUpdate'
				end
				if (attribute == 'svalues') then
					attribute = 'rawData'
				end
				if (attribute == 'rain_lasthour') then
					attribute = 'rainLastHour'
				end

				-- now let's get and store the stuff
				setDeviceAttribute(tableData, attribute, oriAttribute)
			end
		end
	end

	local function createScenesAndGroups()
		local httpData = readHttpDomoticzData()
		if (httpData) then
			for i, httpDevice in pairs(httpData.result) do

				if (httpDevice.Type == 'Scene' or httpDevice.Type == 'Group') then
					local name = httpDevice.Name
					local state = httpDevice.Status -- can be nil
					local id = tonumber(httpDevice.idx)
					local raw = httpDevice.Data
					local device = Device(self, name, state, false)
					device.addAttribute('id', id)
					device.addAttribute('lastUpdate', Time(httpDevice.LastUpdate))
					device.addAttribute('rawData', string.split(raw, ';'))

					if (httpDevice.Type == 'Scene') then
						if (self.scenes[name] ~= nil) then
							utils.log('Scene found with a duplicate name. This scene will be ignored. Please rename: ' .. name, utils.LOG_ERROR)
						else
							self.scenes[name] = device
							self.scenes[id] = device
						end
					else
						if (self.groups[name] ~= nil) then
							utils.log('Group found with a duplicate name. This group will be ignored. Please rename: ' .. name, utils.LOG_ERROR)
						else
							self.groups[name] = device
							self.groups[id] = device
						end
					end
				end
			end
		end
	end

	local function updateGroupAndScenes()
		-- assume that the groups and scenes have been created using the http data first
		if (_G.otherdevices_scenesgroups) then

			for name, state in pairs(_G.otherdevices_scenesgroups) do

				-- name is either a scene or a group
				local device = (self.scenes and self.scenes[name]) or (self.groups and self.groups[name])

				if (device) then
					device._setStateAttribute(state)
				end
			end
		end
	end

	local function createMissingHTTPDevices()
		local httpData = readHttpDomoticzData()

		if (httpData) then
			for i, httpDevice in pairs(httpData.result) do
				local id = tonumber(httpDevice.idx)
				if (httpData.Type ~= 'Group' and httpData.Type ~= 'Scene') then
					if (self.devices[id] == nil) then
						-- we have a device that is not passed by Domoticz to the scripts
						local name = httpDevice.Name
						local state = httpDevice.Status -- can be nil
						local raw = httpDevice.Data
						local device = Device(self, name, state, false)

						self.devices[name] = device
						self.devices[id] = device

						device.addAttribute('id', id)
						device.addAttribute('lastUpdate', Time(httpDevice.LastUpdate))
						device.addAttribute('rawData', string.split(raw, ';'))
					end
				end
			end
		end
	end

	local function getValueFromFormatted(value, unit)
		--local s = string.gsub(value, '%.', '')
		local s = string.gsub(value, '%,', '') -- remove , (assume it is NOT a decimal separator)
		s = string.gsub(s, ' ' .. unit, '')
		return tonumber(s)
	end

	local function extendDevicesWithHTTPData()
		local httpData = readHttpDomoticzData()

		if (httpData) then
			for i, httpDevice in pairs(httpData.result) do
				if (self.devices[tonumber(httpDevice['idx'])]) then

					local device
					local id = tonumber(httpDevice.idx)

					if (httpDevice.Type == 'Scene' or httpDevice.Type == 'Group') then
						if (httpDevice.Type == 'Scene') then
							device = self.scenes[id]
						else
							device = self.groups[id]
						end
					else
						device = self.devices[id]
					end

					if (device == nil) then
						-- oops
						-- something is wrong
						utils.log('Cannot find a device with this id: ' .. tostring(id), utils.LOG_ERROR)
						self.logDevice(httpDevice)
					else

						device.addAttribute('description', httpDevice.Description)
						device.addAttribute('batteryLevel', httpDevice.BatteryLevel)
						device.addAttribute('signalLevel', httpDevice.SignalLevel)
						device.addAttribute('deviceSubType', httpDevice.SubType)

						if (device.level == nil) then
							-- for those non-dimmer-like devices that do have a level
							device.addAttribute('level', httpDevice.Level)
						end

						device.addAttribute('deviceType', httpDevice.Type)
						device.addAttribute('hardwareName', httpDevice.HardwareName)
						device.addAttribute('hardwareType', httpDevice.HardwareType)
						device.addAttribute('hardwareId', httpDevice.HardwareID)
						device.addAttribute('hardwareTypeValue', httpDevice.HardwareTypeVal)
						device.addAttribute('hardwareTypeVal', httpDevice.HardwareTypeVal)
						device.addAttribute('switchType', httpDevice.SwitchType)
						device.addAttribute('switchTypeValue', httpDevice.SwitchTypeVal)
						device.addAttribute('timedOut', httpDevice.HaveTimeout)
						device.addAttribute('counterToday', httpDevice.CounterToday or '')
						device.addAttribute('counterTotal', httpDevice.Counter or '')


						if (device.deviceType == 'Heating' and device.deviceSubType == 'Zone') then
							device.addAttribute('setPoint', tonumber(device.rawData[2]))
							device.addAttribute('heatingMode', device.rawData[3])
						end

						if (device.deviceType == 'Lux' and device.deviceSubType == 'Lux') then
							device.addAttribute('lux', tonumber(device.rawData[1]))
						end

						if (device.deviceType == 'General' and device.deviceSubType == 'kWh') then
							device.addAttribute('WhTotal', tonumber(device.rawData[2]))
							device.addAttribute('WActual', tonumber(device.rawData[1]))
							local todayFormatted = httpDevice.CounterToday or ''
							-- risky business, we assume no decimals, just thousands separators
							-- there is no raw value available for today
							local s = string.gsub(todayFormatted, '%,', '')
							--s = string.gsub(s, '%,', '')
							s = string.gsub(s, ' kWh', '')
							device.addAttribute('WhToday', tonumber(s))
						end

						if (device.deviceType == 'Usage' and device.deviceSubType == 'Electric') then
							device.addAttribute('WActual', tonumber(device.rawData[1]))
						end


						if (device.deviceType == 'P1 Smart Meter' and device.deviceSubType == 'Energy') then
							device.addAttribute('WActual', tonumber(device.rawData[5]))
						end

						if (device.deviceType == 'Thermostat' and device.deviceSubType == 'SetPoint') then
							device.addAttribute('setPoint', tonumber(device.rawData[1]))
						end

						if (device.deviceSubType == 'Text') then
							device.addAttribute('text', httpDevice.Data) -- could be old because it may not be fetched
						end
					end
				end
			end
		end
	end

	createVariables()
	createDevices()
	createScenesAndGroups()
	updateGroupAndScenes()
	createMissingHTTPDevices()
	extendDevicesWithHTTPData()

	setIterators(self.devices)
	setIterators(self.changedDevices)
	setIterators(self.variables)
	setIterators(self.scenes)
	setIterators(self.groups)


	return self
end

return Domoticz
