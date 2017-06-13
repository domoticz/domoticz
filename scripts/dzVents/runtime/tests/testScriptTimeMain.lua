local _ = require 'lodash'
_G._ = require 'lodash'

package.path = package.path .. ";../?.lua"
package.path = package.path .. ";../../?.lua"  -- two folders up

describe('script_time_main', function()

	setup(function()
		_G.TESTMODE = true

		_G.timeofday = {
			Daytime = 'dt',
			Nighttime = 'nt',
			SunriseInMinutes = 'sunrisemin',
			SunsetInMinutes = 'sunsetmin'
		}

		_G.globalvariables = {
			Security = 'sec'
		}
		_G.devicechanged = {
			['onscript1'] = 'On',
		}
		_G.otherdevices = {
			['onscript1'] = 'On',
			['device1'] = 'On'
		}

		_G.otherdevices_lastupdate = {
			['onscript1'] = '2016-03-20 12:23:00',
		}
		_G.otherdevices_idx = {
			['onscript1'] = 1,
		}

		_G.otherdevices_svalues = {
			['onscript1'] = '1;2;3',
		}

		_G.uservariables = {}

		_G['uservariables_lastupdate'] = {}

	end)

	teardown(function()

	end)

	it("should do it's thing", function()
		_G.commandArray = {}
		local main = require('timer_scripts_dispatcher')
		assert.is_same({
			{["onscript1"]="Off"},
			{["SendNotification"]="Me#timer every minute#0#pushover"},
			{["Scene:scene 1"]="On"}}, main)
	end)

end)