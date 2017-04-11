local self = {
	LOG_INFO = 2,
	LOG_MODULE_EXEC_INFO = 1.5,
	LOG_DEBUG = 3,
	LOG_ERROR = 1
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

function self.getDevicesPath()
	if (_G.TESTMODE) then
		return globalvariables['script_path'] .. '/tests/devices.lua'
	else
		return globalvariables['script_path'] .. '../devices.lua' -- parent folder
	end
end

function self.osExecute(cmd)
	if (_G.TESTMODE) then return end
	os.execute(cmd)
end

function self.getSed(target, replacement)
	return "sed 's/" .. target .. "/" .. replacement .. "/'"
end

function self.requestDomoticzData(ip, port)
	-- create a bunch of commands that will convert
	-- the json returned from Domoticz into a lua table
	-- of course you can use json parsers but that either
	-- requires installing packages or takes a lot
	-- of lua processing power since the json can be huge
	-- the call is detached from the Domoticz process to it more or less
	-- runs in its own process, not blocking execution of Domoticz

	local filePath = self.getDevicesPath() .. '.tmp'

	if (not self.fileExists(filePath)) then
		local sed1 = self.getSed("],", "},")
		local sed2 = self.getSed('   "', '   ["')
		local sed3 = self.getSed('         "', '         ["')
		local sed4 = self.getSed('" :', '"]=')
		local sed5 = self.getSed(': \\[', ': {')
		local sed6 = self.getSed('= \\[', '= {')

		local cmd = "{ echo 'return ' ; curl 'http://" ..
				ip .. ":" .. port ..
				"/json.htm?type=devices&displayhidden=1&filter=all&used=true' -s " ..
				"; } " ..
				" | " .. sed1 ..
				" | " .. sed2 ..
				" | " .. sed3 ..
				" | " .. sed4 ..
				" | " .. sed5 ..
				" | " .. sed6 .. " > " .. filePath .. " 2>/dev/null &"

		-- this will create a lua-requirable file with fetched data
		self.log('Fetching Domoticz data: ' .. cmd, self.LOG_DEBUG)
		self.osExecute(cmd)
	end
end

function self.activateDevicesFile()

	if (_G.TESTMODE) then return end

	local tmpFilePath = self.getDevicesPath() .. '.tmp'

	if (self.fileExists(tmpFilePath)) then

		local fd = io.popen('lsof "' .. tmpFilePath .. '"')
		local fileopened = (#fd:read("*a") > 0)

		if (not fileopened) then
			local targetFilePath = self.getDevicesPath()
			local mvCmd = 'mv -f "' .. tmpFilePath .. '" "' .. targetFilePath .. '"'

			self.log('Copying ' .. tmpFilePath .. ' to ' .. targetFilePath, self.LOG_DEBUG)
			self.osExecute(mvCmd)

		else
			self.log('Skipping copying devices.lua.tmp, file is being written', self.LOG_DEBUG)
		end
	end

end

function self.print(msg)
	if (_G.TESTMODE) then return end
	print(msg)
end

function self.log(msg, level)

	if (level == nil) then level = self.LOG_INFO end

	local lLevel = _G.logLevel == nil and 1 or _G.logLevel

	if (level <= lLevel) then
		self.print(msg)
	end
end

return self