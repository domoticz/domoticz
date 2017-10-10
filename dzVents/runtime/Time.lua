local utils = require('Utils')
local _MS -- kind of a cache so we don't have to extract ms every time

local function getTimezone()
	local now = os.time()
	local dnow = os.date('*t', now)
	local diff = os.difftime(now, os.time(os.date("!*t", now)))
	if (dnow.isdst) then
		diff = diff + 3600
	end

	return diff
end

function string:split(sep)
	local sep, fields = sep or ":", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields + 1] = c end)
	return fields
end

local LOOKUP = { 'sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat' }

local function getSMs(s)
	local ms = 0
	local parts = string.split(s, '.') -- do string splittin instead of math stuff.. can't seem to get the floating points right
	s = tonumber(parts[1])
	if (parts[2] ~= nil) then
		-- should always be three digits!!
		ms = tonumber(parts[2])
	end

	return s, ms
end

local function parseDate(sDate)
	return string.match(sDate, "(%d+)%-(%d+)%-(%d+)% (%d+):(%d+):([0-9%.]+)")
end

local function _getMS()

	if (_MS == nil) then

		local dzCurrentTime = _G.globalvariables['currentTime']
		local y, mon, d, h, min, s = parseDate(dzCurrentTime)
		local ms
		s, ms = getSMs(s)
		_MS = ms
	end

	return _MS
end

local getDiffParts = function(secDiff, ms, offsetMS)

	-- ms is the ms part that should be added to secDiff to get 'sss.ms'

	if (ms == nil) then
		ms = 0
	end

	if (offsetMS == nil) then
		offsetMS = 0
	end

	local secs = secDiff
	local msDiff

	msDiff = (secs * 1000) - ms + offsetMS

	if (math.abs(msDiff) < 1000) then
		secs = 0
	end

	local minDiff = math.floor(math.abs((secs / 60)))
	local hourDiff = math.floor(math.abs((secs / 3600)))
	local dayDiff = math.floor(math.abs((secs / 86400)))

	local cmp

	if (msDiff == 0) then
		cmp = 0
	elseif (msDiff > 0) then
		cmp = -1
	else
		cmp = 1
	end

	return
		math.abs(msDiff),
		math.abs(secs),
		minDiff,
		hourDiff,
		dayDiff,
		cmp
end


local function Time(sDate, isUTC, _testMS)

	local ms
	local now
	local time = {}
	local localTime = {} -- nonUTC
	local self = {}

	local getMS = function()

		if (_testMS ~= nil) then
			return _testMS
		else
			return _getMS()
		end

	end

	if (isUTC~=nil and isUTC==true) then
		now = os.date('!*t')
	else
		now = os.date('*t')
		isUTC = false
	end

	if (sDate == nil or sDate == '') then
		local now
		if (isUTC) then
			now = os.date('!*t')
		else
			now = os.date('*t')
		end
		local ms = _testMS == nil and getMS() or _testMS
		sDate = now.year .. '-' .. now.month ..'-' .. now.day .. ' ' .. now.hour .. ':' .. now.min .. ':' .. now.sec .. '.' .. tostring(ms)
	end

	local y,mon,d,h,min,s = parseDate(sDate)

	-- extract s and ms
	s, ms = getSMs(s)

	local dDate = os.time{year=y,month=mon,day=d,hour=h,min=min,sec=s }

	time = os.date('*t', dDate)

	-- calculate how many minutes that was from now
	local tToday = os.time{
		day= now.day,
		year= now.year,
		month= now.month,
		hour= now.hour,
		min= now.min,
		sec= now.sec
	}

	local msDiff, secDiff, minDiff, hourDiff, dayDiff = getDiffParts(os.difftime(tToday, dDate), ms, getMS())

	if (isUTC) then
		localTime = os.date('*t', os.time(time) + getTimezone())
		self = localTime
		self['utcSystemTime'] = now
		self['utcTime'] = time
	else
		self = time
	end

	self.rawDate = self.year .. '-' .. self.month .. '-' .. self.day
	self.rawTime = self.hour .. ':' .. self.min .. ':' .. self.sec
	self.milliSeconds = ms
	self.dayAbbrOfWeek = LOOKUP[self.wday]

	self['raw'] = sDate
	self['isToday'] = (now.year == time.year and
		now.month==time.month and
		now.day==time.day)

	self['msAgo'] = msDiff
	self['minutesAgo'] = minDiff
	self['secondsAgo'] = secDiff
	self['hoursAgo'] = hourDiff
	self['daysAgo'] = dayDiff

	self['secondsSinceMidnight'] = self.hour * 3600 + self.min * 60 + self.sec
	self['utils'] = utils
	self['isUTC'] = isUTC
	self['dDate'] = dDate

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

	self['current'] = os.date('*t')

	function self.compare(t)
		if (t.raw ~= nil) then

			local msDiff, secDiff, minDiff, hourDiff, dayDiff, cmp =
				getDiffParts(os.difftime(dDate, t.dDate), t.milliSeconds, ms) -- offset is 'our' ms value

			return {
				mins = minDiff,
				hours = hourDiff,
				secs = secDiff,
				days = dayDiff,
				ms = msDiff,
				compare = cmp -- 0 == equal, -1==(t earlier than self), 1=(t later than self)
			}
		else
			utils.log('Invalid time format passed to diff. Should a Time object', utils.LOG_ERROR)
		end
	end

	function self.getISO()
		return os.date("!%Y-%m-%dT%TZ", os.time(time))
	end

	function getMinutesBeforeSunrise(minutes)
		return _G.timeofday['SunriseInMinutes'] - minutes
	end

	function getMinutesAfterSunrise(minutes)
		return _G.timeofday['SunriseInMinutes'] + minutes
	end

	function getMinutesBeforeSunset(minutes)
		return _G.timeofday['SunsetInMinutes'] - minutes
	end

	function getMinutesAfterSunset(minutes)
		return _G.timeofday['SunsetInMinutes'] + minutes
	end

	function getSunset()
		return _G.timeofday['SunsetInMinutes']
	end

	function getSunrise()
		return _G.timeofday['SunriseInMinutes']
	end

	function minutesToTime(minutes)

		local hh = math.floor(minutes / 60)
		local mm = minutes - (hh * 60)

		return hh, mm
	end

	local function timeIsInRange(startH, startM, stopH, stopM)

		local function getMinutes(hours, minutes)
			return (hours * 60) + minutes
		end

		local testH = self.hour
		local testM = self.min

		if (stopH < startH) then -- add 24 hours if endhours < startHours
			local stopHOrg = stopH
			stopH = stopH + 24
			if (testH <= stopHOrg) then -- if endhours has increased the currenthour should also increase
				testH = testH + 24
			end
		end

		local startTVal = getMinutes(startH, startM)
		local stopTVal = getMinutes(stopH, stopM)
		local curTVal = getMinutes(testH, testM)
		return (curTVal >= startTVal and curTVal <= stopTVal)
	end

	function self.ruleIsOnDay(rule)
		local days = string.match(rule, 'on% (.+)$')
		if (days ~= nil) then
			-- 'on <day>' was specified
			local hasDayMatch = string.find(days, self.dayAbbrOfWeek)
			if (hasDayMatch) then
				return true
			else
				return false
			end
		end
		return nil -- no 'on <days>' was specified in the rule
	end

	function self.ruleIsAtSunrise(rule)
		if (string.find(rule, 'at sunrise')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getSunrise())
		end

		return nil -- no 'at sunrise' was specified in rule
	end

	function self.ruleIsBeforeSunrise(rule)
		-- xx minutes before sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeSunrise(minutes))
		end

		return nil -- no xx minutes before sunrise found
	end

	function self.ruleIsAfterSunrise(rule)
		-- xx minutes after sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterSunrise(minutes))
		end

		return nil -- no xx minutes after sunrise found
	end

	function self.ruleIsAtSunset(rule)
		if (string.find(rule, 'at sunset')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getSunset())
		end

		return nil -- no 'at sunset' was specified in the rule
	end

	function self.ruleIsBeforeSunset(rule)
		-- xx minutes before sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeSunset(minutes))
		end

		return nil -- no xx minutes before sunset found
	end

	function self.ruleIsAfterSunset(rule)
		-- xx minutes after sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterSunset(minutes))
		end

		return nil -- no xx minutes before sunset found
	end

	function self.ruleIsAtNight(rule)
		if (string.find(rule, 'at nighttime')) then
			return _G.timeofday['Nighttime'] -- coming from domotic
		end
		return nil -- no 'at nighttime' was specified in the rule
	end

	function self.ruleIsAtDayTime(rule)
		if (string.find(rule, 'at daytime')) then
			return _G.timeofday['Daytime'] -- coming from domotic
		end
		return nil -- no 'at daytime' was specified
	end

	function self.ruleMatchesMinuteSpecification(rule)

		local function fitsMinuteRule(m)
			return (self.min / m == math.floor(self.min / m))
		end

		if (string.find(rule, 'every minute')) then
			return true
		end

		if (string.find(rule, 'every other minute')) then
			return fitsMinuteRule(2)
		end

		local minutes = tonumber(string.match(rule, 'every (%d+) minutes'))
		if (minutes ~= nil) then
			if ((60 / minutes) ~= math.floor(60 / minutes) or minutes >= 60) then
				self.utils.log(rule .. ' is not a valid timer definition. Can only run every 1, 2, 3, 4, 5, 6, 10, 12, 15, 20 and 30 minutes.', utils.LOG_ERROR)
				return false
			end

			return fitsMinuteRule(minutes)
		end

		return nil -- nothing specified for this rule
	end

	function self.ruleMatchesHourSpecification(rule)

		local function fitsHourRule(h)
			-- always fit every whole hour (hence the self.min == 0)
			return (self.hour / h == math.floor(self.hour / h) and self.min == 0)
		end

		if (string.find(rule, 'every hour')) then
			return fitsHourRule(1)
		end

		if (string.find(rule, 'every other hour')) then
			return fitsHourRule(2)
		end

		local hours = tonumber(string.match(rule, 'every% (%d+)% hours'))
		if (hours ~= nil) then
			if ((24 / hours) ~= math.floor(24 / hours) or hours >= 24) then
				self.utils.log(rule .. ' is not a valid timer definition. Can only run every  1, 2, 3, 4, 6, 8, 12 hours.', utils.LOG_ERROR)
				return false
			end

			return fitsHourRule(hours)
		end

		return nil -- nothing specified for this rule
	end

	function self.ruleMatchesTime(rule)

		local hh, mm
		local timePattern = 'at% ([0-9%*]+):([0-9%*]+)'

		hh, mm = string.match(rule, timePattern .. '% ')
		if (hh == nil or mm == nil) then
			-- check for end-of string
			hh, mm = string.match(rule, timePattern .. '$')
		end
		if (hh ~= nil) then
			if (mm == '*') then
				return (self.hour == tonumber(hh))
			elseif (hh == '*') then
				return (self.min == tonumber(mm))
			--elseif (hh ~= '*' and hh ~= '*') then
			else
				hh = tonumber(hh)
				mm = tonumber(mm)
				if (hh ~= nil and mm ~= nil) then
					return (mm == self.min and hh == self.hour)
				else
					-- invalid
					return false
				end

			end
		end
		return nil -- no at hh:mm found in rule
	end

	function self.ruleMatchesTimeRange(rule)
		local fromH, fromM, toH, toM = string.match(rule, 'at% ([0-9%*]+):([0-9%*]+)-([0-9%*]+):([0-9%*]+)')

		if (fromH ~= nil) then
			-- all will be nil if fromH is nil
			fromH = tonumber(fromH)
			fromM = tonumber(fromM)
			toH = tonumber(toH)
			toM = tonumber(toM)

			if (fromH == nil or fromM == nil or toH == nil or toM == nil) then
				-- invalid format
				return false
			else
				return timeIsInRange(fromH, fromM, toH, toM)
			end
		end

		return nil

	end

	function getMoment(moment)
		local minutes
		-- first check if it in the form of hh:mm
		hh, mm = string.match(moment, '([0-9]+):([0-9]+)')

		if (hh ~= nil and mm ~= nil) then
			return tonumber(hh), tonumber(mm)
		end

		-- check if it is before sunrise
		minutes = tonumber(string.match(moment, '(%d+) minutes before sunrise'))
		if (minutes) then
			return minutesToTime(getMinutesBeforeSunrise(minutes))
		end

		-- check if it is after sunrise
		minutes = tonumber(string.match(moment, '(%d+) minutes after sunrise'))
		if (minutes) then
			return minutesToTime(getMinutesAfterSunrise(minutes))
		end

		-- check if it is before sunset
		minutes = tonumber(string.match(moment, '(%d+) minutes before sunset'))
		if (minutes) then
			return minutesToTime(getMinutesBeforeSunset(minutes))
		end

		-- check if it is after sunset
		minutes = tonumber(string.match(moment, '(%d+) minutes after sunset'))
		if (minutes) then
			return minutesToTime(getMinutesAfterSunset(minutes))
		end

		-- check at sunrise
		local sunrise = string.match(moment, 'sunrise')
		if (sunrise) then
			return minutesToTime(getSunrise())
		end

		-- check at sunset
		local sunset = string.match(moment, 'sunset')
		if (sunset) then
			return minutesToTime(getSunset())
		end

		return nil
	end

	function self.ruleMatchesBetweenRange(rule)
		-- between 16:20 and 23:15
		-- between sunset and sunrise
		-- between xx minutes before/after sunset/sunrise and ..

		local from, to = string.match(rule, 'between% (.+)% and% (.+)')

		if (from == nil or to == nil) then
			return nil
		end

		local fromHH, fromMM, toHH, toMM

		fromHH, fromMM = getMoment(from)
		toHH, toMM = getMoment(to)
		if (fromHH == nil or fromMM == nil or toHH == nil or toMM == nil) then
			return nil
		end

		return timeIsInRange(fromHH, fromMM, toHH, toMM)

	end

	function self.matchesRule(rule)
		if (string.len(rule == nil and "" or rule) == 0) then
			return false
		end

		local res
		local total = false

		-- at least one processor should return something else than nil
		-- a processor returns true, false or nil. It is nil when the
		-- rule the processor requires is not present

		local function updateTotal(res)
			total = res ~= nil and (total or res) or total
		end

		res = self.ruleIsOnDay(rule) -- range
		if (res == false) then
			-- on <days> was specified but 'now' is not
			-- on any of the specified days
			return false
		end
		updateTotal(res)


		local _between = self.ruleMatchesBetweenRange(rule) -- range
		if (_between == false) then
			-- rule had between xxx and yyy is not in that range now
			return false
		end
		res = _between
		updateTotal(res)

		if (_between == nil) then
			-- there was not a between rule
			-- a between-range can have before/after sunrise/set rules
			-- so it cannot be combined with these here

			res = self.ruleIsBeforeSunset(rule) -- moment
			if (res == false) then
				-- rule has xx minutes before sunset and now is not at that time
				return false
			end
			updateTotal(res)


			res = self.ruleIsAfterSunset(rule) -- moment
			if (res == false) then
				-- rule has xx minutes after sunset and now is not at that time
				return false
			end
			updateTotal(res)

			res = self.ruleIsBeforeSunrise(rule) -- moment
			if (res == false) then
				-- rule has xx minutes before sunrise and now is not at that time
				return false
			end
			updateTotal(res)

			res = self.ruleIsAfterSunrise(rule) -- moment
			if (res == false) then
				-- rule has xx minutes after sunrise and now is not at that time
				return false
			end
			updateTotal(res)
		end

		res = self.ruleIsAtSunset(rule) -- moment
		if (res == false) then
			-- rule has at sunset and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtSunrise(rule) -- moment
		if (res == false) then
			-- rule has at sunrise and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtNight(rule) -- range
		if (res == false) then
			-- rule has at nighttime but it is not night time now
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtDayTime(rule) -- range
		if (res == false) then
			-- rule has at daytime but it is at night now
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesHourSpecification(rule) -- moment
		if (res == false) then
			-- rule has every xx hour but its not the right time
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesMinuteSpecification(rule) -- moment
		if (res == false) then
			-- rule has every xx minute but its not the right time
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesTime(rule) -- moment / range
		if (res == false) then
			-- rule had at hh:mm part but didn't match (or was invalid)
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesTimeRange(rule) -- range
		if (res == false) then
			-- rule had at hh:mm-hh:mm but time is not in that range now
			return false
		end
		updateTotal(res)



		return total

	end

	return self
end

return Time