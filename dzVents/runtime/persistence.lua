-- Internal persistence library

--[[ Provides ]]
-- persistence.store(path, ...): Stores arbitrary items to the file at the given path
-- persistence.load(path): Loads files that were previously stored with store and returns them

--[[ Limitations ]]
-- Does not export userdata, threads or function values

-- Data is serialized with serpent into a loadable Lua chunk. The write is
-- crash-safe: the chunk is first written to <path>.tmp, flushed and closed,
-- the previous file is preserved as <path>.bak, and the temporary file is
-- then renamed over the target (atomic on POSIX, so there is never a moment
-- without a complete data file). When the serialized data is identical to
-- what is already on disk the write is skipped entirely, which reduces both
-- flash wear and the number of windows in which a power cut can do harm.

local serpent = require('serpent')

local SERPENT_OPTIONS = { indent = '\t', sortkeys = true, comment = false, nocode = true }

local function serialize(...)
	local n = select('#', ...)
	local parts = { '-- Persistent Data' }
	if n == 0 then
		table.insert(parts, 'return')
	elseif n == 1 then
		table.insert(parts, serpent.dump((select(1, ...)), SERPENT_OPTIONS))
	else
		local names = {}
		for i = 1, n do
			table.insert(parts, 'local obj' .. i .. ' = (function() ' .. serpent.dump((select(i, ...)), SERPENT_OPTIONS) .. ' end)()')
			table.insert(names, 'obj' .. i)
		end
		table.insert(parts, 'return ' .. table.concat(names, ', '))
	end
	return table.concat(parts, '\n') .. '\n'
end

local function readFile(path)
	local file = io.open(path, 'rb')
	if not file then
		return nil
	end
	local content = file:read('*a')
	file:close()
	return content
end

local function writeFile(path, content)
	local file, err = io.open(path, 'wb')
	if not file then
		return nil, err
	end
	file:write(content)
	file:flush()
	file:close()
	return true
end

persistence =
{
	store = function (path, ...)
		local serialized = serialize(...)

		if type(path) ~= 'string' then
			-- treat it as an already opened file
			path:write(serialized)
			return
		end

		local current = readFile(path)
		if current == serialized then
			return
		end

		local tmpPath = path .. '.tmp'
		local ok, err = writeFile(tmpPath, serialized)
		if not ok then
			return error(err)
		end

		if current ~= nil then
			-- keep the previous data for crash recovery
			writeFile(path .. '.bak', current)
		end

		ok, err = os.rename(tmpPath, path)
		if not ok then
			-- Windows cannot rename over an existing file
			os.remove(path)
			ok, err = os.rename(tmpPath, path)
			if not ok then
				return error(err)
			end
		end
	end;

	load = function (path)
		local f, e = loadfile(path)
		if f then
			return f();
		else
			return nil, e;
		end;
	end;
}

return persistence
