local jsonParser = require('JSON')
local _ = require('lodash')

local self = {
	LOG_ERROR = 1,
	LOG_FORCE = 0.5,
	LOG_MODULE_EXEC_INFO = 2,
	LOG_INFO = 3,
	LOG_WARNING = 3,
	LOG_DEBUG = 4,
	DZVERSION = '3.1.5',
}

function jsonParser:unsupportedTypeEncoder(value_of_unsupported_type)
	if type(value_of_unsupported_type) == 'function' then
		return '"Function"'
	else
		return nil
	end
end

function math.pow(x, y)
	self.log('Function math.pow(x, y) has been deprecated in Lua 5.3. Please consider changing code to x^y', self.LOG_FORCE)
	return x^y
end

-- Cast anything but functions to string
self.toStr = function (value)
	local dblQuote = function (v)
		return '"'..v..'"'
	end

	local str = ''
	if _.isString(value) then
		str = value
	elseif _.isBoolean(value) then
		str = value and 'true' or 'false'
	elseif _.isNil(value) then
		str = 'nil'
	elseif _.isNumber(value) then
		str = value .. ''
	elseif _.isFunction(value) then
		str = 'function'
	elseif _.isTable(value) then
		str = '{'
		for k, v in pairs(value) do
			v = ( _.isString(v) and dblQuote(v) ) or self.toStr(v)
			if _.isNumber(k) then
				str = str .. v .. ', '
			else
				str = str .. '[' .. dblQuote(k) .. ']=' .. v .. ', '
			end
		end
		str = str:sub(0, ( #str - 2 ) ) .. '}'
	end
	return str
end

self.fuzzyLookup = function (search, target) -- search must be string/number, target must be string/number or array of string/numbers
	if type(target) == 'table' then
		local lowestFuzzyValue = math.maxinteger
		local lowestFuzzyKey = ''
		for _, targetString in ipairs(target) do
			local fuzzyValue =  self.fuzzyLookup(search, targetString)
			if  fuzzyValue < lowestFuzzyValue then
				lowestFuzzyKey = targetString
				lowestFuzzyValue = fuzzyValue
			end
		end
		return lowestFuzzyKey
	else
		local search, target = (tostring(search)):lower(), (tostring(target)):lower()
		local searchLength, targetLength, res = #search, #target, {}
		for i = 0, searchLength do res[i] = { [0] = i } end
		for j = 1, targetLength do res[0][j] = j end
		for k = 1, searchLength do
			for l = 1, targetLength do
				local cost = search:sub(k,k) == target:sub(l,l) and 0 or 1
				res[k][l] = math.min(res[k-1][l]+1, res[k][l-1]+1, res[k-1][l-1]+cost)
			end
		end
		return res[searchLength][targetLength]
	end
end

function self.setLogMarker(logMarker)
	_G.logMarker = logMarker or _G.moduleLabel
end

function self.rightPad(str, len, char)
	if char == nil then char = ' ' end
	return str .. string.rep(char, len - #str)
end

function self.leftPad(str, len, char)
	if char == nil then char = ' ' end
	return string.rep(char, len - #str) .. str
end

function self.centerPad(str, len, char )
	if char == nil then char = ' ' end
	return string.rep(char, ( len - #str) / 2 ) .. str .. string.rep(char, ( len - #str) /2)
end

function self.leadingZeros(num, len )
	return self.leftPad(tostring(num),len,'0')
end

function self.numDecimals(num, int, dec)
	if int == nil then int = 99 end
	if dec == nil then dec = 0 end
	local fmt = '%' .. int .. '.' .. dec .. 'f'
	return string.format(fmt,num)
end

function self.fileExists(name)
	local ok, err, code = os.rename(name, name)
	return code ~= 2
end

function self.stringSplit(text, sep, convertNumber, convertNil)
	if not(text) then return {} end
	local sep = sep or '%s'
	local include = '+'
	if convertNil then include = '*' end
	local t = {}
	for str in string.gmatch(text, "([^" ..sep.. "]" .. include .. ")" ) do
		if convertNil and str == '' then str = convertNil end
		table.insert(t, ( convertNumber and tonumber(str) ) or str)
	end
	return t
end

function self.stringToSeconds(str)

	local now = os.date('*t')
	local daySeconds = 24 * 3600
	local weekSeconds = 7 * daySeconds
	local num2Days = { 'sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat' }
	local days2Num = { sun = 1, mon = 2, tue = 3, wed = 4, thu = 5, fri = 6, sat = 7 }

	local function calcDelta(str)
		local function timeDelta(str)
			local hours, minutes, seconds = 0, 0, 0
			if str:match('%d+:%d%d:%d%d') then
				hours, minutes, seconds = str:match("(%d+):(%d%d):(%d%d)")
			else
				hours, minutes = str:match("(%d+):(%d%d)")
			end
			return ( hours * 3600 + minutes * 60 + seconds - ( now.hour * 3600 + now.min * 60 + now.sec ))
		end

		local delta
		local deltaT = timeDelta(str)
		if str:match(' on ') then
			for _, day in ipairs(num2Days) do
				if str:lower():find(day) then
					local newDelta = ( days2Num[day] - now.wday + 7 ) % 7 * daySeconds + deltaT
					if newDelta < 0 then newDelta = newDelta + weekSeconds end
					if delta == nil or newDelta < delta then delta = newDelta end
				end
			end
		else
			if deltaT < 0 then deltaT = deltaT + daySeconds end
		end

		if delta == nil and deltaT < 0 then deltaT = deltaT + weekSeconds end
		return delta or deltaT
	end

	return math.tointeger(calcDelta(str))
end

function self.inTable(searchTable, element)
	if type(searchTable) ~= 'table' then return false end
	local res = res
	for k, v in pairs(searchTable) do
		if type(v) == 'table' then res = self.inTable(v, element) end
		res = res or (( tostring(k) == tostring(element) and 'key' ) or ( tostring(v) == tostring(element) and 'value' ))
		if res then return res end
	end
	return false
end

function self.round(value, decimals)
	local nVal = tonumber(value)
	local nDec = ( decimals == nil and 0 ) or tonumber(decimals)
	if nVal >= 0 and nDec > 0 then
		return math.floor( (nVal * 10 ^ nDec) + 0.5) / (10 ^ nDec)
	elseif nVal >=0 then
		return math.floor(nVal + 0.5)
	elseif nDec and nDec > 0 then
		return math.ceil ( (nVal * 10 ^ nDec) - 0.5) / (10 ^ nDec)
	else
		return math.ceil(nVal - 0.5)
	end
end

function string.sMatch(text, match) -- add sanitized match function to string "library"
	local sanitizedMatch = match:gsub("([%%%^%$%(%)%.%[%]%*%+%-%?])", "%%%1") -- escaping all 'magic' chars
	return text:match(sanitizedMatch)
end

function self.toCelsius(f, relative)
	if (relative) then
		return f*(1/1.8)
	end
	return ((f-32) / 1.8)
end

function self.osCommand(cmd)
	local file = assert ( io.popen(cmd) )
	local output = assert ( file:read('*all') )
	local rc = { file:close() }
	return output, rc[3]
end

function self.osExecute(cmd)
	if (_G.TESTMODE) then return end
	os.execute(cmd)
end

function self.print(msg, filename)
	if (_G.TESTMODE) then return end
	if filename == nil then print(msg) return end

	local targetDirectory = _G.dataFolderPath .. '/../dumps/'
	if not( self.fileExists(targetDirectory)) then
		os.execute( 'mkdir ' .. targetDirectory )
	end

	local f = io.open(_G.dataFolderPath .. '/../dumps/' .. filename, 'a' )
	f:write(msg,'\n')
	f:close()
end

function self.urlEncode(str, strSub)

	if (strSub == nil) then
		strSub = "+"
	else
		strSub = "%" .. strSub
	end

	if (str) then
		str = string.gsub(str, "\n", "\r\n")
		str = string.gsub(str, "([^%w %-%_%.%~])",
			function(c) return string.format("%%%02X", string.byte(c)) end)
		str = string.gsub(str, " ", strSub)
	end
	return str
end

function self.urlDecode(str, strSub)

	local hex2Char = function(x)
		return string.char(tonumber(x, 16))
	end

	return str:gsub("%%(%x%x)", hex2Char)
end

function self.isJSON(str, content)

	local str = str or ''
	local content = content or ''
	local jsonPatternOK = '^%s*%[*%s*{.+}%s*%]*%s*$'
	local jsonPatternOK2 = '^%s*%[.+%]*%s*$'
	local ret = ( str:match(jsonPatternOK) == str ) or ( str:match(jsonPatternOK2) == str ) or content:find('application/json')
	return ret ~= nil
end

function self.fromJSON(json, fallback)

	if not(json) then
		return fallback
	end

	if json:find("'") then
		local _, singleQuotes = json:gsub("'","'")
		local _, doubleQuotes = json:gsub('"','"')
		if singleQuotes > doubleQuotes then
			json = json:gsub("'",'"')
		end
	end

	if self.isJSON(json) then

		local parse = function(j)
			return jsonParser:decode(j)
		end

		ok, results = pcall(parse, json)

		if (ok) then
			return results
		end
		self.log('Error parsing json to LUA table: ' .. _.str(results) , self.LOG_ERROR)
	else
		self.log('Error parsing json to LUA table: (invalid json string) ' .. _.str(json) , self.LOG_ERROR)
	end

	return fallback

end

function self.fromBase64(codedString) -- from http://lua-users.org/wiki/BaseSixtyFour
	if type(codedString) ~= 'string' then
		self.log('fromBase64: parm should be a string; you supplied a ' .. type(codedString), self.LOG_ERROR)
		return nil
	end
	local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
	local data = string.gsub(codedString, '[^'.. b ..'=]', '')
	return (data:gsub('.', function(x)
		if (x == '=') then return '' end
		local r, f = '',(b:find(x)-1)
		for i = 6, 1, -1 do r = r .. (f%2^i-f%2^(i-1)>0 and '1' or '0') end
		return r;
	end):gsub('%d%d%d?%d?%d?%d?%d?%d?', function(x)
		if (#x ~= 8) then return '' end
		local c = 0
		for i = 1, 8 do c = c + (x:sub(i, i) == '1' and 2^(8-i) or 0) end
		return string.char(c)
	end))
end

function self.toBase64(s) -- from http://lua-users.org/wiki/BaseSixtyFour
	if type(s) == 'number' then s = tostring(s)
	elseif type(s) ~= 'string' then
		self.log('toBase64: parm should be a number or a string; you supplied a ' .. type(s), self.LOG_ERROR)
		return nil
	end
	local bs =
	{	[0] =
				'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
				'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
				'0','1','2','3','4','5','6','7','8','9',
				'+','/'
	}
	local byte, rep = string.byte, string.rep
	local pad = 2 - ((#s-1) % 3)
	s = (s..rep('\0', pad)):gsub("...", function(cs)
		local a, b, c = byte(cs, 1, 3)
		return bs[a>>2] .. bs[(a&3)<<4|b>>4] .. bs[(b&15)<<2|c>>6] .. bs[c&63]
	end)
	return s:sub(1, #s-pad) .. rep('=', pad)
end

function self.hasLines(str, eol)
	local eol = eol or '\n'
	return str:find(eol)
end

function self.fromLines(str, eol)
	local eol = eol or '\n'
	return self.stringSplit(str, eol)
end

function self.isXML(str, content)

	local str = str or ''
	local content = content or ''
	local xmlPattern = '^%s*%<.+%>%s*$'
	local ret = ( str:match(xmlPattern) == str or content:find('application/xml') or content:find('text/xml')) and not(str:sub(1,30):find('DOCTYPE html') )
	return ret

end

function self.fromXML(xml, fallback)

	if xml and self.isXML(xml) then
		local parseXML = function(x)
			local xmlParser = xml2Lua.parser(xmlHandler)
			xmlParser:parse(x)
			return xmlHandler.root
		end

		if xml == nil then
			return fallback
		end

		if xml2Lua == nil then
			xml2Lua = require('xml2lua')
		end

		if xmlHandler == nil then
			xmlHandler = require("xmlhandler.tree")
		end

		ok, results = pcall(parseXML, xml)

		if (ok) then
			return results
		end
		-- self.log('Error parsing xml to Lua table: ' .. _.str(results), self.LOG_ERROR)
	else
		self.log('Error parsing xml to LUA table: (invalid xml string) ' .. _.str(xml) , self.LOG_ERROR)
	end
	return fallback

end

function self.toXML(luaTable, header)

	if header == nil then header = 'LuaTable' end

	local toXML = function(luaTable, header)
		return xmlParser.toXml(luaTable, header)
	end

	if (xmlParser == nil) then
		xmlParser = require('xml2lua')
	end

	ok, results = pcall(toXML, luaTable, header)

	if (ok) then
		return results
	end

	self.log('Error converting LUA table to XML: ' .. _.str(results), self.LOG_ERROR)
	return nil

end

function self.toJSON(luaTable)

	local toJSON = function(j)
		return jsonParser:encode(j)
	end

	ok, results = pcall(toJSON, luaTable)

	if (ok) then
		return results
	end

	self.log('Error converting LUA table to json: ' .. _.str(results), self.LOG_ERROR)
	return nil

end

function self.log(msg, level)

	if (level == nil) then level = self.LOG_INFO end

	if (type(level) ~= 'number') then
		self.print('Error: log level is not a number. Got: ' .. tostring(level) .. ', type: ' .. type(level))
		return
	end

	local lLevel = _G.logLevel == nil and 1 or _G.logLevel
	local marker = ''

	if (level == self.LOG_ERROR) then
		marker = marker .. 'Error: (' .. self.DZVERSION .. ') '
	elseif (level == self.LOG_DEBUG) then
		marker = marker .. 'Debug: '
	elseif (level == self.LOG_INFO or level == self.LOG_MODULE_EXEC_INFO) then
		marker = marker .. 'Info: '
	elseif (level == self.LOG_FORCE) then
		marker = marker .. '!Info: '
	end

	if (_G.logMarker ~= nil) then
		marker = marker .. _G.logMarker .. ': '
	end

	if (level <= lLevel) then
		local maxLength = 6000 -- limit 3 * 2048 hardCoded in main/Logger.cpp
		msg = self.toStr(msg)
		for i = 1, #msg, maxLength do
			self.print( marker .. msg:sub(i, i + maxLength - 1 ) )
		end
	end
end

function self.rgbToHSB(r, g, b)
	local hsb = {h = 0, s = 0, b = 0}

	local min = math.min(r, g, b)
	local max = math.max(r, g, b)

	local delta = max - min;

	hsb.b = max;
	hsb.s = max ~= 0 and (255 * delta / max) or 0;

	if (hsb.s ~= 0) then
		if (r == max) then
			hsb.h = (g - b) / delta
		elseif (g == max) then
			hsb.h = 2 + (b - r) / delta
		else
			hsb.h = 4 + (r - g) / delta
		end
	else
		hsb.h = -1
	end

	hsb.h = hsb.h * 60
	if (hsb.h < 0) then
		hsb.h = hsb.h + 360
	end

	hsb.s = hsb.s * (100 / 255)
	hsb.b = hsb.b * (100 / 255)

	local isWhite = (hsb.s < 20)
	return hsb.h, hsb.s, hsb.b, isWhite
end

local function loopGlobal(parm, baseType)
	if parm == nil then return false end
	local res = 'id'
	local search = parm
	if type(parm) == 'table' then
		search = search.id
	elseif type(parm) == 'number' then
		res = 'name'
	end
	for i, item in ipairs(_G.domoticzData) do
		if item.baseType == baseType and ( item.id == search or item.name == search ) then return item[res] end
	end
	return false
end

function self.deviceExists(parm)
	return loopGlobal(parm, 'device')
end

function self.sceneExists(parm)
	return loopGlobal(parm, 'scene')
end

function self.groupExists(parm)
	return loopGlobal(parm, 'group')
end

function self.hardwareExists(parm)
	return loopGlobal(parm, 'hardware')
end

function self.variableExists(parm)
	return loopGlobal(parm, 'uservariable')
end

function self.cameraExists(parm)
	return loopGlobal(parm, 'camera')
end

function self.dumpTable(t, level, filename, done)
	local level = level or "> "
	local done = done or {}
	for attr, value in pairs(t or {}) do
		if type(value) ~= 'function' then
			if type(value) == 'table' and not(done[value]) then
				done[value] = true
				self.print(level .. attr .. ':', filename)
				self.dumpTable(value, level .. '	', filename, done)
			else
				self.print(level .. attr .. ': ' .. tostring(value), filename)
			end
		else
			self.print(level .. attr .. '()', filename)
		end
	end
end

function self.dumpSelection(object, selection)
	self.print ('dump ' .. selection .. ' of ' .. object.baseType .. ' ' .. object.name)
	if selection == 'attributes' then
		for attr, value in pairs(object) do
			if type(value) ~= 'function' and type(value) ~= 'table' then
				self.print('> ' .. attr .. ': ' .. tostring(value))
			end
		end
		if object.baseType ~= 'hardware' then
			self.print('')
			self.print('> lastUpdate: ' .. (object.lastUpdate.raw or '') )
		end
		if object.baseType ~= 'variable' and object.baseType ~= 'hardware' then
			self.print('> adapters: ' .. table.concat(object._adapters or {},', ') )
		end
		if object.baseType == 'device' then
			self.print('> levelNames: ' .. table.concat(object.levelNames or {},', ') )
			self.print('> rawData: ' .. table.concat(object.rawData or {},', ') )
		end
	elseif selection == 'tables' then
		for attr, value in pairs(object) do
			if type(value) == 'table' then
				self.print('> ' .. attr .. ': ' )
			end
		end
	elseif selection == 'functions' then
		for attr, value in pairs(object) do
			if type(value) == 'function' then
				self.print('> ' .. attr .. '()')
			end
		end
	end
end

function self.hsbToRGB(h, s, v)
	local r, b, g, C, V, S, X, m, r1, b1, g1

	local function inRange(value, low, high)
		return (value >= low and value <= high)
	end

	local function getRGB(C,X,h)
		if h >= 300 and h < 360 then return C, 0, X end
		if h >= 240 then return X, 0, C end
		if h >= 180 then return 0, X, C end
		if h >= 120 then return 0, C, X end
		if h >= 60 then return X, C, 0 end
		return C, X, 0
	end

	if s > 1 then S = s / 100 else S = s end
	if v > 1 then V = v / 100 else V = v end
	if not(inRange(h,0,360)) then h = h % 360 end

	C = V * S
	X = C * (1 - (h / 60 % 2) - 1)

	m = V - C

	r1, g1, b1 = getRGB(C, X, h)
	r = (r1+m) * 255
	g = (g1+m) * 255
	b = (b1+m) * 255
	return r, g, b

end

function self.humidityStatus (temperature, humidity)
	local constants = domoticz or require('constants')
	local temperature = tonumber(temperature)
	local humidity = tonumber(humidity)

	if humidity <= 30 then return constants.HUM_DRY
	elseif humidity >= 70 then return constants.HUM_WET
	elseif  humidity >= 35 and
			humidity <= 65 and
			temperature >= 22 and
			temperature <= 26 then return constants.HUM_COMFORTABLE
	else return constants.HUM_NORMAL end
end

return self
