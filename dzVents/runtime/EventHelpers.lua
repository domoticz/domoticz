local _ = require('lodash')
local GLOBAL_DATA_MODULE = 'global_data'
local utils = require('Utils')
local persistence = require('persistence')
local HTTPResponse = require('HTTPResponse')
local ShellCommandResponse = require('ShellCommandResponse')
local Timer = require('Timer')
local Security = require('Security')
local SystemEvent = require('SystemEvent')
local CustomEvent = require('CustomEvent')
local HistoricalStorage = require('HistoricalStorage')
local sep = string.sub(package.config, 1, 1)

local function EventHelpers(domoticz, mainMethod)
	local _gv = globalvariables
	local globalsDefinition

	local currentPath = _gv['script_path']

	if (_G.TESTMODE) then
		-- make sure you run the tests from the tests folder !!!!
		_G.scriptsFolderPath = currentPath .. 'scripts'

		package.path =
			currentPath .. 'scripts/?.lua;' ..
			currentPath .. 'data/?.lua;' ..
			currentPath .. '/../?.lua;' ..
			package.path
	end

	local validEventTypes = 'devices,timer,security,customEvents,system,httpResponses,shellCommandResponses,scenes,groups,variables'
	local inValidEventTypes = 'on,logging,active,data,execute'

	-- defaults
	local hyperTextTransferProtocol = 'http'
	local serverPort = '8080'
	local serverAddress = '127.0.0.1'

	if _gv.domoticz_listening_port and tostring(_gv.domoticz_listening_port) ~= '' and tonumber(_gv.domoticz_listening_port) ~= 0 then
		serverPort = _gv.domoticz_listening_port
	elseif _gv.domoticz_is_secure and _gv.domoticz_secure_listening_port and
		tostring(_gv.domoticz_secure_listening_port) ~= '' and tonumber(_gv.domoticz_secure_listening_port) ~= 0 then
		hyperTextTransferProtocol = 'https'
		serverPort = _gv.domoticz_secure_listening_port
	end

	if _gv.domoticz_wwwbind ~= nil and tostring(_gv.domoticz_wwwbind) ~= '' and tostring(_gv.domoticz_wwwbind) ~= '::' then
		serverAddress = _gv.domoticz_wwwbind
	end

	local API_url = hyperTextTransferProtocol .. '://' .. serverAddress .. ':' .. serverPort

	local settings = {
		['Log level'] = _gv.dzVents_log_level or 1,
		['Domoticz url'] = API_url,
		url = API_url,
		webRoot = _gv.domoticz_webroot,
		wwwBind = _gv.domoticz_wwwbind,
		serverPort = port,
		secureServer = _gv.domoticz_is_secure,
		dzVentsVersion = _gv.dzVents_version,
		domoticzVersion = _gv.domoticz_version,
		location = {
			name = _gv.domoticz_title or 'Domoticz',
			latitude = _gv.latitude or 0,
			longitude = _gv.longitude or 0,
		}
	}

	if settings.webRoot ~= nil and settings.webRoot ~= '' then
		settings['Domoticz url'] = settings['Domoticz url'] .. '/' .. settings.webRoot
		settings.url = settings['Domoticz url']
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
	}

	if (_G.TESTMODE) then
		self.scriptsFolderPath = scriptsFolderPath
		self.dataFolderPath = _G.dataFolderPath
		function self._getUtilsInstance()
			return utils
		end
	end

	function self.getStorageContext(storageDef, module)

		local storageContext = {}
		local fileStorage, value, ok
		--require('lodash').print(1, storageDef)
		if (storageDef ~= nil) then
			-- load the datafile for this module
			ok, fileStorage = pcall(require, module)
			package.loaded[module] = nil -- no caching
			if (ok) then
				-- only transfer data as defined in storageDef
				for _var, _def in pairs(storageDef) do
					local var, def

					if (type(_var) == 'number' and type(_def) == 'string') then
						var = _def
						def = {}
					else
						var = _var
						def = _def
					end

					if type(fileStorage) == 'boolean' then

						local function preserve(fullQualifiedName, fullQualifiedNameFaulty)
							local inf = io.open(fullQualifiedName, 'rb')
							local outf = io.open(fullQualifiedNameFaulty, 'w')
							outf:write(inf:read('*a'))
							inf:close()
							outf:close()
							os.remove(fullQualifiedName)
						end

						local fullQualifiedName = _G.dataFolderPath .. sep .. module .. '.lua'
						local fullQualifiedNameFaulty = _G.dataFolderPath .. sep .. module .. '.faulty'

						utils.log('There was an issue with the require of the datamodule "' .. fullQualifiedName .. '"', utils.LOG_ERROR)
						preserve(fullQualifiedName,fullQualifiedNameFaulty)
					elseif def.history ~= nil and def.history == true then
						storageContext[var] = HistoricalStorage(fileStorage[var], def.maxItems, def.maxHours, def.maxMinutes, def.getValue)
					else
						if (fileStorage[var] == nil) then
							-- obviously this is a var that was added later
							-- initialize it
							storageContext[var] = def.initial
						else
							storageContext[var] = fileStorage[var]
						end
					end
				end
			else
				for _var, _def in pairs(storageDef) do

					local var, def

					if (type(_var) == 'number' and type(_def) == 'string') then
						var = _def
						def = {}
					else
						var = _var
						def = _def
					end

					--require('lodash').print('>', var, def, '<')
					if (def.history ~= nil and def.history == true) then
						-- no initial value, just an empty history
						storageContext[var] = HistoricalStorage(fileStorage[var], def.maxItems, def.maxHours, def.maxMinutes, def.getValue)
					else
						if (def.initial ~= nil) then
							storageContext[var] = def.initial
						else
							storageContext[var] = nil
						end
					end
				end
			end

		end
		storageContext.initialize = function(varName)
			if (storageContext[varName] ~= nil and storageDef[varName].history == nil ) then
				storageContext[varName] = storageDef[varName].initial
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

				if (type(var) == 'number' and type(def) == 'string') then
					data[def] = storageContext[def]
				else
					if (def.history ~= nil and def.history == true) then
						data[var] = storageContext[var]._getForStorage()
					else
						data[var] = storageContext[var]
					end
				end
			end
			local ok, err = pcall(persistence.store, dataFilePath, data)

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
		res.scriptName = eventHandler.name
		if (eventHandler.trigger ~= nil) then
			res.trigger = eventHandler.trigger
		end
		return res
	end

	local function deprecationWarning(moduleName, key, value, quoted)
		local msg

		if quoted then
			msg = 'dzVents deprecation warning: please use "on = { [\'' .. key .. '\'] = { \'' .. tostring(value) .. '\' } }". Module: ' .. moduleName
		else
			msg = 'dzVents deprecation warning: please use "on = { [\'' .. key .. '\'] = { ' .. tostring(value) .. ' } }". Module: ' .. moduleName
		end

		utils.log(msg, utils.LOG_ERROR)
	end

	function self.callEventHandler(eventHandler, subject)

		local useStorage = false

		if (eventHandler['execute'] ~= nil) then

			-- ==================
			-- Prepare storage
			-- ==================
			if (eventHandler.data ~= nil) then
				useStorage = true
				local localStorageContext = self.getStorageContext(eventHandler.data, eventHandler.dataFileName)

				if (localStorageContext) then
					self.domoticz.data = localStorageContext
				else
					self.domoticz.data = {}
				end
			end

			if (globalsDefinition) then
				local globalStorageContext = self.getStorageContext(globalsDefinition, '__data_global_data')
				self.domoticz.globalData = globalStorageContext
			else
				self.domoticz.globalData = {}
			end

			-- ==================
			-- Run script
			-- ==================
			local ok, res, info

			local baseType = subject and subject.baseType or ''

			if (baseType == domoticz.BASETYPE_DEVICE) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_DEVICE)
				ok, res = pcall(eventHandler['execute'], self.domoticz, subject, info)

			elseif (baseType == domoticz.BASETYPE_VARIABLE) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_VARIABLE)
				ok, res = pcall(eventHandler['execute'], self.domoticz, subject, info)

			elseif (baseType == domoticz.BASETYPE_SECURITY) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_SECURITY)
				local security = Security(self.domoticz, info.trigger)
				ok, res = pcall(eventHandler['execute'], self.domoticz, security, info)

			elseif (baseType == domoticz.BASETYPE_SCENE or baseType == domoticz.BASETYPE_GROUP) then
				if (baseType == domoticz.BASETYPE_SCENE) then
					info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_SCENE)
				else
					info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_GROUP)
				end
				ok, res = pcall(eventHandler['execute'], self.domoticz, subject, info)

			elseif (baseType == domoticz.BASETYPE_SHELLCOMMAND_RESPONSE) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_SHELLCOMMANDRESPONSE)
				info.trigger = subject.callback
				local response = ShellCommandResponse(self.domoticz, subject)
				ok, res = pcall(eventHandler['execute'], self.domoticz, response, info)

			elseif (baseType == domoticz.BASETYPE_HTTP_RESPONSE) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_HTTPRESPONSE)
				info.trigger = subject.callback
				local response = HTTPResponse(self.domoticz, subject)
				ok, res = pcall(eventHandler['execute'], self.domoticz, response, info)

			elseif (baseType == domoticz.BASETYPE_SYSTEM_EVENT) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_DOMOTICZ)
				info.trigger = subject.type
				local dze = SystemEvent(self.domoticz, subject)
				ok, res = pcall(eventHandler['execute'], self.domoticz, dze, info)
			elseif (baseType == domoticz.BASETYPE_CUSTOM_EVENT) then
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_CUSTOM)
				info.trigger = subject.type
				local custom = CustomEvent(self.domoticz, subject)
				ok, res = pcall(eventHandler['execute'], self.domoticz, custom, info)
			else
				-- timer
				info = getEventInfo(eventHandler, self.domoticz.EVENT_TYPE_TIMER)
				local timer = Timer(self.domoticz, info.trigger)
				ok, res = pcall(eventHandler['execute'], self.domoticz, timer, info)
			end

			if (ok) then

				-- ==================
				-- Persist storage
				-- ==================

				if (useStorage) then
					self.writeStorageContext(eventHandler.data,
						eventHandler.dataFilePath,
						eventHandler.dataFileName,
						self.domoticz.data)
				end

				if (globalsDefinition) then
					self.writeStorageContext(globalsDefinition,
						_G.dataFolderPath .. '/__data_global_data.lua',
						_G.dataFolderPath .. '/__data_global_data',
						self.domoticz.globalData)
				end

				self.domoticz.data = nil
				self.domoticz.globalData = nil

				return res
			else
				utils.log('An error occurred when calling event handler ' .. eventHandler.name, utils.LOG_ERROR)
				utils.log(res, utils.LOG_ERROR) -- error info
			end
		else
			utils.log('No "execute" function found in event handler ' .. eventHandler, utils.LOG_ERROR)
		end

		self.domoticz.data = nil
		self.domoticz.globalData = nil
	end

	function self.scandir(directory, type)
		local pos, len
		local i, t, popen = 0, {}, io.popen
		local cmd
		local namesLookup = {}

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

				local name = string.sub(filename, 1, pos - 1)
				table.insert(t, {
					['type'] = type,
					['name'] = name
				})

				namesLookup[name] = true -- for quick lookup for filename

				--utils.log('Found module in ' .. directory .. ' folder: ' .. t[#t].name, utils.LOG_DEBUG)
			end
		end
		pfile:close()
		return t, namesLookup
	end

	function self.processTimeRuleFunction(fn)

		local ok, res = pcall(fn, self.domoticz)

		if (not ok) then
			utils.log('Error executing custom timer function.', utils.LOG_ERROR)
			utils.log(res, utils.LOG_ERROR)
			if (_G.TESTMODE) then
				print(res)
			end
			return false
		end
		return res
	end

	function self.handleEvents(events, subject)

		local originalLogLevel = _G.logLevel -- a script can override the level

		local function restoreLogging()
			_G.logLevel = originalLogLevel
			_G.logMarker = nil
		end

		if (type(events) ~= 'table') then
			restoreLogging()
			return
		end

		for eventIdx, eventHandler in pairs(events) do

			if (eventHandler.logging) then
				if type(eventHandler.logging) ~= 'table' then
					if type(eventHandler.logging) == 'number' then
						local level = eventHandler.logging
						eventHandler.logging = {}
						eventHandler.logging.level = level
					end
					utils.log(eventHandler.type .. ' script ' .. eventHandler.name .. '.lua has a malformed logging section. Check the documentation.', utils.LOG_FORCE)
				end

				if (eventHandler.logging.level ~= nil) then
					_G.logLevel = eventHandler.logging.level
				end
				if (eventHandler.logging.marker ~= nil) then
					_G.logMarker = eventHandler.logging.marker
				end
			end

			local moduleLabel
			local moduleLabelInfo = ''
			local triggerInfo
			local scriptType = eventHandler.type == 'external' and 'external script: ' or 'internal script: '

			if (eventHandler.type == 'external') then
				moduleLabel = eventHandler.name .. '.lua'
			else
				moduleLabel = eventHandler.name .. ''
			end

			domoticz.moduleLabel = eventHandler.name:gsub('%.lua','') -- used in dynamic logmarker

			local baseType = subject and subject.baseType or ''

			if (baseType == domoticz.BASETYPE_DEVICE) then
				moduleLabelInfo = ' Device: "' .. subject.name .. ' (' .. subject.hardwareName .. ')", Index: ' .. tostring(subject.id)
			elseif (baseType == domoticz.BASETYPE_VARIABLE) then
				moduleLabelInfo = ' Variable: "' .. subject.name .. '" Index: ' .. tostring(subject.id)
			elseif (baseType == domoticz.BASETYPE_SECURITY) then
				moduleLabelInfo = ' Security: "' .. subject.name .. '"'
			elseif (baseType == domoticz.BASETYPE_SCENE or baseType == domoticz.BASETYPE_GROUP) then
				moduleLabelInfo = (subject.baseType == 'scene' and ' Scene' or ' Group') .. ': "' .. subject.name .. '", Index: ' .. tostring(subject.id)
			elseif (baseType == domoticz.BASETYPE_HTTP_RESPONSE) then
				moduleLabelInfo = ' HTTPResponse: "' .. subject.callback .. '"'
			elseif (baseType == domoticz.BASETYPE_SHELLCOMMAND_RESPONSE) then
				moduleLabelInfo = ' ShellCommandResponse: "' .. subject.callback .. '"'
			elseif (baseType == domoticz.BASETYPE_SYSTEM_EVENT) then
				moduleLabelInfo = ' Domoticz event: "' .. subject.name .. '"'
			elseif (baseType == domoticz.BASETYPE_CUSTOM_EVENT) then
				moduleLabelInfo = ' Custom event: "' .. subject.name .. '"'
			end

			triggerInfo = eventHandler.trigger and ( ', trigger: "' .. eventHandler.trigger .. '"' ) or ''

			local clockTimeStampAtStart = os.clock()
			local timeStampAtStart = os.time()

			utils.log('------ Start ' .. scriptType .. moduleLabel ..':' .. moduleLabelInfo .. triggerInfo, utils.LOG_MODULE_EXEC_INFO)
			self.callEventHandler(eventHandler, subject)

			local clockTimeSpend = os.clock() - clockTimeStampAtStart
			local realTimeSpend = os.time() - timeStampAtStart

			if realTimeSpend > 9 or clockTimeSpend > 7 then
				utils.log('------ Finished ' .. moduleLabel .. ' after >' .. realTimeSpend .. ' seconds. (using '.. tostring(clockTimeSpend):sub(1,5) .. ' seconds CPU time !)' , utils.LOG_ERROR)
			elseif realTimeSpend > 6 or clockTimeSpend > 5 then
				utils.log('------ Finished ' .. moduleLabel .. ' after >' .. realTimeSpend .. ' seconds. (using '.. tostring(clockTimeSpend):sub(1,5) .. ' seconds CPU time !)' , utils.LOG_FORCE)
			else
				utils.log('------ Finished ' .. moduleLabel , utils.LOG_MODULE_EXEC_INFO)
			end

			if (tonumber(_gv['dzVents_log_level']) == utils.LOG_DEBUG or TESTMODE ) then
				local moduleSummary = _gv.script_path  .. 'module.log'
				utils.log('Debug: Writing module summary to ' .. moduleSummary ,utils.LOG_FORCE)

				local f = io.open(moduleSummary, 'a' )
				f:write(
					os.date('%x %X - ',timeStampAtStart) .. os.date('%x %X ') .. '(' ..
					string.format('%02d', realTimeSpend) .. ' - ' .. string.format('%.4f',clockTimeSpend) ..
					') ' .. string.format('%35s',moduleLabel) .. ' <<' .. moduleLabelInfo ..
					( eventHandler.trigger and ( ' timer: "' .. eventHandler.trigger .. '"' ) or '') ,'\n')
				f:close()
			end

			restoreLogging()
		end
	end

	function self.processTimeRules(timeRules, testTime)
		-- accepts a table of timeDefs, if one of them matches with the
		-- current time, then it returns true
		-- otherwise it returns false

		local now
		if (testTime == nil) then
			now = self.domoticz.time
		else
			now = testTime
		end

		for i, _rule in pairs(timeRules) do

			if (type(_rule) == 'function') then
				return self.processTimeRuleFunction(_rule), 'function'
			end

			local rule = string.lower(tostring(_rule))
			if (now.matchesRule(rule)) then
				return true, _rule
			end
		end

		return false
	end

	function self.checkSecurity(securityDefs, security)

		for i, def in pairs(securityDefs) do
			if (def == security) then
				return true, def
			end
		end

		return false
	end

	local function addBindingEvent(bindings, event, module)
		if (bindings[event] == nil) then
			bindings[event] = {}
		end
		table.insert(bindings[event], module)
	end

	local function loadEventScripts()
		local scripts = {}
		local errModules = {}
		local internalScripts
		local ok, diskScripts, externalNames, moduleName, i, event, j, err
		local modules = {}
		local scripts = {}

		ok, diskScripts, externalNames = pcall(self.scandir, _G.scriptsFolderPath, 'external')

		if (not ok) then
			utils.log(diskScripts, utils.LOG_ERROR)
		else
			modules = diskScripts
		end

		ok = true

		ok, internalScripts = pcall(self.scandir, _G.generatedScriptsFolderPath, 'internal')

		if (not ok) then
			utils.log(internalScripts, utils.LOG_ERROR)
		else
			for i, internal in pairs(internalScripts) do
				if (externalNames[internal.name]) then
					-- oops already there, skipping
					utils.log('There is already an external script with the name "' .. internal.name .. '.lua". Please rename in the internal script.', utils.LOG_ERROR)
				else
					table.insert(modules, internal)
				end
			end
		end

		-- extract global_data module and make sure it is the first in the list
		-- because then peeps can create some globals available everywhere before the other modules load
		local globalIx
		for i, moduleInfo in pairs(modules) do
			if (moduleInfo.name == GLOBAL_DATA_MODULE) then
				globalIx = i
				break
			end
		end

		if (globalIx ~= nil and globalIx > 1) then
			local globalModule = modules[globalIx]
			table.remove(modules, globalIx)
			table.insert(modules, 1, globalModule)
		end

		for i, moduleInfo in ipairs(modules) do

			local module, skip

			local moduleName = moduleInfo.name
			local logScript = (moduleInfo.type == 'external' and 'Script ' or 'Internal script ')

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

			ok, module = pcall(require, moduleName)

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
								module.type = moduleInfo.type
								module.dataFileName = '__data_' .. moduleName
								module.dataFilePath = _G.dataFolderPath .. '/__data_' .. moduleName .. '.lua'

								table.insert(scripts, module)
							else
								utils.log(logScript .. moduleName .. '.lua has no "on" and/or "execute" section, not a dzVents module. Skipping', utils.LOG_DEBUG)
								--table.insert(errModules, moduleName)
							end
						end
					else
						utils.log(logScript .. moduleName .. '.lua is not a dzVents module. Skipping', utils.LOG_DEBUG)
						--table.insert(errModules, moduleName)
					end
				end
			else
				table.insert(errModules, moduleName)
				utils.log(module, utils.LOG_ERROR)
			end
		end

		return scripts, errModules
	end

	function self.getEventBindings(mode, testTime)
		local bindings = {}
		local ok, i, event, j, device, err
		local modules = {}

		if not self.scripts then
			self.scripts, self.errModules = loadEventScripts()
		end

		if (mode == nil) then mode = 'device' end

		for i, module in pairs(self.scripts) do

			local logScript = (module.type == 'external' and 'Script ' or 'Internal script ')

			for j, event in pairs(module.on) do
				if type(j) ~= 'string' or type(event) ~= 'table' or validEventTypes:find(j) == nil then
					if not self.scripts[i].invalidOnSection then
						self.scripts[i].invalidOnSection = true
						if type(j) == "string" and validEventTypes:find(j) == nil then
							if inValidEventTypes:find(j) then
								utils.log('You entered "' .. tostring(j) .. '" in the on = section. Probably you misplaced your curly brackets', utils.LOG_FORCE)
							else
								utils.log('Valid eventTypes are: ' .. validEventTypes, utils.LOG_DEBUG )
								utils.log('You entered "' .. tostring(j) .. '" in the on = section. Maybe you meant "' .. utils.fuzzyLookup(j, utils.stringSplit(validEventTypes,',')) ..'"?', utils.LOG_FORCE)
							end

						else
							utils.log('You entered "' .. utils.toStr(event) .. '" as trigger but the eventType is not (properly) set')
						end
						utils.log(logScript .. module.name .. '.lua has an invalid on = section;  please check the wiki. Skipping it until fixed.', utils.LOG_ERROR)
					end

				else
					if (mode == 'timer' and j == 'timer') then
						-- { ['timer'] = { 'every minute ', 'every hour' } }

						if type(event) ~= 'table' then
							utils.log(logScript .. module.name .. '.lua has a malformed timer = section. Check the documentation.', utils.LOG_FORCE)
							event = { event }
						end

						local triggered, def = self.processTimeRules(event)
						if (triggered) then
							-- this one can be executed
							module.trigger = def
							event.type = 'timer'
							table.insert(bindings, module)
						end
					elseif (mode == 'device' and j == 'devices') then

						-- { ['devices'] = { 'devA', ['devB'] = { ..timedefs }, .. }

						for devIdx, devName in pairs(event) do

							-- detect if devName is of the form ['devB'] = { 'every hour' }
							if (type(devName) == 'table') then
								local triggered, def = self.processTimeRules(devName, testTime)
								if (triggered) then
									addBindingEvent(bindings, devIdx, module)
								end
							else
								-- a single device name (or id)
								addBindingEvent(bindings, devName, module)
							end
						end
					elseif (mode == 'scenegroups' and (j == 'scenes' or j == 'groups')) then

						-- { ['scenes'] = { 'scA', ['scB'] = { ..timedefs }, .. }

						for scgrpIdx, scgrpName in pairs(event) do

							-- detect if scgrpName is of the form ['devB'] = { 'every hour' }
							if (type(scgrpName) == 'table') then
								local triggered, def = self.processTimeRules(scgrpName, testTime)
								if (triggered) then
									addBindingEvent(bindings, scgrpIdx, module)
								end
							else
								-- a single scene or group name (or id)
								addBindingEvent(bindings, scgrpName, module)
							end
						end
					elseif (mode == 'variable' and j == 'variables') then
						-- { ['variables'] = { 'varA', 'varB' }
						for varIdx, varName in pairs(event) do
							addBindingEvent(bindings, varName, module)
						end
					elseif (mode == 'security' and j == 'security') then
						local triggered, def = self.checkSecurity(event, self.domoticz.security)
						if (triggered) then
							table.insert(bindings, module)
							module.trigger = def
						end
					elseif (mode == 'httpResponse' and j == 'httpResponses') then
						-- { ['httpResponses'] = { 'callbackA', 'callbackB' }
						for i, callbackName in pairs(event) do
							addBindingEvent(bindings, callbackName, module)
						end
					elseif (mode == 'shellcommandResponse' and j == 'shellCommandResponses') then
						-- { ['shellCommandResponses'] = { 'callbackA', 'callbackB' }
						for i, callbackName in pairs(event) do
							addBindingEvent(bindings, callbackName, module)
						end
					elseif (mode == 'system' and j == 'system') then
						-- { ['system'] = { 'start', 'end' }
						for evtIdx, systemEvent in pairs(event) do
							-- detect if system is of the form ['start'] = { 'every hour' }
							if (type(systemEvent) == 'table') then
								local triggered, def = self.processTimeRules(systemEvent, testTime)
								if (triggered) then
									addBindingEvent(bindings, evtIdx, module)
								end
							else
								-- a single system event
								addBindingEvent(bindings, systemEvent, module)
							end
						end
					elseif (mode == 'customEvents' and j == 'customEvents') then
						-- { customEvents = { 'myEvent', ['anotherEvent'] = { ... timeDefs ... } }

						for evtIdx, customEventName in pairs(event) do
							-- detect if customEvent is of the form ['myEvent'] = { 'every hour' }
							if (type(customEventName) == 'table') then
								local triggered, def = self.processTimeRules(customEventName, testTime)
								if (triggered) then
									addBindingEvent(bindings, evtIdx, module)
								end
							else
								-- a single custom event
								addBindingEvent(bindings, customEventName, module)
							end
						end
					end
				end
			end
		end
		return bindings, self.errModules
	end

	function self.dumpCommandArray(commandArray, fromIndex, force)
		local printed = false
		local level = utils.LOG_DEBUG

		if (fromIndex == nil) then
			fromIndex = 1
		end

		if (force == true and force ~= nil or _gv['testmode'] == true) then
			level = utils.LOG_INFO
		end

		for k, v in pairs(commandArray) do
			if ((fromIndex ~= nil and k >= fromIndex) or fromIndex == nil) then
				if (not printed) then
					utils.log('Commands sent to Domoticz: ', level)
				end
				if (type(v) == 'table') then
					for kk, vv in pairs(v) do
						utils.log('- ' .. kk .. ' = ' .. _.str(vv), level)
					end
				else
					utils.log(k .. ' = ' .. v, level)
				end
				printed = true
			end
		end
		if (printed) then utils.log('=====================================================', level) end
	end

	function self.findScriptForTarget(target, allEventScripts)
		-- event could be like: myPIRLivingRoom
		-- or myPir(.*)

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

			local function strFind(str, key)
				return string.find(str, key)
			end

			local ok, res = pcall(strFind, scriptTrigger, '*');
			if not ok then
				utils.log('Script name: ' .. scripts[1].name  .. '.lua, has a malformed on = section. The trigger = ' .. _.str(scriptTrigger) , utils.LOG_ERROR)
				allEventScripts[scriptTrigger] = ''
			elseif res then -- a wild-card was used
				scriptTrigger = ('^' .. scriptTrigger:gsub("[%^$]","."):gsub("*", ".*") .. '$')

				local function sMatch(text, match) -- specialized sanitized match function to allow combination of Lua magic chars in wildcards
					local sanitizedMatch = match:gsub("([%%%(%)%[%]%+%-%?])", "%%%1") -- escaping all 'magic' chars except *, ., ^ and $
					return text:match(sanitizedMatch)
				end

				if sMatch(target, scriptTrigger) then
					if modules == nil then modules = {} end

					for i, mod in pairs(scripts) do
						table.insert(modules, mod)
					end

				end

			else
				if (scriptTrigger == target) then
					-- there is trigger for this target

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

		domoticz.changedDevices().forEach( function(device)

			local scriptsToExecute = self.findScriptForTarget(device.name, allEventScripts)
			local idScripts = allEventScripts[device.id]

			local caSize = _.size(self.domoticz.commandArray)

			if (idScripts ~= nil) then
				-- merge id scripts with name scripts
				if (scriptsToExecute == nil) then
					scriptsToExecute = {}
				end
				for i, mod in pairs(idScripts) do
					table.insert(scriptsToExecute, mod)
				end
			end

			if (scriptsToExecute ~= nil) then
				utils.log('Handling events for: "' .. device.name .. '", value: "' .. tostring(device.state) .. '"', utils.LOG_MODULE_EXEC_INFO)
				self.handleEvents(scriptsToExecute, device)
				self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
			end

		end)

		return self.domoticz.commandArray
	end

	function self.dispatchSceneGroupEventsToScripts(domoticz)

		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local allEventScripts = self.getEventBindings('scenegroups')

		local processItem = function(item, loglabel)
			utils.log(loglabel .. '-event for: ' .. item.name .. ' value: ' .. tostring(item.state), utils.LOG_DEBUG)

			local scriptsToExecute = self.findScriptForTarget(item.name, allEventScripts)
			local idScripts = allEventScripts[item.id]
			local caSize = _.size(self.domoticz.commandArray)

			if (idScripts ~= nil) then
				-- merge id scripts with name scripts
				if (scriptsToExecute == nil) then
					scriptsToExecute = {}
				end
				for i, mod in pairs(idScripts) do
					table.insert(scriptsToExecute, mod)
				end
			end

			if (scriptsToExecute ~= nil) then
				utils.log('Handling events for: "' .. item.name .. '", value: "' .. tostring(item.state) .. '"', utils.LOG_MODULE_EXEC_INFO)
				self.handleEvents(scriptsToExecute, item)
				self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
			end

		end

		domoticz.changedScenes().forEach(function(scene)
			processItem(scene, 'Scene')
		end)

		domoticz.changedGroups().forEach(function(group)
			processItem(group, 'Group')
		end)

		return self.domoticz.commandArray
	end

	function self.dispatchTimerEventsToScripts()
		local scriptsToExecute = self.getEventBindings('timer')

		self.handleEvents(scriptsToExecute)
		self.dumpCommandArray(self.domoticz.commandArray)

		return self.domoticz.commandArray
	end

	function self.dispatchSecurityEventsToScripts()

		local updates =_G.securityupdates

		if (updates ~= nil) then

			for i, securityState in pairs(updates) do

				local security = {
					baseType = domoticz.BASETYPE_SECURITY,
					name = securityState
				}

				local caSize = _.size(self.domoticz.commandArray)

				self.domoticz.security = securityState

				local scriptsToExecute = self.getEventBindings('security', nil)

				self.handleEvents(scriptsToExecute, security)

				self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
			end
		end

		return self.domoticz.commandArray
	end

	function self.dispatchVariableEventsToScripts(domoticz)
		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local allEventScripts = self.getEventBindings('variable')

		domoticz.changedVariables().forEach(function(variable)

			local caSize = _.size(self.domoticz.commandArray)

			-- first search by name

			local scriptsToExecute = self.findScriptForTarget(variable.name, allEventScripts)
			if (scriptsToExecute == nil) then
				-- search by id
				scriptsToExecute = allEventScripts[variable.id]
			end

			if (scriptsToExecute ~= nil) then
				utils.log('Handling variable-events for: "' .. variable.name .. '", value: "' .. tostring(variable.value) .. '"', utils.LOG_MODULE_EXEC_INFO)
				self.handleEvents(scriptsToExecute, variable)
				self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
			end
		end)

		return self.domoticz.commandArray
	end

	function self.dispatchShellCommandResponseEventsToScripts(domoticz)
		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local shellcommandResponseScripts = self.getEventBindings('shellcommandResponse')

		local responses =_G.shellcommandresponse

		if (responses ~= nil) then
			for i, response in pairs(responses) do
				response.baseType = domoticz.BASETYPE_SHELLCOMMAND_RESPONSE
				local callback = response.callback
				local caSize = _.size(self.domoticz.commandArray)

				local scriptsToExecute = self.findScriptForTarget(callback, shellcommandResponseScripts)

				if (scriptsToExecute ~= nil) then
					utils.log('Handling shellcommandResponse-events for: "' .. callback .. '"', utils.LOG_MODULE_EXEC_INFO)
					self.handleEvents(scriptsToExecute, response)
					self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
				end

			end

		end

		return self.domoticz.commandArray

	end

	function self.dispatchHTTPResponseEventsToScripts(domoticz)
		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local httpResponseScripts = self.getEventBindings('httpResponse')

		local responses =_G.httpresponse

		if (responses ~= nil) then
			for i, response in pairs(responses) do

				response.baseType = domoticz.BASETYPE_HTTP_RESPONSE
				local callback = response.callback
				local caSize = _.size(self.domoticz.commandArray)

				local scriptsToExecute = self.findScriptForTarget(callback, httpResponseScripts)

				if (scriptsToExecute ~= nil) then
					utils.log('Handling httpResponse-events for: "' .. callback .. '"', utils.LOG_MODULE_EXEC_INFO)
					self.handleEvents(scriptsToExecute, response)
					self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
				end

			end

		end

		return self.domoticz.commandArray

	end

	function self.dispatchSystemEventsToScripts(domoticz)
		if (domoticz == nil) then
			-- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		if (_G.notification == nil) then
			return
		end

		local systemEventScripts = self.getEventBindings('system', nil)
		local systemEvents =_G.notification.domoticz

		if (systemEvents ~= nil) then
			for i, event in pairs(systemEvents) do

				event.name = SystemEvent(self.domoticz, event)["type"]
				if event.name == nil then
					utils.log('System event "' .. event.type .. '" is not supported yet.', utils.LOG_ERROR)
				else
					event.baseType = domoticz.BASETYPE_SYSTEM_EVENT

					local caSize = _.size(self.domoticz.commandArray)
					local scriptsToExecute = self.findScriptForTarget(event.name, systemEventScripts)

					if (scriptsToExecute ~= nil) then
						utils.log('Handling system event for: "' .. event.name .. '"', utils.LOG_MODULE_EXEC_INFO)
						self.handleEvents(scriptsToExecute, event)
						self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
					end
				end
			end
		end

		return self.domoticz.commandArray
	end

	function self.dispatchCustomEventsToScripts(domoticz)
		if (domoticz == nil) then
			-- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		if (_G.notification == nil or _G.notification.customevent == nil ) then
			return
		end

		for _, row in ipairs(_G.notification.customevent) do
			 if row.data.name:match('^___%a*__$') then
				table.insert(self.domoticz.commandArray, { [ row.data.name:sub(4,-3) ] = row.data.data })
			 end
		end

		local customEventScripts = self.getEventBindings('customEvents', nil)
		local customEvents = _G.notification.customevent

		if (customEvents ~= nil) then
			for i, customEvent in pairs(customEvents) do

				customEvent.name = customEvent.data.name
				customEvent.baseType = domoticz.BASETYPE_CUSTOM_EVENT

				local caSize = _.size(self.domoticz.commandArray)

				local scriptsToExecute = self.findScriptForTarget(customEvent.name, customEventScripts)

				if (scriptsToExecute ~= nil) then
					utils.log('Handling Domoticz custom event for: "' .. customEvent.name .. '"', utils.LOG_MODULE_EXEC_INFO)
					self.handleEvents(scriptsToExecute, customEvent)
					self.dumpCommandArray(self.domoticz.commandArray, caSize + 1)
				end
			end
		end

		return self.domoticz.commandArray
	end

	function self.getEventSummary(domoticz)
		if (domoticz == nil) then -- you can pass a domoticz object for testing purposes
			domoticz = self.domoticz
		end

		local items = {}
		local length = 0

		domoticz.changedDevices().forEach( function(device)
			table.insert(items, '- Device: ' .. device.name)
			length = length + 1
		end)

		domoticz.changedVariables().forEach(function(variable)
			table.insert(items, '- Variable: ' .. variable.name)
			length = length + 1
		end)

		local securityUpdates =_G.securityupdates
		if (securityUpdates ~= nil) then
			for i, securityState in pairs(securityUpdates) do
				table.insert(items, '- Security: ' .. securityState)
				length = length + 1
			end
		end

		domoticz.changedScenes().forEach(function(scene)
			table.insert(items, '- Scene: ' .. scene.name)
			length = length + 1
		end)

		domoticz.changedGroups().forEach(function(group)
			table.insert(items, '- Group: ' .. group.name)
			length = length + 1
		end)

		local responses =_G.httpresponse

		if (responses ~= nil) then
			for i, response in pairs(responses) do
				table.insert(items, '- HTTPResponse: ' .. response.callback)
				length = length + 1
			end
		end

		local domoticzEvents = _G.notification and _G.notification.domoticz or nil

		if (domoticzEvents ~= nil) then
			for i, domoticzEvent in pairs(domoticzEvents) do
				table.insert(items, '- Domoticz: ' .. domoticzEvent.type) -- .. ' - ' .. domoticzEvent.status)
				length = length + 1
			end
		end

		local customEvents = _G.notification and _G.notification.customEvents or nil

		if (customEvents ~= nil) then
			for i, customEvent in pairs(customEvents) do
				table.insert(items, '- Custom: ' .. customEvent.type) -- .. ' - ' .. customEvent.status)
				length = length + 1
			end
		end

		return items, length;

	end

	return self
end

return EventHelpers
