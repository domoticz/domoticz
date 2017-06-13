local MAIN_METHOD = 'execute'
local GLOBAL_DATA_MODULE = 'global_data'
local GLOBAL = false
local LOCAL = true

local SCRIPT_DATA = 'data'
local GLOBAL_DATA = 'globalData'

local utils = require('Utils')
local persistence = require('persistence')

local HistoricalStorage = require('HistoricalStorage')

local function EventHelpers(settings, domoticz, mainMethod)

	local globalsDefinition

	local currentPath = debug.getinfo(1).source:match("@?(.*/)")

	if (_G.TESTMODE) then
		scriptsFolderPath = currentPath .. 'tests/scripts'
		package.path = package.path .. ';' .. currentPath .. '/tests/scripts/?.lua'
		package.path = package.path .. ';' .. currentPath .. '/tests/scripts/storage/?.lua'
		package.path = package.path .. ';' .. currentPath .. '/../?.lua'
	end

	if (settings == nil) then
		ok, settings = pcall(require, 'dzVents_settings')
		if (not ok) then
			utils.log('Error reading the settings file dzVents_settings.lua. Make sure you have it. You can rename dzVents_settings_example.lua and use that as an example.', utils.LOG_ERROR)
			utils.log(settings, utils.LOG_ERROR)
		end
	end

	_G.logLevel = settings['Log level']

	if (domoticz == nil) then
		local Domoticz = require('Domoticz')
		domoticz = Domoticz(settings)
	end

	local self = {
		['utils'] = utils, -- convenient for testing and stubbing
		['domoticz'] = domoticz,
		['settings'] = settings,
		['mainMethod'] = mainMethod or MAIN_METHOD,
		['deviceValueExtentions'] = {
			['_Temperature'] = true,
			['_Dewpoint'] = true,
			['_Humidity'] = true,
			['_Barometer'] = true,
			['_Utility'] = true,
			['_Weather'] = true,
			['_Rain'] = true,
			['_RainLastHour'] = true,
			['_UV'] = true
		}
	}

	if (_G.TESTMODE) then
		self.scriptsFolderPath = scriptsFolderPath
	end

	function self.getStorageContext(storageDef, module)

		local storageContext = {}
		local fileStorage, value

		if (storageDef ~= nil) then
			-- load the datafile for this module
			ok, fileStorage = pcall(require, module)
			package.loaded[module] = nil -- no caching
			if (ok) then
				-- only transfer data as defined in storageDef
				for var, def in pairs(storageDef) do

					if (def.history ~= nil and def.history == true) then
						storageContext[var] = HistoricalStorage(fileStorage[var], def.maxItems, def.maxHours, def.maxMinutes, def.getValue)
					else
						storageContext[var] = fileStorage[var]
					end
				end
			else
				for var, def in pairs(storageDef) do

					if (def.history ~= nil and def.history == true) then
						-- no initial value, just an empty history
						storageContext[var] = HistoricalStorage(fileStorage[var], def.maxItems, def.maxHours, def.maxMinutes, def.getValue)
					else
						if (storageDef[var].initial ~= nil) then
							storageContext[var] = storageDef[var].initial
						else
							storageContext[var] = nil
						end
					end
				end
			end
		end
		fileStorage = nil
		return storageContext
	end

	function self.writeStorageContext(storageDef, dataFilePath, dataFileModuleName, storageContext)

		local data = {}

		if (storageDef ~= nil) then
			-- transfer only stuf as described in storageDef
			for var, def in pairs(storageDef) do
				if (def.history ~= nil and def.history == true) then
					data[var] = storageContext[var]._getForStorage()
				else
					data[var] = storageContext[var]
				end
			end
			if (not utils.fileExists(scriptsFolderPath .. '/storage')) then
				os.execute('mkdir ' .. scriptsFolderPath .. '/storage')
			end

			ok, err = pcall(persistence.store, dataFilePath, data)

			-- make sure there is no cache for this 'data' module
			package.loaded[dataFileModuleName] = nil
			if (not ok) then
				utils.log('There was a problem writing the storage values', utils.LOG_ERROR)
				utils.log(err, utils.LOG_ERROR)
			end
		end
	end

	local function getEventInfo(eventHandler)
		local res = {}
		if (eventHandler.trigger ~= nil) then
			res.type = self.domoticz.EVENT_TYPE_TIMER
			res.trigger = eventHandler.trigger
		else
			res.type = self.domoticz.EVENT_TYPE_DEVICE
		end
		return res
	end

	function self.callEventHandler(eventHandler, device)
		local useStorage = false
		if (eventHandler[self.mainMethod] ~= nil) then

			-- ==================
			-- Prepare storage
			-- ==================
			if (eventHandler.data ~= nil) then
				useStorage = true
				local localStorageContext = self.getStorageContext(eventHandler.data, eventHandler.dataFileName)

				if (localStorageContext) then
					self.domoticz[SCRIPT_DATA] = localStorageContext
				else
					self.domoticz[SCRIPT_DATA] = {}
				end
			end

			if (globalsDefinition) then
				local globalStorageContext = self.getStorageContext(globalsDefinition, '__data_global_data')
				self.domoticz[GLOBAL_DATA] = globalStorageContext
			else
				self.domoticz[GLOBAL_DATA] = {}
			end

			-- ==================
			-- Run script
			-- ==================
			local info = getEventInfo(eventHandler)
			local ok, res = pcall(eventHandler[self.mainMethod], self.domoticz, device, info)
			if (ok) then

				-- ==================
				-- Persist storage
				-- ==================

				if (useStorage) then
					self.writeStorageContext(eventHandler.data,
						eventHandler.dataFilePath,
						eventHandler.dataFileName,
						self.domoticz[SCRIPT_DATA])
				end

				if (globalsDefinition) then
					self.writeStorageContext(globalsDefinition,
						scriptsFolderPath .. '/storage/__data_global_data.lua',
						scriptsFolderPath .. '/storage/__data_global_data',
						self.domoticz[GLOBAL_DATA])
				end

				self.domoticz[SCRIPT_DATA] = nil
				self.domoticz[GLOBAL_DATA] = nil

				return res
			else
				utils.log('An error occured when calling event handler ' .. eventHandler.name, utils.LOG_ERROR)
				utils.log(res, utils.LOG_ERROR) -- error info
			end
		else
			utils.log('No' .. self.mainMethod .. 'function found in event handler ' .. eventHandler, utils.LOG_ERROR)
		end

		self.domoticz[SCRIPT_DATA] = nil
		self.domoticz[GLOBAL_DATA] = nil
	end

	function self.reverseFind(s, target)
		-- string: 'this long string is a long string'
		-- string.findReverse('long') > 23, 26

		local reversed = string.reverse(s)
		local rTarget = string.reverse(target)
		-- reversed: gnirts gnol a si gnirts gnol siht
		-- rTarget = gnol

		local from, to = string.find(reversed, rTarget)
		if (from ~= nil) then
			-- return 1 less
			local targetPos = string.len(s) - to + 1
			return targetPos, targetPos + string.len(target) - 1
		else
			return nil, nil
		end
	end

	function self.getDeviceNameByEvent(event)
		-- event can be of the form <device name>_<value extension>
		-- where device name can contain underscores as well
		-- we have to extract the device name here and peel away the
		-- known value extension

		local pos, len = self.reverseFind(event, '_')

		local name = event
		if (pos ~= nil and pos > 1) then -- cannot start with _ (we use that for our _always script)
		local valueExtension = string.sub(event, pos)

		-- only peel away the first part if the extension is known
		if (self.deviceValueExtentions[valueExtension]) then
			name = string.sub(event, 1, pos - 1)
		end
		end
		return name
	end

	function self.scandir(directory)
		local pos, len
		local i, t, popen = 0, {}, io.popen
		local sep = string.sub(package.config, 1, 1)
		local cmd

		if (sep == '/') then
			cmd = 'ls -a "' .. directory .. '"'
		else
			-- assume windows for now
			cmd = 'dir "' .. directory .. '" /b /ad'
		end

		t = {}
		local pfile = popen(cmd)
		for filename in pfile:lines() do
			pos, len = string.find(filename, '.lua')

			if (pos and pos > 0) then
				table.insert(t, string.sub(filename, 1, pos - 1))
				utils.log('Found module in ' .. directory .. ' folder: ' .. t[#t], utils.LOG_DEBUG)
			end
		end
		pfile:close()
		return t
	end

	function self.getDayOfWeek(testTime)
		local d
		if (testTime ~= nil) then
			d = testTime.day
		else
			d = os.date('*t').wday
		end

		local lookup = { 'sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat' }
		utils.log('Current day .. ' .. lookup[d], utils.LOG_DEBUG)
		return lookup[d]
	end

	function self.getNow(testTime)
		if (testTime == nil) then
			local timenow = os.date("*t")
			return timenow
		else
			utils.log('h=' .. testTime.hour .. ' m=' .. testTime.min)
			return testTime
		end
	end

	function self.isTriggerByMinute(m, testTime)
		local time = self.getNow(testTime)
		return (time.min / m == math.floor(time.min / m))
	end

	function self.isTriggerByHour(h, testTime)
		local time = self.getNow(testTime)
		return (time.hour / h == math.floor(time.hour / h) and time.min == 0)
	end

	function self.isTriggerByTime(t, testTime)
		local tm, th
		local time = self.getNow(testTime)

		-- specials: sunset, sunrise
		if (t == 'sunset' or t == 'sunrise') then
			local minutesnow = time.min + time.hour * 60

			if (testTime ~= nil) then
				if (t == 'sunset') then
					return (minutesnow == testTime['SunsetInMinutes'])
				else
					return (minutesnow == testTime['SunriseInMinutes'])
				end
			else
				if (t == 'sunset') then
					return (minutesnow == timeofday['SunsetInMinutes'])
				else
					return (minutesnow == timeofday['SunriseInMinutes'])
				end
			end
		end

		local pos = string.find(t, ':')

		if (pos ~= nil and pos > 0) then
			th = string.sub(t, 1, pos - 1)
			tm = string.sub(t, pos + 1)

			if (tm == '*') then
				return (time.hour == tonumber(th))
			elseif (th == '*') then
				return (time.min == tonumber(tm))
			elseif (th ~= '*' and tm ~= '*') then
				return (tonumber(tm) == time.min and tonumber(th) == time.hour)
			else
				utils.log('wrong time format', utils.LOG_ERROR)
				return false
			end

		else
			utils.log('Wrong time format, should be hh:mm ' .. tostring(t), utils.LOG_DEBUG)
			return false
		end
	end

	function self.evalTimeTrigger(t, testTime)
		if (testTime) then utils.log(t, utils.LOG_INFO) end

		-- t is a single timer definition
		t = string.lower(t) -- normalize

		-- first get a possible on section (days)
		local onPos = string.find(t, ' on ')
		local days

		if (onPos ~= nil and onPos > 0) then
			days = string.sub(t, onPos + 4)
			t = string.sub(t, 1, onPos - 1)
		end

		-- now we can skip everything if the current day
		-- cannot be found in the days string
		if (days ~= nil and string.find(days, self.getDayOfWeek(testTime)) == nil) then
			-- today is not part of this trigger definition
			return false
		end

		local m, h
		local words = {}
		for w in t:gmatch("%S+") do
			table.insert(words, w)
		end

		-- specials
		if (t == 'every minute') then
			return self.isTriggerByMinute(1, testTime)
		end

		if (t == 'every other minute') then
			return self.isTriggerByMinute(2, testTime)
		end

		if (t == 'every hour') then
			return self.isTriggerByHour(1, testTime)
		end

		if (t == 'every other hour') then
			return self.isTriggerByHour(2, testTime)
		end

		-- others

		if (words[1] == 'every') then

			if (words[3] == 'minutes') then
				m = tonumber(words[2])
				if (m ~= nil) then
					return self.isTriggerByMinute(m, testTime)
				else
					utils.log(t .. ' is not a valid timer definition', utils.LOG_ERROR)
				end
			elseif (words[3] == 'hours') then
				h = tonumber(words[2])
				if (h ~= nil) then
					return self.isTriggerByHour(h, testTime)
				else
					utils.log(t .. ' is not a valid timer definition', utils.LOG_ERROR)
				end
			end
		elseif (words[1] == 'at' or words[1] == 'at:') then
			-- expect a time stamp
			local time = words[2]
			return self.isTriggerByTime(time, testTime)
		end
	end

	function self.handleEvents(events, device)
		if (type(events) ~= 'table') then
			return
		end

		for eventIdx, eventHandler in pairs(events) do
			utils.log('=====================================================', utils.LOG_MODULE_EXEC_INFO)
			utils.log('>>> Handler: ' .. eventHandler.name, utils.LOG_MODULE_EXEC_INFO)
			if (device) then
				utils.log('>>> Device: "' .. device.name .. '" Index: ' .. tostring(device.id), utils.LOG_MODULE_EXEC_INFO)
			end

			utils.log('.....................................................', utils.LOG_INFO)

			self.callEventHandler(eventHandler, device)

			utils.log('.....................................................', utils.LOG_INFO)
			utils.log('<<< Done ', utils.LOG_MODULE_EXEC_INFO)
			utils.log('-----------------------------------------------------', utils.LOG_MODULE_EXEC_INFO)
		end
	end

	function self.checkTimeDefs(timeDefs, testTime)
		-- accepts a table of timeDefs, if one of them matches with the
		-- current time, then it returns true
		-- otherwise it returns false
		for i, timeDef in pairs(timeDefs) do
			if (self.evalTimeTrigger(timeDef, testTime)) then
				return true, timeDef
			end
		end
		return false
	end

	function self.getEventBindings(mode)
		local bindings = {}
		local errModules = {}
		local ok, modules, moduleName, i, event, j, device
		ok, modules = pcall(self.scandir, scriptsFolderPath)
		if (not ok) then
			utils.log(modules, utils.LOG_ERROR)
			return nil
		end

		for i, moduleName in pairs(modules) do

			local module, skip
			ok, module = pcall(require, moduleName)

			if (ok) then

				if (moduleName == GLOBAL_DATA_MODULE) then
					if (module.data ~= nil) then
						globalsDefinition = module.data
						if (_G.TESTMODE) then
							self.globalsDefinition = globalsDefinition
						end
					else
						utils.log('Globals module has no storage section', utils.LOG_ERROR)
					end
				else
					if (type(module) == 'table') then
						skip = false
						if (module.active ~= nil) then
							local active = false
							if (type(module.active) == 'function') then
								active = module.active(self.domoticz)
							else
								active = module.active
							end

							if (not active) then
								skip = true
							end
						end
						if (not skip) then
							if (module.on ~= nil and module[self.mainMethod] ~= nil) then
								module.name = moduleName
								module.dataFileName = '__data_' .. moduleName
								module.dataFilePath = scriptsFolderPath .. '/storage/__data_' .. moduleName .. '.lua'
								for j, event in pairs(module.on) do
									if (mode == 'timer') then
										if (type(j) == 'number' and type(event) == 'string' and event == 'timer') then
											-- { 'timer' }
											-- execute every minute (old style)
											module.trigger = event
											table.insert(bindings, module)
										elseif (type(j) == 'string' and j == 'timer' and type(event) == 'string') then
											-- { ['timer'] = 'every minute' }
											if (self.evalTimeTrigger(event)) then
												module.trigger = event
												table.insert(bindings, module)
											end
										elseif (type(j) == 'string' and j == 'timer' and type(event) == 'table') then
											-- { ['timer'] = { 'every minute ', 'every hour' } }
											local triggered, def = self.checkTimeDefs(event)
											if (triggered) then
												-- this one can be executed
												module.trigger = def
												table.insert(bindings, module)
											end
										end
									else
										if (event ~= 'timer' and j ~= 'timer') then

											if (type(j) == 'string' and j == 'devices' and type(event) == 'table') then

												-- { ['devices'] = { 'devA', 'devB', .. }

												for devIdx, devName in pairs(event) do
													if (bindings[devName] == nil) then
														bindings[devName] = {}
													end
													table.insert(bindings[devName], module)
												end

											else
												-- let's not try to resolve indexes to names here for performance reasons
												if (bindings[event] == nil) then
													bindings[event] = {}
												end
												table.insert(bindings[event], module)
											end
										end
									end
								end
							else
								utils.log('Script ' .. moduleName .. '.lua has no "on" and/or "' .. self.mainMethod .. '" section. Skipping', utils.LOG_ERROR)
								table.insert(errModules, moduleName)
							end
						end
					else
						utils.log('Script ' .. moduleName .. '.lua is not a valid module. Skipping', utils.LOG_ERROR)
						table.insert(errModules, moduleName)
					end
				end
			else
				table.insert(errModules, moduleName)
				utils.log(module, utils.LOG_ERROR)
			end
		end

		return bindings, errModules
	end

	function self.getTimerHandlers()
		return self.getEventBindings('timer')
	end

	function self.fetchHttpDomoticzData(ip, port, interval)

		local sep = string.sub(package.config, 1, 1)
		if (sep ~= '/') then return end -- only on linux

		if (ip == nil or port == nil) then
			utils.log('Invalid ip for contacting Domoticz', utils.LOG_ERROR)
			return
		end

		if (self.evalTimeTrigger(interval)) then
			self.utils.requestDomoticzData(ip, port)
		end
	end

	function self.dumpCommandArray(commandArray)
		local printed = false
		for k, v in pairs(commandArray) do
			if (type(v) == 'table') then
				for kk, vv in pairs(v) do
					utils.log('[' .. k .. '] = ' .. kk .. ': ' .. vv, utils.LOG_MODULE_EXEC_INFO)
				end
			else
				utils.log(k .. ': ' .. v, utils.LOG_MODULE_EXEC_INFO)
			end
			printed = true
		end
		if (printed) then utils.log('=====================================================', utils.LOG_MODULE_EXEC_INFO) end
	end

	function self.findScriptForChangedDevice(changedDeviceName, allEventScripts)
		-- event could be like: myPIRLivingRoom
		-- or myPir(.*)
		utils.log('Searching for scripts for changed device: ' .. changedDeviceName, utils.LOG_DEBUG)

		for scriptTrigger, scripts in pairs(allEventScripts) do
			if (string.find(scriptTrigger, '*')) then -- a wild-card was use
			-- turn it into a valid regexp
			scriptTrigger = string.gsub(scriptTrigger, "*", ".*")

			if (string.match(changedDeviceName, scriptTrigger)) then
				-- there is trigger for this changedDeviceName
				return scripts
			end
			else
				if (scriptTrigger == changedDeviceName) then
					-- there is trigger for this changedDeviceName
					return scripts
				end
			end
		end

		return nil
	end

	function self.dispatchDeviceEventsToScripts(changedDevices)
		if (changedDevices == nil) then
			-- get it from the globals
			changedDevices = devicechanged
		end

		local allEventScripts = self.getEventBindings()

		local handledDevices = {}

		if (changedDevices ~= nil) then

			-- check if there's a new devices.lua file waiting to be activated
			utils.activateDevicesFile()

			for changedDeviceName, changedDeviceValue in pairs(changedDevices) do

				utils.log('Event in devicechanged: ' .. changedDeviceName .. ' value: ' .. changedDeviceValue, utils.LOG_DEBUG)
				local scriptsToExecute

				-- find the device for this name
				-- could be MySensor or MySensor_Temperature
				-- the device returned would be MySensor in that case
				local baseName = self.getDeviceNameByEvent(changedDeviceName)
				local device = self.domoticz.devices[baseName]

				if (device ~= nil) then

					if (handledDevices[tostring(device.id)] == nil) then -- make sure a device is only handled once (so not for MySensor and MySensor_Temperature)

						-- first search by name
						scriptsToExecute = self.findScriptForChangedDevice(device.name, allEventScripts)

						if (scriptsToExecute == nil) then
							-- search by id
							scriptsToExecute = allEventScripts[device.id]
						end

						if (scriptsToExecute ~= nil) then
							utils.log('Handling events for: "' .. changedDeviceName .. '", value: "' .. changedDeviceValue .. '"', utils.LOG_INFO)
							self.handleEvents(scriptsToExecute, device)
						end

						-- mark as handled
						handledDevices[tostring(device.id)] = true
					else
						utils.log('Skipping: ' .. changedDeviceName .. '. Already processed', utils.LOG_DEBUG)
					end
				else
					-- this is weird.. basically impossible because the list of device objects is based on what
					-- Domoticz passes along.
				end
			end
		end
		self.dumpCommandArray(self.domoticz.commandArray)
		return self.domoticz.commandArray
	end

	function self.autoFetchHttpDomoticzData()
		-- this is indirectly triggered by the timer script
		if (settings['Enable http fetch']) then
			self.fetchHttpDomoticzData(settings['Domoticz ip'],
				settings['Domoticz port'],
				settings['Fetch interval'])
		end
	end

	function self.dispatchTimerEventsToScripts()
		local scriptsToExecute = self.getTimerHandlers()

		self.handleEvents(scriptsToExecute)

		self.autoFetchHttpDomoticzData()

		-- check if there's a new devices.lua file waiting to be activated
		utils.activateDevicesFile()

		self.dumpCommandArray(self.domoticz.commandArray)

		return self.domoticz.commandArray
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

	return self
end

return EventHelpers