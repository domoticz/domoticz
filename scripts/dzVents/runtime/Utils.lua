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

function self.urlEncode(str)
	if (str) then
		str = string.gsub(str, "\n", "\r\n")
		str = string.gsub(str, "([^%w %-%_%.%~])",
			function(c) return string.format("%%%02X", string.byte(c)) end)
		str = string.gsub(str, " ", "+")
	end
	return str
end

function self.log(msg, level)

	if (level == nil) then level = self.LOG_INFO end

	local lLevel = _G.logLevel == nil and 1 or _G.logLevel
	local marker = ''


	if (level == self.LOG_ERROR) then
		marker = marker .. 'Error: '
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
		self.print(tostring(marker) .. msg)
	end
end

return self