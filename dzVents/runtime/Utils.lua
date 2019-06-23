local jsonParser
local _ = require('lodash')

local self = {
	LOG_ERROR = 1,
	LOG_FORCE = 0.5,
	LOG_MODULE_EXEC_INFO = 2,
	LOG_INFO = 3,
	LOG_DEBUG = 4,
	DZVERSION = '2.4.24',
}

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
	n = math.pow(10, n or 0)
	x = x * n
	if x >= 0 then
		x = math.floor(x + 0.5)
	else
		x = math.ceil(x - 0.5)
	end
	return x / n
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
		marker = marker .. 'Info:  '
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
		if h >= 240  then return X, 0, C end
		if h >= 180  then return 0, X, C end
		if h >= 120  then return 0, C, X end
		if h >=  60  then return X, C, 0 end
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
