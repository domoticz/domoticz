local self = {
	LOG_ERROR = 1,
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

function self.log(msg, level)

	if (level == nil) then level = self.LOG_INFO end

	local lLevel = _G.logLevel == nil and 1 or _G.logLevel
	local marker = ''

	if (_G.logMarker ~= nil) then
		marker = _G.logMarker .. ': '
	end

	if (level <= lLevel) then
		self.print(tostring(marker) .. msg)
	end
end

return self