local utils = require('Utils')

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

local function Time(sDate, isUTC)
	local now

	if (isUTC~=nil and isUTC==true) then
		now = os.date('!*t')
	else
		now = os.date('*t')
	end

	local time = {}
	local localTime = {} -- nonUTC
	local self = {}
	if (sDate ~= nil and sDate ~= '') then
		local y,mon,d,h,min,s = string.match(sDate, "(%d+)%-(%d+)%-(%d+)% (%d+):(%d+):(%d+)")
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

		local secDiff = os.difftime(tToday, dDate)
		local minDiff = math.floor((secDiff / 60))
		local hourDiff = math.floor((secDiff / 3600))

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

		self.dayAbbrOfWeek = LOOKUP[self.wday]

		self['raw'] = sDate
		self['isToday'] = (now.year == time.year and
			now.month==time.month and
			now.day==time.day)

		self['minutesAgo'] = minDiff
		self['secondsAgo'] = secDiff
		self['hoursAgo'] = hourDiff
		self['secondsSinceMidnight'] = self.hour * 3600 + self.min * 60 + self.sec
		self['utils'] = utils
	end

	if (_G.TESTMODE) then
		function self._getUtilsInstance()
			return utils
		end
	end

	self['current'] = os.date('*t')

	function self.getISO()
		return os.date("!%Y-%m-%dT%TZ", os.time(time))
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
			return (minutesnow == _G.timeofday['SunriseInMinutes'])
		end

		return nil -- no 'at sunrise' was specified in rule
	end

	function self.ruleIsBeforeSunrise(rule)
		-- xx minutes before sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == _G.timeofday['SunriseInMinutes'] - minutes)
		end

		return nil -- no xx minutes before sunrise found
	end

	function self.ruleIsAfterSunrise(rule)
		-- xx minutes after sunrise

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunrise'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == _G.timeofday['SunriseInMinutes'] + minutes)
		end

		return nil -- no xx minutes after sunrise found
	end

	function self.ruleIsAtSunset(rule)
		if (string.find(rule, 'at sunset')) then
			local minutesnow = self.min + self.hour * 60
			return (minutesnow == _G.timeofday['SunsetInMinutes'])
		end

		return nil -- no 'at sunset' was specified in the rule
	end

	function self.ruleIsBeforeSunset(rule)
		-- xx minutes before sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes before sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == _G.timeofday['SunsetInMinutes'] - minutes)
		end

		return nil -- no xx minutes before sunset found
	end

	function self.ruleIsAfterSunset(rule)
		-- xx minutes after sunset

		local minutes = tonumber(string.match(rule, '(%d+) minutes after sunset'))

		if (minutes ~= nil) then

			local minutesnow = self.min + self.hour * 60

			return (minutesnow == _G.timeofday['SunsetInMinutes'] + minutes)
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
		fromH, fromM, toH, toM = string.match(rule, 'at% ([0-9%*]+):([0-9%*]+)-([0-9%*]+):([0-9%*]+)')

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

		res = self.ruleIsOnDay(rule)
		if (res == false) then
			-- on <days> was specified but 'now' is not
			-- on any of the specified days
			return false
		end
		updateTotal(res)

		res = self.ruleIsBeforeSunset(rule)
		if (res == false) then
			-- rule has xx minutes before sunset and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAfterSunset(rule)
		if (res == false) then
			-- rule has xx minutes after sunset and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsBeforeSunrise(rule)
		if (res == false) then
			-- rule has xx minutes before sunrise and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAfterSunrise(rule)
		if (res == false) then
			-- rule has xx minutes after sunrise and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtSunset(rule)
		if (res == false) then
			-- rule has at sunset and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtSunrise(rule)
		if (res == false) then
			-- rule has at sunrise and now is not at that time
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtNight(rule)
		if (res == false) then
			-- rule has at nighttime but it is not night time now
			return false
		end
		updateTotal(res)

		res = self.ruleIsAtDayTime(rule)
		if (res == false) then
			-- rule has at daytime but it is at night now
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesHourSpecification(rule)
		if (res == false) then
			-- rule has every xx hour but its not the right time
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesMinuteSpecification(rule)
		if (res == false) then
			-- rule has every xx minute but its not the right time
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesTime(rule)
		if (res == false) then
			-- rule had at hh:mm part but didn't match (or was invalid)
			return false
		end
		updateTotal(res)

		res = self.ruleMatchesTimeRange(rule)
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