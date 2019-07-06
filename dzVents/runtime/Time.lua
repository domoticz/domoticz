local utils = require('Utils')
local _MS -- kind of a cache so we don't have to extract ms every time

local isEmpty = function(v)
	return (v == nil or v == '')
end

local function getTimezone()
	local now = os.time()
	local dnow = os.date('*t', now)
	local diff = os.difftime(now, os.time(os.date("!*t", now)))
	if (dnow.isdst) then
		diff = diff + 3600
	end

	return diff
end

local LOOKUP = { 'sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat' }

local function getSMs(s)
	local ms = 0
	local parts = utils.stringSplit(s, '.') -- do string splittin instead of math stuff.. can't seem to get the floating points right
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


-- week functions as taken from http://lua-users.org/wiki/WeekNumberInYear
-- Get day of a week at year beginning
--(tm can be any date and will be forced to 1st of january same year)
-- return 1=mon 7=sun
function getYearBeginDayOfWeek(tm)
	local yearBegin = os.time{year=os.date("*t",tm).year,month=1,day=1}
	local yearBeginDayOfWeek = tonumber(os.date("%w",yearBegin))
	-- sunday correct from 0 -> 7
	if(yearBeginDayOfWeek == 0) then
		yearBeginDayOfWeek = 7
	end
	return yearBeginDayOfWeek
end

-- tm: date (as retruned fro os.time)
-- returns basic correction to be add for counting number of week
--  weekNum = math.floor((dayOfYear + returnedNumber) / 7) + 1
-- (does not consider correctin at begin and end of year)
function getDayAdd(tm)
	local yearBeginDayOfWeek = getYearBeginDayOfWeek(tm)
	local dayAdd
	if(yearBeginDayOfWeek < 5 ) then
		-- first day is week 1
		dayAdd = (yearBeginDayOfWeek - 2)
	else
		-- first day is week 52 or 53
		dayAdd = (yearBeginDayOfWeek - 9)
	end
	return dayAdd
end
-- tm is date as returned from os.time()
-- return week number in year based on ISO8601
-- (week with 1st thursday since Jan 1st (including) is considered as Week 1)
-- (if Jan 1st is Fri,Sat,Sun then it is part of week number from last year -> 52 or 53)
function getWeekNumberOfYear(tm)
	local dayOfYear = os.date("%j",tm)
	local dayAdd = getDayAdd(tm)
	local dayOfYearCorrected = dayOfYear + dayAdd
	if(dayOfYearCorrected < 0) then
		-- week of last year - decide if 52 or 53
		local lastYearBegin = os.time{year=os.date("*t",tm).year-1,month=1,day=1}
		local lastYearEnd = os.time{year=os.date("*t",tm).year-1,month=12,day=31}
		dayAdd = getDayAdd(lastYearBegin)
		dayOfYear = dayOfYear + os.date("%j",lastYearEnd)
		dayOfYearCorrected = dayOfYear + dayAdd
	end
	local weekNum = math.floor((dayOfYearCorrected) / 7) + 1
	if( (dayOfYearCorrected > 0) and weekNum == 53) then
		-- check if it is not considered as part of week 1 of next year
		local nextYearBegin = os.time{year=os.date("*t",tm).year+1,month=1,day=1}
		local yearBeginDayOfWeek = getYearBeginDayOfWeek(nextYearBegin)
		if(yearBeginDayOfWeek < 5 ) then
		  weekNum = 1
		end
	end
	return weekNum
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

	local msDiff, secDiff, minDiff, hourDiff, dayDiff, cmp = getDiffParts(os.difftime(tToday, dDate), ms, getMS())

	if (cmp > 0) then
		-- time is in the future so the xxAgo items should be negative
		msDiff = -msDiff
		secDiff = -secDiff
		minDiff = -minDiff
		hourDiff = -hourDiff
		dayDiff = -dayDiff
	end

	if (isUTC) then
		localTime = os.date('*t', os.time(time) + getTimezone())
		self = localTime
		self['utcSystemTime'] = now
		self['utcTime'] = time
		self['utcTime'].minutes = time.min
		self['utcTime'].seconds = time.sec
	else
		self = time
	end

	self.rawDate = self.year .. '-' .. string.format("%02d", self.month) .. '-' .. string.format("%02d", self.day)
	self.rawTime = string.format("%02d", self.hour) .. ':' .. string.format("%02d", self.min) .. ':' .. string.format("%02d", self.sec)
	self.rawDateTime = self.rawDate .. ' ' .. self.rawTime
	self.milliSeconds = ms
	self.milliseconds = ms
	self.dayAbbrOfWeek = LOOKUP[self.wday]

	-- Note: %V doesn't work on Windows so we have to use a custum function here
	-- doesn't work: self.week = tonumber(os.date('%V', dDate))
	self.week = getWeekNumberOfYear(dDate)

	self['raw'] = sDate
	self['isToday'] = (now.year == time.year and
		now.month==time.month and
		now.day==time.day)

	self['msAgo'] = msDiff
	self['millisecondsAgo'] = msDiff
	self['minutesAgo'] = minDiff
	self['secondsAgo'] = secDiff
	self['hoursAgo'] = hourDiff
	self['daysAgo'] = dayDiff

	self['minutes'] = self.min
	self['seconds'] = self.sec

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

	-- compares to self against the provided Time object (t)
	function self.compare(t)
		if (t.raw ~= nil) then

			local msDiff, secDiff, minDiff, hourDiff, dayDiff, cmp =
				getDiffParts(os.difftime(dDate, t.dDate), t.milliseconds, ms) -- offset is 'our' ms value

			return {
				mins = minDiff,
				hours = hourDiff,
				secs = secDiff,
				seconds = secDiff,
				minutes = minDiff,
				days = dayDiff,
				ms = msDiff,
				milliseconds = msDiff,
				compare = cmp -- 0 == equal, -1==(t earlier than self), 1=(t later than self)
			}
		else
			utils.log('Invalid time format passed to diff. Should a Time object', utils.LOG_ERROR)
		end
	end

	-- return ISO format
	function self.getISO()
		return os.date("!%Y-%m-%dT%TZ", os.time(time))
	end

	function getCivilTwilightStart()
		return _G.timeofday['CivTwilightStartInMinutes']
	end

	function getCivilTwilightEnd()
		return _G.timeofday['CivTwilightEndInMinutes']
	end

	function getSunset()
		return _G.timeofday['SunsetInMinutes']
	end

	function getSunrise()
		return _G.timeofday['SunriseInMinutes']
	end

	-- return minutes before civil twilight start
	function getMinutesBeforeCivilTwilightStart(minutes)
		return getCivilTwilightStart() - minutes
	end

	-- return minutes after civil twilight start
	function getMinutesAfterCivilTwilightStart(minutes)
		return getCivilTwilightStart() + minutes
	end

	-- return minutes before civil twilight end
	function getMinutesBeforeCivilTwilightEnd(minutes)
		return getCivilTwilightEnd() - minutes
	end

	-- return minutes after civil twilight end
	function getMinutesAfterCivilTwilightEnd(minutes)
		return getCivilTwilightEnd() + minutes
	end

	-- return minutes before sunrise
	function getMinutesBeforeSunrise(minutes)
		return getSunrise() - minutes
	end

	-- return minutes after sunrise
	function getMinutesAfterSunrise(minutes)
		return getSunrise() + minutes
	end

	-- return minutes before sunset
	function getMinutesBeforeSunset(minutes)
		return getSunset() - minutes
	end

	-- return minutes after sunset
	function getMinutesAfterSunset(minutes)
		return getSunset() + minutes
	end

	-- returns hours part and minutes part of the passed-in minutes amount
	function minutesToTime(minutes)

		local hh = math.floor(minutes / 60)
		local mm = minutes - (hh * 60)

		return hh, mm
	end

	-- returns true if the current time is within a time range: startH:startM and stopH:stopM
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

	-- returns true if self.day is on the rule: on day1,day2...
	function self.ruleIsOnDay(rule)
		local days = string.match(rule, 'on% (.+)$')
		if (isEmpty(days)) then
			return nil
		end

		local isDayRule = false
		for i,day in pairs({'mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'}) do
			if (string.find(days, day) ~= nil) then
				isDayRule = true
				break
			end
		end

		if (not isDayRule) then
			return nil
		end

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

	-- returns true if self.week matches rule in week 1,3,4 / every odd-week, every even-week, in week 5-12,23,44
	function self.ruleIsInWeek(rule)

		if (string.find(rule, 'every odd week') and not ((self.week % 2) == 0)) then
			return true
		elseif (string.find(rule, 'every even week') and ((self.week % 2) == 0)) then
			return true
		elseif string.find(rule, 'every even week') or string.find(rule, 'every odd week') then
			return false
		end

		local weeks = string.match(rule, 'in week% ([0-9%-%,% ]*)')

		if (weeks == nil) then
			return nil
		end

		-- from here on, if there is a match we return true otherwise false

		-- remove spaces and add a comma
		weeks = string.gsub(weeks, ' ', '') .. ',' --remove spaces and add a , so each number is terminated with a , so we can do simple search for the number

		-- do a quick scan first to see if we already have a match without needing to search for ranges
		if (string.find(weeks, tostring(self.week) .. ',')) then
			return true
		end

		-- now get the ranges
		for set, from, to in string.gmatch(weeks, '(([0-9]*)-([0-9]*))') do
			to = tonumber(to)
			from = tonumber(from)
			if (isEmpty(from) and not isEmpty(to) and self.week <= to ) then
				return true
			end
			if (not isEmpty(from) and isEmpty(to) and self.week >= from ) then
				return true
			end
			if (not isEmpty(from) and not isEmpty(to) and to ~= nil) then
				if (self.week >= from and self.week <= to) then
					return true
				end
			end
		end

		return false
	end

	function self.ruleIsOnDate(rule)

		local dates = string.match(rule, 'on% ([0-9%*%/%,% %-]*)')
		if (isEmpty(dates)) then
			return nil
		end

		-- remove spaces and add a comma
		dates = string.gsub(dates, ' ', '') .. ',' --remove spaces and add a , so each number is terminated with a , so we can do simple search for the number
		-- do a quick scan first to see if we already have a match without needing to search for ranges and wildcards

		local dday = ''
		local mmonth = ''

		local _ = require('lodash')

		if (self.day < 10) then dday = '0' .. tostring(self.day) end
		if (self.month < 10) then mmonth = '0' .. tostring(self.month) end

		if (
			string.find(dates, tostring(self.day) .. '/' .. tostring(self.month) .. ',') or
			(dday~='' and string.find(dates, dday .. '/' .. tostring(self.month) .. ',')) or
			(mmonth ~= '' and string.find(dates, tostring(self.day) .. '/' .. mmonth .. ',')) or
			(dday ~= '' and mmonth ~= '' and string.find(dates, dday .. '/' .. mmonth .. ','))
		) then
			return true
		end

		-- wildcards
		for set, day, month in string.gmatch(dates, '(([0-9%*]*)/([0-9%*]*))') do
			if (day == '*' and month ~= '*') then
				if (self.month == tonumber(month)) then
					return true
				end
			end
			if (day ~= '*' and month == '*') then
				if (self.day == tonumber(day)) then
					return true
				end
			end
		end

		local getParts = function(set)
			local day, month = string.match(set, '([0-9%*]+)/([0-9%*]+)')
			return day and tonumber( day ), month and tonumber( month )
		end

		--now get the ranges
		for fromSet, toSet in string.gmatch(dates, '([0-9%/%*]*)-([0-9%/%*]*)') do
			local fromDay, toDay, fromMonth, toMonth

			if (isEmpty(fromSet) and not isEmpty(toSet)) then
				toDay, toMonth = getParts(toSet)
				if ((self.month < toMonth) or (self.month == toMonth and self.day <= toDay)) then
					return true
				end
			elseif (not isEmpty(fromSet) and isEmpty(toSet)) then
				fromDay, fromMonth = getParts(fromSet)
				if ((self.month > fromMonth) or (self.month == fromMonth and self.day >= fromDay)) then
					return true
				end
			else

				toDay, toMonth = getParts(toSet)
				fromDay, fromMonth = getParts(fromSet)
				--local _ = require('lodash');
				--_.print('sm', self.month, 'sd', self.day, 'fm', fromMonth, 'tm', toMonth, 'fd', fromDay, 'td', toDay)
				--_.print('( self.month > fromMonth and self.month < toMonth )', (self.month > fromMonth and self.month < toMonth))
				--_.print('( fromMonth == toMonth and self.month == fromMonth and self.day >= fromDay and self.day <= toDay ) or', (fromMonth == toMonth and self.month == fromMonth and self.day >= fromDay and self.day <= toDay))
				--_.print('( self.month == fromMonth and toMonth < fromMonth and self.day >= fromDay ) or', (self.month == fromMonth and toMonth < fromMonth and self.day >= fromDay) )
				--_.print('( self.month == toMonth and toMonth < fromMonth and  self.day <= toDay )', (self.month == toMonth and toMonth < fromMonth and self.day <= toDay))
				--_.print('( self.month == toMonth and toMonth > fromMonth and self.day <= toDay )', (self.month == toMonth and toMonth > fromMonth and self.day <= toDay))
				if (
					( self.month > fromMonth and self.month < toMonth ) or
					( fromMonth == toMonth and self.month == fromMonth and self.day >= fromDay and self.day <= toDay ) or
					( self.month == fromMonth and toMonth < fromMonth and self.day >= fromDay ) or
					( self.month == fromMonth and toMonth > fromMonth and self.day >= fromDay ) or
					( self.month == toMonth and toMonth < fromMonth and  self.day <= toDay ) or
					( self.month == toMonth and toMonth > fromMonth and self.day <= toDay )
				) then
					return true
				end
			end
		end

		return false
	end

	--returns true if self.time is at civil twilight start
	function self.ruleIsAtCivilTwilightStart(rule)
		if (string.find(rule, 'at civiltwilightstart')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getCivilTwilightStart())
		end

		return nil -- no 'at civiltwilightstart' was specified in rule
	end

	-- returns true if self.time is in the rule 'xx minutes before civiltwilightstart'
	function self.ruleIsBeforeCivilTwilightStart(rule)
		-- xx minutes before civil twilight start

		local minutes = tonumber(string.match(rule, '(%d+) minutes before civiltwilightstart'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeCivilTwilightStart(minutes))
		end

		return nil -- no xx minutes before civil twilight start found
	end

	-- return true if the self.time is in the rule xx minutes after civil twilight start
	function self.ruleIsAfterCivilTwilightStart(rule)
		-- xx minutes after civil twilight start

		local minutes = tonumber(string.match(rule, '(%d+) minutes after civiltwilightstart'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterCivilTwilightStart(minutes))
		end

		return nil -- no xx minutes after civil twilight start found
	end

	--returns true if self.time is at civil twilight end
	function self.ruleIsAtCivilTwilightEnd(rule)
		if (string.find(rule, 'at civiltwilightend')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getCivilTwilightEnd())
		end

		return nil -- no 'at civiltwilightend' was specified in rule
	end

	-- returns true if self.time is in the rule 'xx minutes before civiltwilightend'
	function self.ruleIsBeforeCivilTwilightEnd(rule)
		-- xx minutes before civil twilight end

		local minutes = tonumber(string.match(rule, '(%d+) minutes before civiltwilightend'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeCivilTwilightEnd(minutes))
		end

		return nil -- no xx minutes before civil twilight end found
	end

	-- return true if the self.time is in the rule xx minutes after civil twilight end
	function self.ruleIsAfterCivilTwilightEnd(rule)
		-- xx minutes after civil twilight end

		local minutes = tonumber(string.match(rule, '(%d+) minutes after civiltwilightend'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterCivilTwilightEnd(minutes))
		end

		return nil -- no xx minutes after civil twilight end found
	end

	--returns true if self.time is at sunrise
	function self.ruleIsAtSunrise(rule)
		if (string.find(rule, 'at sunrise')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getSunrise())
		end

		return nil -- no 'at sunrise' was specified in rule
	end

	-- returns true if self.time is in the rule 'xx minutes before sunrise'
	function self.ruleIsBeforeSunrise(rule)
		-- xx minutes before sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeSunrise(minutes))
		end

		return nil -- no xx minutes before sunrise found
	end

	-- return true if the self.time is in the rule xx minutes after sunrise
	function self.ruleIsAfterSunrise(rule)
		-- xx minutes after sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterSunrise(minutes))
		end

		return nil -- no xx minutes after sunrise found
	end

	-- returns true if self.time is at sunset
	function self.ruleIsAtSunset(rule)
		if (string.find(rule, 'at sunset')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == getSunset())
		end

		return nil -- no 'at sunset' was specified in the rule
	end

	-- returns true if self.time is before sunset
	function self.ruleIsBeforeSunset(rule)
		-- xx minutes before sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesBeforeSunset(minutes))
		end

		return nil -- no xx minutes before sunset found
	end

	--returns true if self.time is after sunset
	function self.ruleIsAfterSunset(rule)
		-- xx minutes after sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == getMinutesAfterSunset(minutes))
		end

		return nil -- no xx minutes before sunset found
	end

	-- returns true if self.time is after civil twilight start and before civil twilight end
	function self.ruleIsAtCivilNightTime(rule)
		if (string.find(rule, 'at civilnighttime')) then
			return _G.timeofday['Civilnighttime'] -- coming from domoticz
		end
		return nil -- no 'at civilnighttime' was specified in the rule
	end

	-- return true if self.time is after civil twilight end and before civil twilight start
	function self.ruleIsAtCivilDayTime(rule)
		if (string.find(rule, 'at civildaytime')) then
			return _G.timeofday['Civildaytime'] -- coming from domoticz
		end
		return nil -- no 'at civildaytime' was specified
	end

	-- returns true if self.time is after sunset and before sunrise
	function self.ruleIsAtNight(rule)
		if (string.find(rule, 'at nighttime')) then
			return _G.timeofday['Nighttime'] -- coming from domoticz
		end
		return nil -- no 'at nighttime' was specified in the rule
	end

	-- return true if self.time is after sunrise and before sunset (daytime)
	function self.ruleIsAtDayTime(rule)
		if (string.find(rule, 'at daytime')) then
			return _G.timeofday['Daytime'] -- coming from domoticz
		end
		return nil -- no 'at daytime' was specified
	end

	-- returns true if self.min fits in the every xx minute /every other minute/every minute rule
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

	-- return true if self.hour fits the every hour/every other hour/every xx hours rule
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

	-- return true if self.time is at hh:mm / *:mm/ hh:*
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

	-- returns true if self.time is in time range: at hh:mm-hh:mm
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

	-- returns true if 'moment' matches with any of the moment-like time rules
	function getMoment(moment)
		local minutes
		-- first check if it in the form of hh:mm
		hh, mm = string.match(moment, '([0-9]+):([0-9]+)')

		if (hh ~= nil and mm ~= nil) then
			return tonumber(hh), tonumber(mm)
		end

		-- check if it is before civil twilight start
		minutes = tonumber(string.match(moment, '(%d+) minutes before civiltwilightstart'))
		if (minutes) then
			return minutesToTime(getMinutesBeforeCivilTwilightStart(minutes))
		end

		-- check if it is after civil twilight start
		minutes = tonumber(string.match(moment, '(%d+) minutes after civiltwilightstart'))
		if (minutes) then
			return minutesToTime(getMinutesAfterCivilTwilightEnd(minutes))
		end
		-- check if it is before civil twilight end
		minutes = tonumber(string.match(moment, '(%d+) minutes before civiltwilightend'))
		if (minutes) then
			return minutesToTime(getMinutesBeforeCivilTwilightEnd(minutes))
		end

		-- check if it is after civil twilight end
		minutes = tonumber(string.match(moment, '(%d+) minutes after civiltwilightend'))
		if (minutes) then
			return minutesToTime(getMinutesAfterCivilTwilightEnd(minutes))
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

		-- check at civiltwilightstart
		local twilightstart = string.match(moment, 'civiltwilightstart')
		if (twilightstart) then
			return minutesToTime(getCivilTwilightStart())
		end

		-- check at civiltwilightend
		local twilightend = string.match(moment, 'civiltwilightend')
		if (twilightend) then
			return minutesToTime(getCivilTwilightEnd())
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

	-- returns true if self.time is in a between xx and yy range
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

	-- returns true if self.time matches the rule
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

		res = self.ruleIsInWeek(rule)
		if (res == false) then
			-- in week <weeks> was specified but 'now' is not
			-- on any of the specified weeks
			return false
		end

		updateTotal(res)

		res = self.ruleIsOnDate(rule)
		if (res == false) then
			-- on date <dates> was specified but 'now' is not
			-- on any of the specified dates
			return false
		end
		updateTotal(res)

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
				return false
			end
			updateTotal(res)


			res = self.ruleIsAfterSunset(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsBeforeCivilTwilightStart(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsAfterCivilTwilightStart(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsBeforeCivilTwilightEnd(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsAfterCivilTwilightEnd(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsBeforeSunrise(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)

			res = self.ruleIsAfterSunrise(rule) -- moment
			if (res == false) then
				return false
			end
			updateTotal(res)
		end

		res = self.ruleIsAtCivilTwilightStart(rule) -- moment
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtCivilTwilightEnd(rule) -- moment
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtSunset(rule) -- moment
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtSunrise(rule) -- moment
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtNight(rule) -- range
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtDayTime(rule) -- range
		if (res == false) then
			return false
		end
		updateTotal(res)
		
		res = self.ruleIsAtCivilNightTime(rule) -- range
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtCivilDayTime(rule) -- range
		if (res == false) then
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtDayTime(rule) -- range
		if (res == false) then
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
