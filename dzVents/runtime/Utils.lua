
local jsonParser
local _ = require('lodash')

local self = {
	LOG_ERROR = 1,
	LOG_FORCE = 0.5,
	LOG_MODULE_EXEC_INFO = 2,
	LOG_INFO = 3,
	LOG_DEBUG = 4,
	DZVERSION = '2.5.7',
}

function math.pow(x, y)
	self.log('Function math.pow(x, y) has been deprecated in Lua 5.3. Please consider changing code to x^y', self.LOG_FORCE)
	return x^y
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
	local f = io.open(name, "r")
	if f ~= nil then
		io.close(f)
		return true
	else
		return false
	end
end

function self.stringSplit(text, sep)
	if not(text) then return {} end
	local sep = sep or '%s'
	local t = {}
	for str in string.gmatch(text, "([^"..sep.."]+)") do
		table.insert(t, str)
	end
	return t
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

function self.round(x, n)
	-- n = math.pow(10, n or 0)
	n = 10^(n or 0)
	x = x * n
	if x >= 0 then
		x = math.floor(x + 0.5)
	else
		x = math.ceil(x - 0.5)
	end
	return x / n
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

function self.osExecute(cmd)
	if (_G.TESTMODE) then return end
	os.execute(cmd)
end

function self.print(msg)
	if (_G.TESTMODE) then return end
	print(msg)
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

function self.fromJSON(json, fallback)

	local parse = function(j)
		return jsonParser:decode(j)
	end

	if json == nil then
		return fallback
	end

	if (jsonParser == nil) then
		jsonParser = require('JSON')
	end

	ok, results = pcall(parse, json)

	if (ok) then
		return results
	end

	self.log('Error parsing json to LUA table: ' .. results, self.LOG_ERROR)
	return fallback

end

function self.fromBase64(codedString)  -- from http://lua-users.org/wiki/BaseSixtyFour
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

function self.fromXML(xml, fallback)

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

	self.log('Error parsing XML to LUA table: ' .. results, self.LOG_ERROR)
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

	self.log('Error converting LUA table to XML: ' .. results, self.LOG_ERROR)
	return nil

end

function self.toJSON(luaTable)

	local toJSON = function(j)
		return jsonParser:encode(j)
	end

	if (jsonParser == nil) then
		jsonParser = require('JSON')
	end

	ok, results = pcall(toJSON, luaTable)

	if (ok) then
		return results
	end

	self.log('Error converting LUA table to json: ' .. results, self.LOG_ERROR)
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
		self.print(tostring(marker) .. _.str(msg))
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
	local res = 'id'
	if type(parm) == 'number' then res = 'name' end
	for i, item in ipairs(_G.domoticzData) do
		if item.baseType == baseType and ( item.id == parm or item.name == parm ) then return item[res] end
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

function self.variableExists(parm)
	return loopGlobal(parm, 'uservariable')
end

function self.cameraExists(parm)
	return loopGlobal(parm, 'camera')
end

function self.dumpTable(t, level)
	local level = level or "> "
	for attr, value in pairs(t or {}) do
		if (type(value) ~= 'function') then
			if (type(value) == 'table') then
				self.print(level .. attr .. ':')
				self.dumpTable(value, level .. '	')
			else
				self.print(level .. attr .. ': ' .. tostring(value))
			end
		else
			self.print(level .. attr .. '()')
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

return self
