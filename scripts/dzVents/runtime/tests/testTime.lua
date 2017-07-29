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

		it('should have secondsSinceMidnight', function()
			local t = Time('2017-01-01 11:45:12')
			assert.is_same(42312, t.secondsSinceMidnight)
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

			describe('at daytime', function()

				it('should return true if it is day time', function()
					_G.timeofday = { ['Daytime'] = true }
					local t = Time('2017-01-01 00:00:00')
					assert.is_true(t.ruleIsAtDayTime('at daytime'))
				end)

				it('should return if it is not day time', function()
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

				it('should return if it is not day time', function()
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


			describe('on <days>', function()

				it('should return true when it is on the day', function()
					local t = Time('2017-06-05 02:04:00')
					assert.is_true(t.ruleIsOnDay('on mon'))
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

		end)

		describe('combis', function()

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
