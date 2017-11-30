local Time = require('Time')
local MAXLIMIT = 100
local utils = require('Utils')

if (_G.TESTMODE) then
	MAXLIMIT = 10
end

local function setIterators(object, collection)
	local res
	object['forEach'] = function(func)
		for i, item in ipairs(collection) do
			res = func(item, i, collection)
			if (res == false) then -- abort
				return
			end
		end
	end

	object['reduce'] = function(func, accumulator)
		for i, item in ipairs(collection) do
			accumulator = func(accumulator, item, i, collection)
		end
		return accumulator
	end

	object['find'] = function(func, direction)
		local stop = false
		local from, to
		if (direction == -1) then
			from = #collection -- last index in table
			to = 1
		else
			direction = 1 -- to be sure
			from = 1
			to = #collection
		end
		for i = from, to, direction do
			local item = collection[i]
			stop = func(item, i, collection)
			if (stop) then
				return item, i
			end
		end
		return nil, nil
	end

	object['filter'] = function(filter)
		local res = {}
		for i, item in ipairs(collection) do
			if (filter(item, i, collection)) then
				res[i] = item
			end
		end
		setIterators(res, res)
		return res
	end
end

local function HistoricalStorage(data, maxItems, maxHours, maxMinutes, getData)
	-- IMPORTANT: data must be time-stamped in UTC format

	local newAdded = false
	if (maxItems == nil or maxItems > MAXLIMIT) then
		maxItems = MAXLIMIT
	end
	-- maybe we should make a limit anyhow in the number of items

	local self = {
		newData = nil,
		storage = {} -- youngest first, oldest last
	}

	-- setup our internal list of history items
	-- already pruned to the bounds as set by maxItems and/or maxHours

	maxMinutes = maxMinutes and maxMinutes or 0
	maxHours = maxHours and maxHours or 0
	maxMinutes = maxMinutes + maxHours * 60

	if (data == nil) then
		self.storage = {}
		self.size = 0
	else
		-- transfer to self
		-- that way we can easily prune or ditch based
		-- on maxItems and/or maxHours
		local count = 0
		for i, sample in ipairs(data) do
			local t = Time(sample.time, true) -- UTC

			if (count < maxItems) then
				local add = true
				if (maxMinutes > 0 and t.minutesAgo>maxMinutes) then
					add = false
				end
				if (add) then
					table.insert(self.storage, { time = t, data = sample.data })
					count = count + 1
				end
			end
		end
		self.size = count
	end

	-- extend with filter and forEach
	setIterators(self, self.storage)

	local function getSecondsAgo(t)
		local hoursAgo, minsAgo, secsAgo = string.match(t, "(%d+):(%d+):(%d+)")
		secsAgo = secsAgo~=nil and secsAgo or 0
		minsAgo = minsAgo~=nil and minsAgo or 0
		hoursAgo = hoursAgo~=nil and hoursAgo or 0
		return hoursAgo*3600 + minsAgo*60 + secsAgo
	end

	function self.subset(from, to, _setIterators)
		local res = {}
		local skip = false
		local len = 0

		if (from == nil or from < 1 ) then from = 1 end
		if (from and from > self.size) then	skip = true	end
		if (to and from and to < from) then skip = true end
		if (to==nil or to > self.size) then to = self.size end

		if (not skip) then
			for i = from, to do
				table.insert(res, self.storage[i])
				len = len + 1
			end
		end

		if(_setIterators or _setIterators==nil) then
			setIterators(res, res)
		end
		return res, len
	end

	function self.subsetSince(timeAgo, _setIterators)
		local totalSecsAgo = getSecondsAgo(timeAgo)
		local res = {}
		local len = 0

		for i = 1, self.size do
			if (self.storage[i].time.secondsAgo<=totalSecsAgo) then
				table.insert(res, self.storage[i])
				len = len + 1
			end
		end

		if(_setIterators or _setIterators==nil) then
			setIterators(res, res)
		end
		return res, len

	end

	function self._getForStorage()
		-- create a new table with string time stamps
		local res = {}

		self.forEach(function(item)
			table.insert(res,{
				time = item.time.raw,
				data = item.data
			})
		end)
		return res
	end

	function self.setNew(data)
		utils.log('setNew is deprecated. Please use add().', utils.LOG_INFO)
		self.add(data)
	end

	function self.add(data)
		-- see if we have reached the limit
		-- the oldest item like still fals within the range of maxMinutes/maxHours
		if (self.size == maxItems) then
			-- drop the last item
			table.remove(self.storage)
			self.size = self.size - 1
		end

		-- add the new one
		local t = Time(os.date('!%Y-%m-%d %H:%M:%S'), true)
		table.insert(self.storage, 1, {
			time = t,
			data = data
		})
		self.size = self.size + 1
	end

	function self.getNew()
		utils.log('getNew is deprecated. Please use getLatest().', utils.LOG_INFO)
		return self.getLatest()
	end

	function self.get(itemsAgo)
		local item = self.storage[itemsAgo]
		if (item == nil) then
			return nil
		else
			return item
		end
	end

	function self.getAtTime(timeAgo)
		-- find the item closest to minsAgo+hoursAgo
		local totalSecsAgo = getSecondsAgo(timeAgo)
		local res = {}

		for i = 1, self.size do
			if (self.storage[i].time.secondsAgo > totalSecsAgo) then

				if (i>1) then
					local deltaWithPrevious = totalSecsAgo - self.storage[i-1].time.secondsAgo
					local deltaWithCurrent = self.storage[i].time.secondsAgo - totalSecsAgo

					if (deltaWithPrevious < deltaWithCurrent) then
						-- the previous one was closer to the time we were looking for
						return self.storage[i-1], i-1
					else
						return self.storage[i], i
					end
				else
					return self.storage[i], i
				end
			end
		end
		return nil, nil
	end

	function self.getLatest()
		return self.get(1)
	end

	function self.getOldest()
		return self.get(self.size)
	end

	function self.reset()
		for k, v in pairs(self.storage) do
			self.storage[k] = nil
		end
		self.size = 0
		self.newData = nil
	end

	local function _getItemData(item)
		if (type(getData) == 'function') then
			local ok, res = pcall(getData, item)
			if (ok) then
				if (type(res) == 'number') then
					return res
				else
					utils.log('Return value for getData function is not a number: ' .. tostring(res), utils.LOG_ERROR)
					return nil
				end
			else
				utils.log('getData function returned an error. ' .. tostring(res), utils.LOG_ERROR)
			end
		else
			if (type(item.data) == 'number') then
				return item.data
			else
				utils.log('Item data is not a number type. Type is ' .. type(res), utils.LOG_ERROR)
				return nil
			end
		end
	end

	local function _sum(items)
		local count = 0

		local sum = items.reduce(function(acc, item)
			local val = _getItemData(item)
			if (val == nil) then return nil end
			count = count + 1
			return acc + val
		end, 0)
		return sum, count
	end

	local function _avg(items)
		local sum, count = _sum(items)
		return sum/count
	end

	function self.avg(from, to, default)
		local subset, length = self.subset(from, to)

		if (length == 0) then
			return default or 0
		else
			return _avg(subset)
		end
	end

	function self.avgSince(timeAgo, default)
		local subset, length = self.subsetSince(timeAgo)
		if (length == 0) then
			return default or 0
		else
			return _avg(subset)
		end
	end

	local function _min(items)
		local itm =  nil
		local min = items.reduce(function(acc, item)
			local val = _getItemData(item)
			if (val == nil) then return nil end
			if (acc == nil) then
				acc = val
				itm = item
			else
				if (val < acc) then
					acc = val
					itm = item
				end
			end

			return acc
		end, nil)
		return min, itm
	end

	function self.min(from, to)
		local subset, length = self.subset(from, to)
		if (length == 0) then
			return nil, nil
		else
			return _min(subset)
		end
	end

	function self.minSince(timeAgo)
		local subset, length = self.subsetSince(timeAgo)
		if (length==0) then
			return nil, nil
		else
			return _min(subset)
		end
	end

	local function _max(items)
		local itm
		local max = items.reduce(function(acc, item)
			local val = _getItemData(item)
			if (val == nil) then return nil end
			if (acc == nil) then
				acc = val
				itm = item
			else
				if (val > acc) then
					acc = val
					itm = item
				end
			end

			return acc
		end, nil)
		return max, itm
	end

	function self.max(from, to)
		local subset, length = self.subset(from, to)
		if (length==0) then
			return nil, nil
		else
			return _max(subset)
		end
	end

	function self.maxSince(timeAgo)
		local subset, length = self.subsetSince(timeAgo)
		if (length==0) then
			return nil, nil
		else
			return _max(subset)
		end
	end

	function self.sum(from, to)
		local subset, length = self.subset(from, to)
		if (length==0) then
			return 0
		else
			return _sum(subset)
		end
	end

	function self.sumSince(timeAgo)
		local subset, length = self.subsetSince(timeAgo)
		if (length==0) then
			return 0
		else
			return _sum(subset)
		end
	end

	function self.smoothItem(itemIndex, smoothRange)
		if (itemIndex<1 or itemIndex > self.size) then
			return nil
		end

		if (smoothRange == nil or smoothRange < 0) then
			smoothRange = 0
		end

		if (smoothRange == 0) then
			if (itemIndex >= 1 and itemIndex <= self.size) then
				return _getItemData(self.storage[itemIndex])
			else
				return nil
			end
		end

		local from, to
		if ((itemIndex - smoothRange)< 1) then
			from = 1
		else
			from = itemIndex - smoothRange
		end

		if ((itemIndex + smoothRange) > self.size) then
			to = self.size
		else
			to = itemIndex + smoothRange
		end

		local avg = self.avg(from, to)

		return avg
	end

	function self.delta(fromIndex, toIndex, smoothRange, default)
		if (fromIndex < 1 or
			fromIndex > self.size-1 or
			toIndex > self.size or
			toIndex < 1 or
			fromIndex > toIndex or
			toIndex < fromIndex) then
			return default
		end

		local value, item, referenceValue
		if (smoothRange ~= nil) then
			value = self.smoothItem(toIndex, smoothRange)
			referenceValue = self.smoothItem(fromIndex, smoothRange)
		else
			value = _getItemData(self.storage[toIndex])
			if (value == nil) then return nil end
			referenceValue = _getItemData(self.storage[fromIndex])
			if (referenceValue == nil) then return nil end
		end
		return tonumber(referenceValue - value)
	end

	function self.deltaSince(timeAgo, smoothRange, default)
		local item, index = self.getAtTime(timeAgo)

		if (item ~= nil) then
			return self.delta(1, index, smoothRange, default)
		end

		return default
	end

	function self.localMin(smoothRange, default)
		local min, minVal
		self.forEach(function(item, i, collection)

			local val = self.smoothItem(i, smoothRange)
			if (min == nil) then
				-- first one
				min = item
				minVal = val
			else
				if (val < minVal) then
					-- we got one that is even lower
					min = item
					minVal = val
				elseif (val > minVal) then -- we ignore equals
					-- rising again
					-- skip
					return false -- abort
				end
			end
		end)

		return minVal, min
	end

	function self.localMax(smoothRange, default)
		local max, maxVal
		self.forEach(function(item, i, collection)

			local val = self.smoothItem(i, smoothRange)
			if (max == nil) then
				-- first one
				max = item
				maxVal = val
			else
				if (val > maxVal) then
					-- we got one that is even higher
					max = item
					maxVal = val
				elseif (val < maxVal) then -- we ignore equals
				-- lowering again
				-- skip
				return false -- abort
				end
			end
		end)

		return maxVal, max
	end

	return self
end

return HistoricalStorage