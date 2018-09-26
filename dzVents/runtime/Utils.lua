local jsonParser
local _ = require('lodash')

local self = {
	LOG_ERROR = 1,
	LOG_FORCE = 0.5,
	LOG_MODULE_EXEC_INFO = 2,
	LOG_INFO = 3,
	LOG_DEBUG = 4,
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

function self.fromJSON(json)

	local parse = function(j)
		return jsonParser:decode(j)
	end

	if (jsonParser == nil) then
		jsonParser = require('JSON')
	end

	ok, results = pcall(parse, json)

	if (ok) then
		return results
	end

	self.log('Error parsing json to LUA table: ' .. results, self.LOG_ERROR)
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
		marker = marker .. 'Error (2.4.7): '
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

return self
