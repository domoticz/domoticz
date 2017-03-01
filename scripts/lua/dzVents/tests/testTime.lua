_G._ = require 'lodash'

package.path = package.path .. ";../?.lua"

local LOG_INFO = 2
local LOG_DEBUG = 3
local LOG_ERROR = 1

describe('Time', function()
	local utils
	local utcT, localT
	local utcRaw, utcNow, utcPast
	local localRaw, localNow, localPast

	setup(function()
		_G.logLevel = 1
		_G.log = function()	end
		Time = require('Time')
	end)

	teardown(function()
		Time = nil
	end)

	before_each(function()

		utcNow = os.date('!*t', os.time())
		utcPast = os.date('!*t', os.time() - 300)
		utcRaw = tostring(utcPast.year) .. '-' ..
				tostring(utcPast.month) .. '-' ..
				tostring(utcPast.day) .. ' ' ..
				tostring(utcPast.hour) .. ':' ..
				tostring(utcPast.min) .. ':' ..
				tostring(utcPast.sec)
		utcT = Time(utcRaw, true)

		localNow = os.date('*t', os.time())
		localPast = os.date('*t', os.time() - 300)
		localRaw = tostring(localPast.year) .. '-' ..
				tostring(localPast.month) .. '-' ..
				tostring(localPast.day) .. ' ' ..
				tostring(localPast.hour) .. ':' ..
				tostring(localPast.min) .. ':' ..
				tostring(localPast.sec)
		localT = Time(localRaw, false)

	end)

	after_each(function()
		utcT = nil
		localT = nil
	end)


	it('should instantiate', function()
		assert.not_is_nil(utcT)
	end)

	describe('UTC', function()
		it('should have today', function()
			assert.is_same(utcT.current, localNow)
		end)

		it('hould have minutesAgo', function()
			assert.is_same(5, utcT.minutesAgo)
		end)

		it('should have secondsAgo', function()
			assert.is_same(300, utcT.secondsAgo)
		end)

		it('should have hoursAgo', function()
			local p = os.date('!*t', os.time() - 10800)
			local raw = tostring(p.year) .. '-' ..
					tostring(p.month) .. '-' ..
					tostring(p.day) .. ' ' ..
					tostring(p.hour) .. ':' ..
					tostring(p.min) .. ':' ..
					tostring(p.sec)
			local t = Time(raw, true)
			assert.is_same(3, t.hoursAgo)
		end)

		it('should have a raw time', function()
			assert.is_same(utcRaw, utcT.raw)
		end)

		it('should have isToday', function()
			assert.is_true(utcT.isToday)
		end)

		it('should have time properties', function()
			-- time should be converted to local time
			assert.is_same(localPast.year, utcT.year)
			assert.is_same(localPast.moth, utcT.mont)
			assert.is_same(localPast.day, utcT.day)
			assert.is_same(localPast.hour, utcT.hour)
			assert.is_same(localPast.min, utcT.min)
			assert.is_same(localPast.sec, utcT.sec)

			-- however utcTime holds the utc time
			assert.is_same(utcPast.year, utcT.utcTime.year)
			assert.is_same(utcPast.moth, utcT.utcTime.mont)
			assert.is_same(utcPast.day, utcT.utcTime.day)
			assert.is_same(utcPast.hour, utcT.utcTime.hour)
			assert.is_same(utcPast.min, utcT.utcTime.min)
			assert.is_same(utcPast.sec, utcT.utcTime.sec)

		end)

		it('should have a utc system time', function()
			assert.is_same(os.date('!*t'), utcT.utcSystemTime)
		end)
	end)

	describe('non UTC', function()
		it('should have today', function()
			assert.is_same(localT.current, localNow)
		end)

		it('hould have minutesAgo', function()
			assert.is_same(5, localT.minutesAgo)
		end)

		it('should have secondsAgo', function()
			assert.is_same(300, localT.secondsAgo)
		end)

		it('should have hoursAgo', function()
			local p = os.date('*t', os.time() - 10800)
			local raw = tostring(p.year) .. '-' ..
					tostring(p.month) .. '-' ..
					tostring(p.day) .. ' ' ..
					tostring(p.hour) .. ':' ..
					tostring(p.min) .. ':' ..
					tostring(p.sec)
			local t = Time(raw, false)
			assert.is_same(3, t.hoursAgo)
		end)

		it('should have a raw time', function()
			assert.is_same(localRaw, localT.raw)
		end)

		it('should have isToday', function()
			assert.is_true(localT.isToday)
		end)

		it('should have time properties', function()
			assert.is_same(localPast.year, localT.year)
			assert.is_same(localPast.moth, localT.mont)
			assert.is_same(localPast.day, localT.day)
			assert.is_same(localPast.hour, localT.hour)
			assert.is_same(localPast.min, localT.min)
			assert.is_same(localPast.sec, localT.sec)
		end)

		it('should return iso format', function()
			assert.is_same(os.date("!%Y-%m-%dT%TZ", os.time(localPast)), localT.getISO())
		end)
	end)

end)
