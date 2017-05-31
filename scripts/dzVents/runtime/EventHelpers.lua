local GLOBAL_DATA_MODULE = 'global_data'
local GLOBAL = false
local LOCAL = true

local SCRIPT_DATA = 'data'
local GLOBAL_DATA = 'globalData'

local utils = require('Utils')
local persistence = require('persistence')

local HistoricalStorage = require('HistoricalStorage')

local function EventHelpers(domoticz, mainMethod)

	local globalsDefinition

	local currentPath = globalvariables['script_path']

	if (_G.TESTMODE) then
		-- make sure you run the tests from the tests folder !!!!
		_G.scriptsFolderPath = currentPath .. 'scripts'
		package.path = package.path .. ';' .. currentPath .. 'scripts/?.lua'
		package.path = package.path .. ';' .. currentPath .. 'scripts/storage/?.lua'
		package.path = package.path .. ';' .. currentPath .. '/../?.lua'


	end

	local settings = {
		['Log level'] = tonumber(globalvariables['dzVents_log_level']) or  1,
		['Domoticz url'] = 'http://127.0.0.1:' .. (tostring(globalvariables['domoticz_listening_port']) or "8080")
	}

	_G.logLevel = settings['Log level']

	if (domoticz == nil) then
		local Domoticz = require('Domoticz')
		domoticz = Domoticz(settings)
	end

	local self = {
		['utils'] = utils, -- convenient for testing and stubbing
		['domoticz'] = domoticz,
		['settings'] = settings,
	}

	if (_G.TESTMODE) then
		self.scriptsFolderPath = scriptsFolderPath
		function self._getUtilsInstance()
			return utils
		end
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

	local function getEventInfo(eventHandler, mode)
		local res = {}
		res.type = mode
		if (eventHandler.trigger ~= nil) then
			res.trigger = eventHandler.trigger
		end
		return res
	end

	function self.callEventHandler(eventHandler, device, variable, security)


		local useStorage = false


		if (eventHandler['execute'] ~= nil) then

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
			local ok, res, info


			if (device ~= nil) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_DEVICE)
				ok, res = pcall(eventHandler['execute'], self.domoticz, device, info)
			elseif (variable ~= nil) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_VARIABLE)
				ok, res = pcall(eventHandler['execute'], self.domoticz, variable, info)
			elseif (security ~= nil) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_SECURITY)
				ok, res = pcall(eventHandler['execute'], self.domoticz, security, info)
			else
				-- timer
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_TIMER)
				ok, res = pcall(eventHandler['execute'], self.domoticz, nil, info)
			end

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
			utils.log('No "execute" function found in event handler ' .. eventHandler, utils.LOG_ERROR)
		end

		self.domoticz[SCRIPT_DATA] = nil
		self.domoticz[GLOBAL_DATA] = nil
	end

	function self.scandir(directory)
		local pos, len
		local i, t, popen = 0, {}, io.popen
		local sep = string.sub(package.config, 1, 1)
		local cmd

		if (directory == nil) then
			return {}
		end

		if (sep == '/') then
			cmd = 'ls -a "' .. directory .. '"'
		else
			-- assume windows for now
			cmd = 'dir "' .. directory .. '" /B'
		end

		t = {}
		local pfile = popen(cmd)
		for filename in pfile:lines() do
			pos, len = string.find(filename, '.lua', 1, true)
			if (pos and pos > 0 and filename:sub(1, 1) ~= '.' and len == string.len(filename)) then

				table.insert(t, {
					['type'] = 'external',
					['name'] = string.sub(filename, 1, pos - 1)
				})

				utils.log('Found module in ' .. directory .. ' folder: ' .. t[#t].name, utils.LOG_DEBUG)
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
		local onPosStart = string.find(t, '^on ')
		local days

		if (onPos ~= nil and onPos > 0) then
			-- days is the remainder of the string starting after 'on '
			days = string.sub(t, onPos + 4)
			-- make t to be everything before the on part
			t = string.sub(t, 1, onPos - 1)
		elseif (onPosStart~= nil and onPosStart == 1) then
			-- starts with 'on '
			days = string.sub(t, 3)
			t = nil -- we stop further processing in this case
		end

		-- now we can skip everything if the current day
		-- cannot be found in the days string
		if (days ~= nil and string.find(days, self.getDayOfWeek(testTime)) == nil) then
			-- today is not part of this trigger definition
			return false
		end

		if (t == nil) then return true end

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

		return true
	end

	function self.handleEvents(events, device, variable, security)

		local originalLogLevel = _G.logLevel -- a script can override the level

		function restoreLogging()
			_G.logLevel = originalLogLevel
			_G.logMarker = nil
		end

		if (type(events) ~= 'table') then
			return
		end

		for eventIdx, eventHandler in pairs(events) do

			if (eventHandler.logging) then
				if (eventHandler.logging.level ~= nil) then
					_G.logLevel = eventHandler.logging.level
				end
				if (eventHandler.logging.marker ~= nil) then
					_G.logMarker = eventHandler.logging.marker
				end
			end


			utils.log('=====================================================', utils.LOG_MODULE_EXEC_INFO)
			utils.log('>>> Handler: ' .. eventHandler.name .. '.lua', utils.LOG_MODULE_EXEC_INFO)

			if (device) then
				utils.log('>>> Device: "' .. device.name .. '" Index: ' .. tostring(device.id), utils.LOG_MODULE_EXEC_INFO)
			elseif (variable) then
				utils.log('>>> Variable: "' .. variable.name .. '" Index: ' .. tostring(variable.id), utils.LOG_MODULE_EXEC_INFO)
			elseif (security) then
				utils.log('>>> Security: "' .. security .. '"', utils.LOG_MODULE_EXEC_INFO)
			end

			utils.log('.....................................................', utils.LOG_INFO)

			self.callEventHandler(eventHandler, device, variable, security)

			utils.log('.....................................................', utils.LOG_INFO)
			utils.log('<<< Done ', utils.LOG_MODULE_EXEC_INFO)
			utils.log('-----------------------------------------------------', utils.LOG_MODULE_EXEC_INFO)

			restoreLogging()
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

	function addBindingEvent(bindings, event, module)
		if (bindings[event] == nil) then
			bindings[event] = {}
		end
		table.insert(bindings[event], module)
	end

	function self.getEventBindings(mode, testTime)
		local bindings = {}
		local errModules = {}
		local internalScripts = {}
		local hasInternals = false
		local ok, diskScripts, moduleName, i, event, j, device
		local modules = {}


		ok, diskScripts = pcall(self.scandir, _G.scriptsFolderPath)

		if (not ok) then
			utils.log(diskScripts, utils.LOG_ERROR)
		end

		if (_G.scripts == nil) then _G.scripts = {} end

		-- prepare internal modules
		-- todo this could be done entirely in c++
		for name, script in pairs(_G.scripts) do
			table.insert(modules, {
				['type'] = 'internal',
				['code'] = script,
				['name'] = name
			})
		end

		for i, external in pairs(diskScripts) do
			table.insert(modules, external)
		end

		if (mode == nil) then mode = 'device' end

		for i, moduleInfo in pairs(modules) do

			local module, skip

			local moduleName = moduleInfo.name

			_G.domoticz = {
				['LOG_INFO'] = utils.LOG_INFO,
				['LOG_MODULE_EXEC_INFO'] = utils.LOG_MODULE_EXEC_INFO,
				['LOG_DEBUG'] = utils.LOG_DEBUG,
				['LOG_ERROR'] = utils.LOG_ERROR,
				['SECURITY_DISARMED'] = self.domoticz.SECURITY_DISARMED,
				['SECURITY_ARMEDAWAY'] = self.domoticz.SECURITY_ARMEDAWAY,
				['SECURITY_ARMEDHOME'] = self.domoticz.SECURITY_ARMEDHOME,
			}
			ok = true

			if (moduleInfo.type == 'external') then
				ok, module = pcall(require, moduleName)
			else
				module, err = loadstring(moduleInfo.code)
				if (module == nil) then
					module = moduleInfo.name .. ': ' .. err
					ok = false
				else
					module = module()
				end
			end

			_G.domoticz = nil

			if (ok) then

				if (moduleName == GLOBAL_DATA_MODULE) then
					if (module.data ~= nil) then
						globalsDefinition = module.data
						if (_G.TESTMODE) then
							self.globalsDefinition = globalsDefinition
						end
					end

					if (module.helpers ~= nil) then
						self.domoticz.helpers = module.helpers
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
							if (module.on ~= nil and module['execute'] ~= nil) then
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
									elseif (mode == 'device') then
										if (event ~= 'timer' and j ~= 'timer' and j~= 'variable' and j~='variables' and j~='security') then

											if (type(j) == 'string' and j == 'devices' and type(event) == 'table') then

												-- { ['devices'] = { 'devA', ['devB'] = { ..timedefs }, .. }

												for devIdx, devName in pairs(event) do

													-- detect if devName is of the form ['devB'] = { 'every hour' }
													if (type(devName) == 'table') then
														local triggered, def = self.checkTimeDefs(devName, testTime)
														if (triggered) then
															addBindingEvent(bindings, devIdx, module)
														end
													else
														-- a single device name (or id)
														addBindingEvent(bindings, devName, module)
													end
												end

											elseif (type(j) == 'string' and j ~= 'devices' and type(event) == 'table') then
												-- [devicename] = { ...timedefs}
												local triggered, def = self.checkTimeDefs(event, testTime)
												if (triggered) then
													addBindingEvent(bindings, j, module)
												end
											else
												-- single device name or id
												-- let's not try to resolve indexes to names here for performance reasons
												addBindingEvent(bindings, event, module)
											end
										end
									elseif (mode == 'variable') then
										if (type(j) == 'string' and j == 'variable'  and type(event) == 'string') then
											-- { ['variable'] = 'myvar' }
											addBindingEvent(bindings, event, module)
										elseif (type(j) == 'string' and j == 'variables' and type(event) == 'table') then
											-- { ['variables'] = { 'varA', 'varB' }
											for devIdx, varName in pairs(event) do
												addBindingEvent(bindings, varName, module)
											end
										end
									elseif (mode == 'security') then
										if (type(j) == 'string' and j == 'security' and type(event) == 'string') then

											if (event == self.domoticz.security) then
												table.insert(bindings, module)
											end
										end
									end
								end
							else
								utils.log('Script ' .. moduleName .. '.lua has no "on" and/or "execute" section. Skipping', utils.LOG_ERROR)
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

	function self.getVariableHandlers()
		return self.getEventBindings('variable')
	end

	function self.getSecurityHandlers()
		return self.getEventBindings('security')
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

	function self.findScriptForChangedItem(changedItemName, allEventScripts)
		-- event could be like: myPIRLivingRoom
		-- or myPir(.*)
		utils.log('Searching for scripts for changed item: ' .. changedItemName, utils.LOG_DEBUG)

		--[[

			allEventScripts is a dictionary where
			each key is the name or id of a device and the value
			is a table with all the modules for this device

			{
				['myDevice'] = {
					modA, modB, modC
				},
				['anotherDevice'] = {
					modD
				},
				12 = {
					modE, modF
				},
				['myDev*'] = {
					modG, modH
				}
			}

		]]--

		local modules

		-- only search for named and wildcard triggers,
		-- id is done later

		for scriptTrigger, scripts in pairs(allEventScripts) do
			if (string.find(scriptTrigger, '*')) then -- a wild-card was use
				-- turn it into a valid regexp
				scriptTrigger = string.gsub(scriptTrigger, "*", ".*")

				if (string.match(changedItemName, scriptTrigger)) then
					-- there is trigger for this changedItemName

					if modules == nil then modules = {} end

					for i, mod in pairs(scripts) do
						table.insert(modules, mod)
					end

				end

			else
				if (scriptTrigger == changedItemName) then
					-- there is trigger for this changedItemName

					if modules == nil then modules = {} end

					for i, mod in pairs(scripts) do
						table.insert(modules, mod)
					end

				end
			end
		end

		return modules
	end

	function self.dispatchDeviceEventsToScripts(domoticz)

		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local allEventScripts = self.getEventBindings()

		domoticz.changedDevices.forEach( function(device)

			utils.log('Device-event for: ' .. device.name .. ' value: ' .. device.state, utils.LOG_DEBUG)

			local scriptsToExecute

			-- first search by name

			scriptsToExecute = self.findScriptForChangedItem(device.name, allEventScripts)

			if (scriptsToExecute == nil) then
				-- search by id
				scriptsToExecute = allEventScripts[device.id]
			end

			if (scriptsToExecute ~= nil) then
				utils.log('Handling events for: "' .. device.name .. '", value: "' .. device.state .. '"', utils.LOG_INFO)
				self.handleEvents(scriptsToExecute, device, nil, nil)
			end

		end)


		self.dumpCommandArray(self.domoticz.commandArray)
		return self.domoticz.commandArray
	end

	function self.dispatchTimerEventsToScripts()
		local scriptsToExecute = self.getTimerHandlers()

		self.handleEvents(scriptsToExecute)
		self.dumpCommandArray(self.domoticz.commandArray)

		return self.domoticz.commandArray

	end

	function self.dispatchSecurityEventsToScripts()
		local scriptsToExecute = self.getSecurityHandlers()
		self.handleEvents(scriptsToExecute, nil, nil, self.domoticz.security)
		self.dumpCommandArray(self.domoticz.commandArray)

		return self.domoticz.commandArray
	end


	function self.dispatchVariableEventsToScripts(domoticz)
		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local allEventScripts = self.getVariableHandlers()

		domoticz.changedVariables.forEach(function(variable)

			utils.log('Variable-event for: ' .. variable.name .. ' value: ' .. variable.value, utils.LOG_DEBUG)

			local scriptsToExecute

			-- first search by name

			scriptsToExecute = self.findScriptForChangedItem(variable.name, allEventScripts)

			if (scriptsToExecute == nil) then
				-- search by id
				scriptsToExecute = allEventScripts[variable.id]
			end

			if (scriptsToExecute ~= nil) then
				utils.log('Handling variable-events for: "' .. variable.name .. '", value: "' .. variable.value .. '"', utils.LOG_INFO)
				self.handleEvents(scriptsToExecute, nil, variable, nil)
			end
		end)


		self.dumpCommandArray(self.domoticz.commandArray)
		return self.domoticz.commandArray
	end


	return self
end

return EventHelpers