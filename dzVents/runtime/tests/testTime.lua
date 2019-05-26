_G._ = require 'lodash'

package.path = package.path .. ";../?.lua"

describe('Time', function()
	local utils, Time
	local utcT, localT
	local utcRaw, utcNow, utcPast
	local localRaw, localNow, localPast

	setup(function()
		_G.logLevel = 1
		_G.TESTMODE = true
		_G.log = function() end
		_G.globalvariables = {
			['currentTime'] = '2017-08-17 12:13:14.123'
		}
		Time = require('Time')
	end)

	teardown(function()
		Time = nil
	end)

	before_each(function()

		local ms = '.342'
		utcNow = os.date('!*t', os.time())
		utcPast = os.date('!*t', os.time() - 300)
		utcRaw = tostring(utcPast.year) .. '-' ..
				tostring(utcPast.month) .. '-' ..
				tostring(utcPast.day) .. ' ' ..
				tostring(utcPast.hour) .. ':' ..
				tostring(utcPast.min) .. ':' ..
				tostring(utcPast.sec) .. ms
		utcT = Time(utcRaw, true)

		localNow = os.date('*t', os.time())
		localPast = os.date('*t', os.time() - 300)
		localRaw = tostring(localPast.year) .. '-' ..
				tostring(localPast.month) .. '-' ..
				tostring(localPast.day) .. ' ' ..
				tostring(localPast.hour) .. ':' ..
				tostring(localPast.min) .. ':' ..
				tostring(localPast.sec) .. ms
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

		it('should have milliseconds', function()
			assert.is_same(342, utcT.milliSeconds)
		end)

		it('should have secondsSinceMidnight', function()
			local t = Time('2017-01-01 11:45:12')
			assert.is_same(42312, t.secondsSinceMidnight)
		end)

		it('should have a raw time', function()
			assert.is_same(utcRaw, utcT.raw)
		end)

		it('should have leading zeros in raw time', function()
			local t = Time('2017-01-01 01:02:03')
			assert.is_same('01:02:03', t.rawTime)
			assert.is_same('2017-01-01', t.rawDate)
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
			assert.is_same(localPast.min, utcT.minutes)
			assert.is_same(localPast.sec, utcT.sec)
			assert.is_same(localPast.sec, utcT.seconds)

			-- however utcTime holds the utc time
			assert.is_same(utcPast.year, utcT.utcTime.year)
			assert.is_same(utcPast.moth, utcT.utcTime.mont)
			assert.is_same(utcPast.day, utcT.utcTime.day)
			assert.is_same(utcPast.hour, utcT.utcTime.hour)
			assert.is_same(utcPast.min, utcT.utcTime.min)
			assert.is_same(utcPast.min, utcT.utcTime.minutes)
			assert.is_same(utcPast.sec, utcT.utcTime.sec)
			assert.is_same(utcPast.sec, utcT.utcTime.seconds)

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

		it('should have milliseconds', function()
			assert.is_same(342, localT.milliSeconds)
			assert.is_same(342, localT.milliseconds)
		end)

		it('should have week number', function()
			local t = Time('2017-06-05 02:04:00')
			assert.is_same(23, t.week)
			t = Time('2017-01-01 02:04:00')
			assert.is_same(52, t.week)
			t = Time('2016-01-01 02:04:00')
			assert.is_same(53, t.week)
		end)

		it('should have daysAgo', function()
			local p = os.date('*t', os.time() - 190800)
			local raw = tostring(p.year) .. '-' ..
					tostring(p.month) .. '-' ..
					tostring(p.day) .. ' ' ..
					tostring(p.hour) .. ':' ..
					tostring(p.min) .. ':' ..
					tostring(p.sec)
			local t = Time(raw, false)
			assert.is_same(2, t.daysAgo)
		end)

		it('should have msAgo', function()
			assert.is_same((300000 - 342 + 123), localT.msAgo)
			assert.is_same((300000 - 342 + 123), localT.millisecondsAgo)
		end)

		it('should have 0 seconds ago when ms < 1000', function()
			local p = os.date('*t', os.time() - 1) -- 1 second from 'now'
			local raw = tostring(p.year) .. '-' ..
					tostring(p.month) .. '-' ..
					tostring(p.day) .. ' ' ..
					tostring(p.hour) .. ':' ..
					tostring(p.min) .. ':' ..
					tostring(p.sec) .. '.500'
			local t = Time(raw, false, 0)
			assert.is_same(0, t.secondsAgo)
			assert.is_same(500, t.msAgo)
			assert.is_same(500, t.millisecondsAgo)
			assert.is_same(0, t.minutesAgo)
		end)

		it('should return a time diff', function()
			local t1d = os.date('*t', os.time())
			local t1raw = tostring(t1d.year) .. '-' ..
					tostring(t1d.month) .. '-' ..
					tostring(t1d.day) .. ' ' ..
					tostring(t1d.hour) .. ':' ..
					tostring(t1d.min) .. ':' ..
					tostring(t1d.sec) .. '.100'
			local t1 = Time(t1raw, false, 0)

			local t2d = os.date('*t', os.time() - 86400)
			local t2raw = tostring(t2d.year) .. '-' ..
					tostring(t2d.month) .. '-' ..
					tostring(t2d.day) .. ' ' ..
					tostring(t2d.hour) .. ':' ..
					tostring(t2d.min) .. ':' ..
					tostring(t2d.sec) .. '.0'
			local t2 = Time(t2raw, false, 0)

			local t3d = os.date('*t', os.time() - (86400 + 86400 + 60 + 25))
			local t3raw = tostring(t3d.year) .. '-' ..
					tostring(t3d.month) .. '-' ..
					tostring(t3d.day) .. ' ' ..
					tostring(t3d.hour) .. ':' ..
					tostring(t3d.min) .. ':' ..
					tostring(t3d.sec) .. '.0'
			local t3 = Time(t3raw, false, 0)

			local t4d = os.date('*t', os.time() - (86400 + 86400 + 60 + 25))
			local t4raw = tostring(t4d.year) .. '-' ..
					tostring(t4d.month) .. '-' ..
					tostring(t4d.day) .. ' ' ..
					tostring(t4d.hour) .. ':' ..
					tostring(t4d.min) .. ':' ..
					tostring(t4d.sec) .. '.500'
			local t4 = Time(t4raw, false)

			local tFutured = os.date('*t', os.time() + (86400))
			local tFutureraw = tostring(tFutured.year) .. '-' ..
					tostring(tFutured.month) .. '-' ..
					tostring(tFutured.day) .. ' ' ..
					tostring(tFutured.hour) .. ':' ..
					tostring(tFutured.min) .. ':' ..
					tostring(tFutured.sec) .. '.200'
			local tFuture = Time(tFutureraw, false)

			assert.is_same({
				["secs"] = 0,
				["seconds"] = 0,
				["hours"] = 0,
				["days"] = 0,
				["mins"] = 0,
				["minutes"] = 0,
				["ms"] = 0,
				["milliseconds"] = 0,
				["compare"] = 0
			}, t1.compare(t1))

			assert.is_same({
				["secs"] = 86400,
				["seconds"] = 86400,
				["hours"] = 24,
				["days"] = 1,
				["mins"] = 1440,
				["minutes"] = 1440,
				["ms"] = 86400000 + 100,
				["milliseconds"] = 86400000 + 100,
				["compare"] = -1
			}, t1.compare(t2))

			assert.is_same({
				["secs"] = 172885,
				["seconds"] = 172885,
				["hours"] = 48,
				["days"] = 2,
				["mins"] = 2881,
				["minutes"] = 2881,
				["ms"] = 172885000 + 100,
				["milliseconds"] = 172885000 + 100,
				["compare"] = -1
			}, t1.compare(t3))

			assert.is_same({
				["secs"] = 172885,
				["seconds"] = 172885,
				["hours"] = 48,
				["days"] = 2,
				["mins"] = 2881,
				["minutes"] = 2881,
				["compare"] = -1,
				["ms"] = 172885000 - 500 + 100, -- t4 is 500ms closer to t1
				["milliseconds"] = 172885000 - 500 + 100 -- t4 is 500ms closer to t1
			}, t1.compare(t4))

			assert.is_same({
				["secs"] = 86400,
				["seconds"] = 86400,
				["hours"] = 24,
				["days"] = 1,
				["mins"] = 1440,
				["minutes"] = 1440,
				["compare"] = 1,
				["ms"] = 86400100,
				["milliseconds"] = 86400100
			}, t1.compare(tFuture))

		end)

		it('should have time properties', function()
			assert.is_same(localPast.year, localT.year)
			assert.is_same(localPast.moth, localT.mont)
			assert.is_same(localPast.day, localT.day)
			assert.is_same(localPast.hour, localT.hour)
			assert.is_same(localPast.min, localT.min)
			assert.is_same(localPast.sec, localT.sec)
			assert.is_same(localPast.min, localT.minutes)
			assert.is_same(localPast.sec, localT.seconds)
		end)

		it('should return iso format', function()
			assert.is_same(os.date("!%Y-%m-%dT%TZ", os.time(localPast)), localT.getISO())
		end)

		it('should initialise to today', function()
			local now = os.date('*t')
			local t = Time()

			assert.is_same(now.year, t.year)
			assert.is_same(now.day, t.day)
			assert.is_same(now.month, t.month)

		end)

		it('should have negatives minutesAgo when time is in the future', function()

			local localFuture = os.date('*t', os.time() + 546564)

			local futureRaw =  tostring(localFuture.year) .. '-' ..
					tostring(localFuture.month) .. '-' ..
					tostring(localFuture.day) .. ' ' ..
					tostring(localFuture.hour) .. ':' ..
					tostring(localFuture.min) .. ':' ..
					tostring(localFuture.sec)
			local t = Time(futureRaw, false)

			assert.is_same(-9109, t.minutesAgo)
			assert.is_same(-546564, t.secondsAgo)
			assert.is_same(-151, t.hoursAgo)
			assert.is_same(-6, t.daysAgo)

		end)
	end)

	describe('rules', function()

		describe('rule processors', function()

			describe('at hh:mm-hh:mm', function()

				it('should return nil if there is no range set', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_nil(t.ruleMatchesTimeRange('blaba'))
				end)

				it('should return true when time is in range', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_true(t.ruleMatchesTimeRange('at 10:15-13:45'))
				end)

				it('should return false when time is not in range', function()
					local t = Time('2017-01-01 14:45:00')
					assert.is_false(t.ruleMatchesTimeRange('at 10:15-13:45'))
				end)

				it('should return false when range format has asterixes', function()
					local t = Time('2017-01-01 14:45:00')
					assert.is_false(t.ruleMatchesTimeRange('at 10:1*-13:45'))
				end)

				it('should return true if range spans multiple days', function()
					local t = Time('2017-01-01 08:00:00')
					assert.is_true(t.ruleMatchesTimeRange('at 10:00-09:00'))
				end)

				it('should return false when range spans multiple days and time is not in range', function()
					local t = Time('2017-01-01 9:15:00')
					assert.is_false(t.ruleMatchesTimeRange('at 10:00-09:00'))
				end)

				it('should detect the rule within a random string', function()
					local t = Time('2017-01-01 08:00:00')
					assert.is_true(t.ruleMatchesTimeRange('blab ablab ab at 10:00-09:00 blabjablabjabj'))
				end)

			end)

			describe('at hh:mm', function()

				it('should return nil if there is no valid rule', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_nil(t.ruleMatchesTime('blaba'))
				end)

				it('should return false if the time format is not correct but the rule is ok', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_false(t.ruleMatchesTime('at 1*:**'))
				end)

				it('should return true when time matches rule', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_true(t.ruleMatchesTime('at 11:45'))
					assert.is_true(t.ruleMatchesTime('at 11:45 something'))
					assert.is_true(t.ruleMatchesTime('blabla at 11:45 blablab'))

				end)

				it('should match hour wildcard', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_true(t.ruleMatchesTime('at *:45'))
					assert.is_false(t.ruleMatchesTime('at *:46'))
				end)

				it('should match minute wildcard', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_true(t.ruleMatchesTime('at 11:*'))
					assert.is_false(t.ruleMatchesTime('at 12:*'))
				end)

				it('should detect the rule withing a random string', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_true(t.ruleMatchesTime('some rondom at 11:45 stuff'))
				end)

			end)

			describe('every xx hours', function()

				it('should detect every hour', function()
					local t = Time('2017-01-01 01:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('every hour'))

					t = Time('2017-01-01 01:05:00')
					assert.is_false(t.ruleMatchesHourSpecification('every hour'))

					t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('every hour'))
				end)

				it('should detect every other hour', function()
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('every other hour'))

					t = Time('2017-01-01 01:00:00')
					assert.is_false(t.ruleMatchesHourSpecification('every other hour'))

					t = Time('2017-01-01 02:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('every other hour'))

					t = Time('2017-01-01 22:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('every other hour'))

					t = Time('2017-01-01 23:00:00')
					assert.is_false(t.ruleMatchesHourSpecification('every other hour'))
				end)

				it('should detect every xx hours', function()

					local t = Time('2017-01-01 00:00:00')
					local utils = t._getUtilsInstance()
					utils.print = function() end

					local hours = _.range(1, 23, 1)


					for i, h in pairs(hours) do

						local rule = 'every ' .. tostring(h) .. ' hours'
						local res = t.ruleMatchesHourSpecification(rule)
						if ( (24 / h) ~= math.floor(24 / h)) then
							assert.is_false(res)
						else
							assert.is_true(res)
						end
					end

				end)

				it('should return nil if there no matching rule', function()
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleMatchesHourSpecification('bablab'))
				end)

				it('should detect the rule within a random string', function()
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleMatchesHourSpecification('some random every other hour stuff'))
				end)

			end)

			describe('every xx minutes', function()

				it('should detect every minute', function()
					local t = Time('2017-01-01 01:00:00')
					assert.is_true(t.ruleMatchesMinuteSpecification('every minute'))

					t = Time('2017-01-01 01:00:01')
					assert.is_true(t.ruleMatchesMinuteSpecification('every minute'))

					t = Time('2017-01-01 00:00:02')
					assert.is_true(t.ruleMatchesMinuteSpecification('every minute'))
				end)

				it('should detect every other minute', function()
					local t = Time('2017-01-01 01:00:00')
					assert.is_true(t.ruleMatchesMinuteSpecification('every other minute'))

					t = Time('2017-01-01 01:01:00')
					assert.is_false(t.ruleMatchesMinuteSpecification('every other minute'))

					t = Time('2017-01-01 00:02:00')
					assert.is_true(t.ruleMatchesMinuteSpecification('every other minute'))

					t = Time('2017-01-01 00:03:00')
					assert.is_false(t.ruleMatchesMinuteSpecification('every other minute'))
				end)

				it('should detect every xx minutes', function()
					local t = Time('2017-01-01 00:00:00')
					local utils = t._getUtilsInstance()
					utils.print = function() end

					t = Time('2017-01-01 00:00:00')
					local minutes = _.range(1, 59, 1)

					for i, m in pairs(minutes) do

						local rule = 'every ' .. tostring(m) .. ' minutes'
						local res = t.ruleMatchesMinuteSpecification(rule)
						if ((60 / m) ~= math.floor(60 / m)) then
							assert.is_false(res)
						else
							assert.is_true(res)
						end
					end
				end)

				it('should return nil if there no matching rule', function()
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleMatchesMinuteSpecification('bablab'))
				end)

				it('should detect the rule within a random string', function()
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleMatchesMinuteSpecification('some random every other minute stuff'))
				end)

			end)

			describe('at civildaytime', function()
				it('should return true if it is civil day time', function()
					_G.timeofday = { ['Civildaytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtCivilDayTime('at civildaytime'))
				end)

				it('should return false if it is not civil day time', function()
					_G.timeofday = { ['Civildaytime'] = false }
					local t = Time('2017-01-01 00:00:00')
					assert.is_false(t.ruleIsAtCivilDayTime('at civildaytime'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['Civildaytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleIsAtCivilDayTime('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['Civildaytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtCivilDayTime('some random at civildaytime text'))
				end)
			end)

			describe('at civilnighttime', function()

				it('should return true if it is civil day time', function()
					_G.timeofday = { ['Civilnighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtCivilNight('at civilnighttime'))
				end)

				it('should return false if it is not civil day time', function()
					_G.timeofday = { ['Civilnighttime'] = false }
					local t = Time('2017-01-01 00:00:00')
					assert.is_false(t.ruleIsAtCivilNight('at civilnighttime'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['Civilnighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleIsAtCivilNight('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['Civilnighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtCivilNight('some random at civilnighttime text'))
				end)
			end)

			describe('at civiltwilightend', function()

				it('should return true if it is at civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtCivilTwilightEnd('at civiltwilightend'))
				end)

				it('should return if it is not at civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 63 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_false(t.ruleIsAtCivilTwilightEnd('at civiltwilightend'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAtCivilTwilightEnd('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtCivilTwilightEnd('some random at civiltwilightend text'))
				end)
			end)

			describe('xx minutes before civiltwilightend', function()

				it('should return true if it is xx minutes before civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeCivilTwilightEnd('2 minutes before civiltwilightend'))
				end)

				it('should return if it is more than 2 minutes before civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:01:00')
					assert.is_false(t.ruleIsBeforeCivilTwilightEnd('2 minutes before civiltwilightend'))
				end)

				it('should return if it is less than 2 minutes before civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:03:00')
					assert.is_false(t.ruleIsBeforeCivilTwilightEnd('2 minutes before civiltwilightend'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsBeforeCivilTwilightEnd('minutes before civiltwilightend'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeCivilTwilightEnd('some random 2 minutes before civiltwilightend text'))
				end)
			end)

			describe('xx minutes after civiltwilightend', function()

				it('should return true if it is xx minutes after civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterCivilTwilightEnd('2 minutes after civiltwilightend'))
				end)

				it('should return if it is more less 2 minutes after civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:05:00')
					assert.is_false(t.ruleIsAfterCivilTwilightEnd('2 minutes after civiltwilightend'))
				end)

				it('should return if it is more than 2 minutes after civiltwilightend', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:07:00')
					assert.is_false(t.ruleIsAfterCivilTwilightEnd('2 minutes after civiltwilightend'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAfterCivilTwilightEnd('minutes after civiltwilightend'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightEndInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterCivilTwilightEnd('some random 2 minutes after civiltwilightend text'))
				end)
			end)

			describe('at civiltwilightstart', function()

				it('should return true if it is at civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtCivilTwilightStart('at civiltwilightstart'))
				end)

				it('should return if it is not at civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 63 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_false(t.ruleIsAtCivilTwilightStart('at civiltwilightstart'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAtCivilTwilightStart('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtCivilTwilightStart('some random at civiltwilightstart text'))
				end)
			end)

			describe('xx minutes before civiltwilightstart', function()

				it('should return true if it is xx minutes before civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeCivilTwilightStart('2 minutes before civiltwilightstart'))
				end)

				it('should return if it is more than 2 minutes before civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:01:00')
					assert.is_false(t.ruleIsBeforeCivilTwilightStart('2 minutes before civiltwilightstart'))
				end)

				it('should return if it is less than 2 minutes before civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:03:00')
					assert.is_false(t.ruleIsBeforeCivilTwilightStart('2 minutes before civiltwilightstart'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsBeforeCivilTwilightStart('minutes before civiltwilightstart'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeCivilTwilightStart('some random 2 minutes before civiltwilightstart text'))
				end)
			end)

			describe('xx minutes after civiltwilightstart', function()

				it('should return true if it is xx minutes after civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterCivilTwilightStart('2 minutes after civiltwilightstart'))
				end)

				it('should return if it is more less 2 minutes after civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:05:00')
					assert.is_false(t.ruleIsAfterCivilTwilightStart('2 minutes after civiltwilightstart'))
				end)

				it('should return if it is more than 2 minutes after civiltwilightstart', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:07:00')
					assert.is_false(t.ruleIsAfterCivilTwilightStart('2 minutes after civiltwilightstart'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAfterCivilTwilightStart('minutes after civiltwilightstart'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['CivTwilightStartInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterCivilTwilightStart('some random 2 minutes after civiltwilightstart text'))
				end)
			end)

			describe('at daytime', function()

				it('should return true if it is day time', function()
					_G.timeofday = { ['Daytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtDayTime('at daytime'))
				end)

				it('should return false if it is not day time', function()
					_G.timeofday = { ['Daytime'] = false }
					local t = Time('2017-01-01 00:00:00')
					assert.is_false(t.ruleIsAtDayTime('at daytime'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['Daytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleIsAtDayTime('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['Daytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtDayTime('some random at daytime text'))
				end)

			end)

			describe('at nighttime', function()

				it('should return true if it is day time', function()
					_G.timeofday = { ['Nighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtNight('at nighttime'))
				end)

				it('should return false if it is not day time', function()
					_G.timeofday = { ['Nighttime'] = false }
					local t = Time('2017-01-01 00:00:00')
					assert.is_false(t.ruleIsAtNight('at nighttime'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['Nighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_nil(t.ruleIsAtNight('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['Nighttime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtNight('some random at nighttime text'))
				end)
			end)

			describe('at sunset', function()

				it('should return true if it is at sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtSunset('at sunset'))
				end)

				it('should return if it is not at sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 63 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_false(t.ruleIsAtSunset('at sunset'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAtSunset('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtSunset('some random at sunset text'))
				end)
			end)

			describe('xx minutes before sunset', function()

				it('should return true if it is xx minutes before sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeSunset('2 minutes before sunset'))
				end)

				it('should return if it is more than 2 minutes before sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:01:00')
					assert.is_false(t.ruleIsBeforeSunset('2 minutes before sunset'))
				end)

				it('should return if it is less than 2 minutes before sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:03:00')
					assert.is_false(t.ruleIsBeforeSunset('2 minutes before sunset'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsBeforeSunset('minutes before sunset'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeSunset('some random 2 minutes before sunset text'))
				end)
			end)

			describe('xx minutes after sunset', function()

				it('should return true if it is xx minutes after sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterSunset('2 minutes after sunset'))
				end)

				it('should return if it is more less 2 minutes after sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:05:00')
					assert.is_false(t.ruleIsAfterSunset('2 minutes after sunset'))
				end)

				it('should return if it is more than 2 minutes after sunset', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:07:00')
					assert.is_false(t.ruleIsAfterSunset('2 minutes after sunset'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAfterSunset('minutes after sunset'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunsetInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterSunset('some random 2 minutes after sunset text'))
				end)
			end)

			describe('at sunrise', function()

				it('should return true if it is at sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtSunrise('at sunrise'))
				end)

				it('should return if it is not at sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 63 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_false(t.ruleIsAtSunrise('at sunrise'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAtSunrise('at blabalbba'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_true(t.ruleIsAtSunrise('some random at sunrise text'))
				end)
			end)

			describe('xx minutes before sunrise', function()

				it('should return true if it is xx minutes before sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeSunrise('2 minutes before sunrise'))
				end)

				it('should return if it is more than 2 minutes before sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:01:00')
					assert.is_false(t.ruleIsBeforeSunrise('2 minutes before sunrise'))
				end)

				it('should return if it is less than 2 minutes before sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:03:00')
					assert.is_false(t.ruleIsBeforeSunrise('2 minutes before sunrise'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsBeforeSunrise('minutes before sunrise'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:02:00')
					assert.is_true(t.ruleIsBeforeSunrise('some random 2 minutes before sunrise text'))
				end)
			end)

			describe('xx minutes after sunrise', function()

				it('should return true if it is xx minutes after sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterSunrise('2 minutes after sunrise'))
				end)

				it('should return if it is more less 2 minutes after sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:05:00')
					assert.is_false(t.ruleIsAfterSunrise('2 minutes after sunrise'))
				end)

				it('should return if it is more than 2 minutes after sunrise', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:07:00')
					assert.is_false(t.ruleIsAfterSunrise('2 minutes after sunrise'))
				end)

				it('should return nil if the rule is not present', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:04:00')
					assert.is_nil(t.ruleIsAfterSunrise('minutes after sunrise'))
				end)

				it('should detect the rule within a random string', function()
					_G.timeofday = { ['SunriseInMinutes'] = 64 }
					local t = Time('2017-01-01 01:06:00')
					assert.is_true(t.ruleIsAfterSunrise('some random 2 minutes after sunrise text'))
				end)
			end)

			describe('between', function()

				it('should return nil if there is no range set', function()
					local t = Time('2017-01-01 11:45:00')
					assert.is_nil(t.ruleMatchesBetweenRange('blaba'))
				end)

				it('should detect the rule within a random string', function()
					local t = Time('2017-01-01 08:00:00')
					assert.is_true(t.ruleMatchesBetweenRange('blab ablab ab between 10:00 and 09:00 blabjablabjabj'))
				end)

				describe('time stamps', function()

					it('should work with time stamps', function()
						local t = Time('2017-01-01 11:45:00')
						assert.is_true(t.ruleMatchesBetweenRange('between 10:15 and 13:45'))
					end)

					it('should work with time stamps spanning two days', function()
						local t = Time('2017-01-01 23:01:00')
						assert.is_true(t.ruleMatchesBetweenRange('between 23:00 and 01:45'))
					end)

					it('should return false when range spans multiple days and time is not in range', function()
						local t = Time('2017-01-01 9:15:00')
						assert.is_false(t.ruleMatchesBetweenRange('between 10:00 and 09:00'))
					end)



				end)

				describe('twilight stuff', function()

					it('between civiltwilightend and civiltwilightstart', function()
						_G.timeofday = {
							['CivTwilightStartInMinutes'] = 360 , -- 06:00
							['CivTwilightEndInMinutes'] = 1080
						}

						-- time between 18:00 and 06:00
						local t = Time('2017-01-01 01:04:00')
						assert.is_true(t.ruleMatchesBetweenRange('between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 17:00:00')
						assert.is_false(t.ruleMatchesBetweenRange('between civiltwilightend and civiltwilightstart'))
					end)

					it('every x minute between civiltwilightend and civiltwilightstart', function()
						_G.timeofday = {
							['CivTwilightStartInMinutes'] = 360 , -- 06:00
							['CivTwilightEndInMinutes'] = 1080
						}

						-- time between 18:00 and 06:00
						t = Time('2017-01-01 01:01:00')
						assert.is_false(t.matchesRule('every 2 minutes between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 01:01:00')
						assert.is_true(t.matchesRule('every 1 minutes between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 01:02:00')
						assert.is_true(t.matchesRule('every 2 minutes between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 17:00:00')
						assert.is_false(t.matchesRule('every 2 minutes between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 17:01:00')
						assert.is_false(t.matchesRule('every 2 minutes between civiltwilightend and civiltwilightstart'))

						t = Time('2017-01-01 17:01:00')
						assert.is_false(t.matchesRule('every 1 minutes between civiltwilightend and civiltwilightstart'))
					end)

					it('between civiltwilightstart and civiltwilightend', function()

						_G.timeofday = {
							['CivTwilightStartInMinutes'] = 360, -- 06:00
							['CivTwilightEndInMinutes'] = 1080
						}

						-- time between 06:00 and 18:00
						local t = Time('2017-01-01 11:04:00')

						assert.is_true(t.ruleMatchesBetweenRange('between civiltwilightstart and civiltwilightend'))
					end)

					it('between 10 minutes before civiltwilightstart and 10 minutes after civiltwilightend', function()

						_G.timeofday = {
							['CivTwilightStartInMinutes'] = 360, -- 06:00
							['CivTwilightEndInMinutes'] = 1080 -- 18:00
						}

						local rule = 'between 10 minutes before civiltwilightstart and 10 minutes after civiltwilightend'
						-- time between 06:00 and 18:00
						local t = Time('2017-01-01 05:55:00')

						assert.is_true(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 18:06:00')

						assert.is_true(t.ruleMatchesBetweenRange(rule))
					end)
				end)

				describe('sun stuff', function()

					it('between sunset and sunrise', function()
						_G.timeofday = {
							['SunriseInMinutes'] = 360 , -- 06:00
							['SunsetInMinutes'] = 1080
						}

						-- time between 18:00 and 06:00
						local t = Time('2017-01-01 01:04:00')
						assert.is_true(t.ruleMatchesBetweenRange('between sunset and sunrise'))

						t = Time('2017-01-01 17:00:00')
						assert.is_false(t.ruleMatchesBetweenRange('between sunset and sunrise'))
					end)

					it('every x minute between sunset and sunrise', function()
						_G.timeofday = {
							['SunriseInMinutes'] = 360 , -- 06:00
							['SunsetInMinutes'] = 1080
						}

						-- time between 18:00 and 06:00
						t = Time('2017-01-01 01:01:00')
						assert.is_false(t.matchesRule('every 2 minutes between sunset and sunrise'))

						t = Time('2017-01-01 01:01:00')
						assert.is_true(t.matchesRule('every 1 minutes between sunset and sunrise'))

						t = Time('2017-01-01 01:02:00')
						assert.is_true(t.matchesRule('every 2 minutes between sunset and sunrise'))

						t = Time('2017-01-01 17:00:00')
						assert.is_false(t.matchesRule('every 2 minutes between sunset and sunrise'))

						t = Time('2017-01-01 17:01:00')
						assert.is_false(t.matchesRule('every 2 minutes between sunset and sunrise'))

						t = Time('2017-01-01 17:01:00')
						assert.is_false(t.matchesRule('every 1 minutes between sunset and sunrise'))
					end)

					it('between sunrise and sunset', function()

						_G.timeofday = {
							['SunriseInMinutes'] = 360, -- 06:00
							['SunsetInMinutes'] = 1080
						}

						-- time between 06:00 and 18:00
						local t = Time('2017-01-01 11:04:00')

						assert.is_true(t.ruleMatchesBetweenRange('between sunrise and sunset'))
					end)

					it('between 10 minutes before sunrise and 10 minutes after sunset', function()

						_G.timeofday = {
							['SunriseInMinutes'] = 360, -- 06:00
							['SunsetInMinutes'] = 1080 -- 18:00
						}

						local rule = 'between 10 minutes before sunrise and 10 minutes after sunset'
						-- time between 06:00 and 18:00
						local t = Time('2017-01-01 05:55:00')

						assert.is_true(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 18:06:00')

						assert.is_true(t.ruleMatchesBetweenRange(rule))
					end)
				end)


				describe('combined', function()

					it('between 23:12 and sunrise', function()
						_G.timeofday = {
							['SunriseInMinutes'] = 360, -- 06:00
							['SunsetInMinutes'] = 1080
						}

						local rule = 'between 23:12 and sunrise'

						local t = Time('2017-01-01 22:00:00')
						assert.is_false(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 23:12:00')
						assert.is_true(t.ruleMatchesBetweenRange(rule))
					end)

					it('between sunset and 22:33', function()
						_G.timeofday = {
							['SunriseInMinutes'] = 360, -- 06:00
							['SunsetInMinutes'] = 1080
						}

						local rule = 'between sunset and 22:33'
						local t = Time('2017-01-01 17:00:00')
						assert.is_false(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 18:10:00')
						assert.is_true(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 22:34:00')
						assert.is_false(t.ruleMatchesBetweenRange(rule))
					end)

					it('between 2 minutes after sunset and 22:33', function()
						_G.timeofday = {
							['SunriseInMinutes'] = 360, -- 06:00
							['SunsetInMinutes'] = 1080
						}

						local rule = 'between 2 minutes after sunset and 22:33'

						local t = Time('2017-01-01 18:00:00')
						assert.is_false(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 18:02:00')
						assert.is_true(t.ruleMatchesBetweenRange(rule))

						local t = Time('2017-01-01 22:34:00')
						assert.is_false(t.ruleMatchesBetweenRange(rule))
					end)

				end)

			end)

			describe('on <days>', function()

				it('should return true when it is on monday', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDay('on mon'))
				end)

				it('should return true when it is on tuesday', function()
					local t = Time('2017-06-06 02:04:00')
					assert.is_true(t.ruleIsOnDay('on tue'))
				end)

				it('should return true when it is on wednesday', function()
					local t = Time('2017-06-07 02:04:00')
					assert.is_true(t.ruleIsOnDay('on wed'))
				end)

				it('should return true when it is on thursday', function()
					local t = Time('2017-06-08 02:04:00')
					assert.is_true(t.ruleIsOnDay('on thu'))
				end)

				it('should return true when it is on friday', function()
					local t = Time('2017-06-09 02:04:00')
					assert.is_true(t.ruleIsOnDay('on fri'))
				end)

				it('should return true when it is on saturday', function()
					local t = Time('2017-06-10 02:04:00')
					assert.is_true(t.ruleIsOnDay('on sat'))
				end)

				it('should return true when it is on sunday', function()
					local t = Time('2017-06-11 02:04:00')
					assert.is_true(t.ruleIsOnDay('on sun'))
				end)

				it('should return true when it is on the days', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDay('on sun, mon,tue, fri'))
				end)

				it('should return false if it is not on the day', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_false(t.ruleIsOnDay('on tue'))
				end)

				it('should return nil if the rule is not there', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_nil(t.ruleIsOnDay('balbalbalb'))
				end)

				it('should detect the rule within random text', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDay('something balbalba on sun, mon ,tue, fri boebhebalb'))
				end)

			end)

			describe('in week', function()

				it('should return true when matches simple list of weeks', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_true(t.ruleIsInWeek('in week 23'))
					assert.is_true(t.ruleIsInWeek('in week 1,43,33,0,23'))
				end)

				it('should return nil if rule is not there', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_nil(t.ruleIsInWeek('iek 23'))
				end)

				it('should return true when matches odd weeks', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_true(t.ruleIsInWeek('every odd week'))
					assert.is_false(t.ruleIsInWeek('every even week'))
				end)

				it('should return true when matches even weeks', function()
					local t = Time('2017-06-13 02:04:00') -- week 24
					assert.is_false(t.ruleIsInWeek('every odd week'))
					assert.is_true(t.ruleIsInWeek('every even week'))
				end)

				it('should return false if week is not in rule', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_false(t.ruleIsInWeek('in week 2,4,5'))
				end)

				it('should return nil when no weeks are provided', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_nil(t.ruleIsInWeek('in week'))
				end)

				it('should return true when week is in range', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_true(t.ruleIsInWeek('in week -23'))
					assert.is_true(t.ruleIsInWeek('in week 23-'))
					assert.is_true(t.ruleIsInWeek('in week 3,55,-23,6,53'))
					assert.is_true(t.ruleIsInWeek('in week 6,7,8,23-,22,66'))

					assert.is_true(t.ruleIsInWeek('in week 12-25'))
					assert.is_true(t.ruleIsInWeek('in week 12-25,55,6-11'))

				end)

				it('should return false when not in range', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_false(t.ruleIsInWeek('in week 25-'))
					assert.is_false(t.ruleIsInWeek('in week 25-66'))
				end)

			end)

			describe('on date', function()

				it('should return true when on date', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 5/6'))
					assert.is_true(t.ruleIsOnDate('on 1/01-2/2,31/12,5/6,1/1'))

					t = Time('2018-01-2 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 2/1'))
					assert.is_true(t.ruleIsOnDate('on 02/1'))
					assert.is_true(t.ruleIsOnDate('on 2/01'))
					assert.is_true(t.ruleIsOnDate('on 02/01'))
				end)

				it('should return true when */mm', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDate('on */6'))
					assert.is_true(t.ruleIsOnDate('on 12/12, 4/5,*/6,*/8'))
				end)

				it('should return true when dd/*', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 5/*'))
					assert.is_true(t.ruleIsOnDate('on 12/12, 4/5,5/*,*/8'))
				end)

				it('should return true when in range', function()
					local t = Time('2017-06-20 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 20/5-22/6'))

					local t = Time('2017-05-20 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 20/5-22/6'))

					t = Time('2017-05-21 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 20/5-22/6'))

					t = Time('2017-06-20 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 20/5-22/6'))

					t = Time('2017-06-22 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 1/2, 20/5-22/6, 1/11-2/11'))

					t = Time('2017-06-22 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 1/2, 22/6-23/6, 1/11-2/11'))

					t = Time('2017-06-22 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 1/2, -23/6, 1/11-2/11'))
					t = Time('2017-06-22 02:04:00')
					assert.is_true(t.ruleIsOnDate('on 1/2, 20/6-, 1/11-2/11'))
				end)

				it('should return false when not in range', function()
					local t = Time('2017-06-20 02:04:00')
					assert.is_false(t.ruleIsOnDate('on 1/2, 20/5-22/5, 1/11-2/11'))
				end)

				it('should return nil if rule is not there', function()
					local t = Time('2017-06-05 02:04:00') -- week 23
					assert.is_nil(t.ruleIsOnDate('iek 23'))
				end)
			end)

		end)

		describe('combis', function()

			it('should return false when not on every second sunday between 1:00 and 1:30', function()
				local t = Time('2018-12-30 01:04:00') -- on Sunday, odd week at 01:04 
				assert.is_false(t.matchesRule('between 1:00 and 1:30 on sun every odd week'))
				assert.is_false(t.matchesRule('between 2:00 and 2:30 on sun every odd week'))
				assert.is_false(t.matchesRule('between 1:00 and 1:30 on sat every even week'))
				assert.is_false(t.matchesRule('between 2:00 and 2:30 on sat every even week'))
				assert.is_true(t.matchesRule('between 1:00 and 1:30 on sun every even week'))
			end)

			it('should return false when not on the day', function()

				local t = Time('2017-06-05 02:04:00') -- on monday

				assert.is_false(t.matchesRule('every minute on tue'))


			end)

			it('at sunset on mon', function()
				_G.timeofday = { ['SunsetInMinutes'] = 64 }
				local t = Time('2017-06-05 01:04:00') -- on monday
				assert.is_true(t.matchesRule('at sunset on mon'))
			end)

			it('2 minutes before sunset on mon', function()
				_G.timeofday = { ['SunsetInMinutes'] = 64 }
				local t = Time('2017-06-05 01:02:00') -- on monday
				assert.is_true(t.matchesRule('2 minutes before sunset on mon'))
			end)

			it('at sunrise on mon', function()
				_G.timeofday = { ['SunriseInMinutes'] = 64 }
				local t = Time('2017-06-05 01:04:00') -- on monday
				assert.is_true(t.matchesRule('at sunrise on mon'))
			end)

			it('at nighttime on mon', function()
				_G.timeofday = {['Nighttime'] = true }
				local t = Time('2017-06-05 01:04:00') -- on monday
				assert.is_true(t.matchesRule('at nighttime on mon'))
			end)

			it('at nighttime on mon', function()
				_G.timeofday = { ['Daytime'] = true }
				local t = Time('2017-06-05 01:04:00') -- on monday
				assert.is_true(t.matchesRule('at daytime on mon'))
			end)

			it('at daytime every 3 minutes on mon', function()
				_G.timeofday = { ['Daytime'] = true }
				local t = Time('2017-06-05 01:03:00') -- on monday
				assert.is_true(t.matchesRule('at daytime every 3 minutes on mon'))
				assert.is_false(t.matchesRule('at daytime every 7 minutes on mon'))
			end)

			it('at daytime every other hour on mon', function()
				_G.timeofday = { ['Daytime'] = true }
				local t = Time('2017-06-05 02:00:00') -- on monday
				assert.is_true(t.matchesRule('at daytime every other hour on mon'))
			end)

			it('at daytime at 15:* on mon', function()
				_G.timeofday = { ['Daytime'] = true }
				local t = Time('2017-06-05 15:22:00') -- on monday
				assert.is_true(t.matchesRule('at daytime at 15:* on mon'))
			end)

			it('at daytime every 5 minutes at 15:00-16:00 on mon', function()
				_G.timeofday = { ['Daytime'] = true }
				local t = Time('2017-06-05 16:00:00') -- on monday
				assert.is_true(t.matchesRule('at daytime every 5 minutes at 15:00-16:00 on mon'))
				assert.is_true(t.matchesRule('at daytime every 5 minutes at 15:00-16:00 on mon'))
				assert.is_true(t.matchesRule('at daytime every 5 minutes at 15:00-16:00 on mon'))
			end)

			it('in week 47 on mon', function()
				local t = Time('2017-11-20 16:00:00') -- on monday
				assert.is_true(t.matchesRule('in week 47 on mon'))

				t = Time('2017-11-21 16:00:00') -- on monday
				assert.is_false(t.matchesRule('in week 47 on mon'))
			end)

			it('in week 40-50 on mon', function()
				local t = Time('2017-11-20 16:00:00') -- on monday, wk47
				assert.is_true(t.matchesRule('in week 40-50 on mon'))
				assert.is_false(t.matchesRule('in week 1 on mon'))
			end)

			it('on date 20/11', function()
				local t = Time('2017-11-20 16:00:00') -- on monday, wk47
				assert.is_true(t.matchesRule('on 20/11'))
				assert.is_true(t.matchesRule('on 20/10-20/12 on mon'))
				assert.is_false(t.matchesRule('on 20/11-22/11 on fri'))

				assert.is_true(t.matchesRule('on 20/* on mon'))

			end)

			it('on date 20/10-31/10', function()
				local t = Time('2018-10-19 16:00:00') -- on monday, wk43
				assert.is_false(t.matchesRule('on 20/10-31/10'))
			end)

			it('at 08:00-15:00 on 21/4-30/4', function()
				local t = Time('2017-04-21 08:04:00')
				assert.is_true(t.matchesRule('at 08:00-15:00 on 21/4-30/4'))

				t = Time('2017-04-21 07:04:00')
				assert.is_false(t.matchesRule('at 08:00-15:00 on 21/4-30/4'))

			end)
			
			for fromMonth=1,12 do
				for toMonth=math.min(fromMonth+1,12),12 do
					it('at 08:00-23:00 on 01/' .. fromMonth .. '-30/' .. toMonth, function()
						local t = Time()
						if t.dDate > Time(t.year .. '-' .. fromMonth ..'-01 00:00:01').dDate and
						   t.dDate < Time(t.year .. '-' .. toMonth ..'-30 23:59:59').dDate then
							assert.is_true(t.matchesRule('at 00:30-23:30 on 01/' .. fromMonth .. '-30/' .. toMonth)) 
						else
							assert.is_false(t.matchesRule('at 00:30-23:30 on 01/' .. fromMonth .. '-30/' .. toMonth)) 
						end						
					end)
				end
			end

			for fromMonth=1,12 do
				for toMonth=math.min(fromMonth+1,12),12 do
					it('at 08:00-23:00 on */' .. fromMonth .. '-*/' .. toMonth, function()
						local t = Time()
						if  t.dDate > Time(t.year .. '-' .. fromMonth ..'-01 00:00:01').dDate and
							t.dDate < Time(t.year .. '-' .. toMonth ..'-30 23:59:59').dDate then
							assert.is_true(t.matchesRule('at 00:30-23:30 on */' .. fromMonth .. '-*/' .. toMonth)) 
						else
							assert.is_false(t.matchesRule('at 00:30-23:30 on */' .. fromMonth .. '-*/' .. toMonth)) 
						end						
					end)
				end
			end
			
			it('every 3 minutes on -15/4,15/10-', function()
				local t = Time('2017-04-18 11:24:00')
				assert.is_false(t.matchesRule('every 3 minutes on -15/4,15/10-'))

				t = Time('2017-04-15 11:24:00')
				assert.is_true(t.matchesRule('every 3 minutes on -15/4,15/10-'))

				t = Time('2017-10-15 11:24:00')
				assert.is_true(t.matchesRule('every 3 minutes on -15/4,15/10-'))

				t = Time('2017-10-14 11:24:00')
				assert.is_false(t.matchesRule('every 3 minutes on -15/4,15/10-'))

				t = Time('2017-10-15 11:25:00')
				assert.is_false(t.matchesRule('every 3 minutes on -15/4,15/10-'))
			end)

			it('every 10 minutes between 2 minutes after sunset and 22:33 on mon,fri', function()
				_G.timeofday = {
					['SunriseInMinutes'] = 360, -- 06:00
					['SunsetInMinutes'] = 1080
				}

				local rule = 'every 10 minutes between 2 minutes after sunset and 22:33 on mon,fri'

				local t = Time('2017-06-05 18:10:00') -- on monday
				assert.is_true(t.matchesRule(rule))
				local t = Time('2017-06-05 18:09:00')
				assert.is_false(t.matchesRule(rule)) -- not every 10 minutes
				local t = Time('2017-06-08 18:10:00') -- on thu
				assert.is_false(t.matchesRule(rule))
				local t = Time('2017-06-09 18:10:00') -- on fri
				assert.is_true(t.matchesRule(rule))

				local t = Time('2017-06-09 22:34:00') -- on fri
				assert.is_false(t.matchesRule(rule))
			end)

			it('every 10 minutes between 2 minutes after sunset and 22:33 on 20/11-20/12 in week 49 on mon,fri', function()
				_G.timeofday = {
					['SunriseInMinutes'] = 360, -- 06:00
					['SunsetInMinutes'] = 1080 -- 18:00
				}

				local rule = 'every 10 minutes between 2 minutes after sunset and 22:33 on 20/11-20/12 in week 49 on mon,fri'

				local t = Time('2017-11-21 18:10:00') -- on tue, week 47
				assert.is_false(t.matchesRule(rule))

				t = Time('2017-11-24 18:10:00') -- on fri, week 47
				assert.is_false(t.matchesRule(rule)) -- should fail because t is in week 47

				local t = Time('2017-12-08 18:10:00') -- on fri, week 49
				assert.is_true(t.matchesRule(rule))

				t = Time('2017-12-04 18:10:00') -- on mon, week 49
				assert.is_true(t.matchesRule(rule))

			end)

			it('at nighttime at 21:32-05:44 every 5 minutes', function()
				_G.timeofday = { ['Nighttime'] = true }
				local t = Time('2017-06-05 01:05:00') -- on monday
				assert.is_true(t.matchesRule('at nighttime at 21:32-05:44 every 5 minutes'))
			end)

			it('should return false if the rule is empty', function()
				local t = Time('2017-06-05 16:00:00') -- on monday
				assert.is_false(t.matchesRule(''))
			end)

			it('should return false if no processor matches', function()
				local t = Time('2017-06-05 16:00:00') -- on monday
				assert.is_false(t.matchesRule('boe bahb ladsfak'))
			end)


		end)


	end)

end)
