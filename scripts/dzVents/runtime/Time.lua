
local function getTimezone()
	local now = os.time()
	local dnow = os.date('*t', now)
	local diff = os.difftime(now, os.time(os.date("!*t", now)))
	if (dnow.isdst) then
		diff = diff + 3600
	end

	return diff
end


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

		self['raw'] = sDate
		self['isToday'] = (now.year == time.year and
			now.month==time.month and
			now.day==time.day)

		self['minutesAgo'] = minDiff
		self['secondsAgo'] = secDiff
		self['hoursAgo'] = hourDiff
	end

	self['current'] = os.date('*t')

	function self.getISO()
		return os.date("!%Y-%m-%dT%TZ", os.time(time))
	end

	return self
end

return Time