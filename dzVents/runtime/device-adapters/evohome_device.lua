local TimedCommand = require('TimedCommand')

return {

	baseType = 'device',

	name = 'Evohome device adapter',

	matches = function (device, adapterManager)
		local res = (
			device.hardwareTypeValue == 39 or
			device.hardwareTypeValue == 40 or
			device.hardwareTypeValue == 106 or
			device.hardwareTypeValue == 75
		)

		if (not res) then
			adapterManager.addDummyMethod(device, 'updateSetPoint')
			adapterManager.addDummyMethod(device, 'setHotWater')
			adapterManager.addDummyMethod(device, 'setMode')
		end

		return res

	end,

	process = function (device, data, domoticz, utils, adapterManager)

		if (device.deviceSubType == "Hot Water" ) then

			if device.rawData[2] == "On" then device.state = "On" else device.state = "Off" end
			device.mode = tostring(device.rawData[3] or "n/a")
			device.untilDate = tostring(device.rawData[4] or "n/a")

			function device.setHotWater(state, mode, untilDate)
				 if mode == 'TemporaryOverride' and untilDate then
					mode = mode .. "&until=" .. untilDate
				 end
				local url = domoticz.settings['Domoticz url'] ..
					"/json.htm?type=setused&idx=" .. device.id ..
					"&setpoint=&state=" .. state ..
					"&mode=" .. mode ..
					"&used=true"
				return domoticz.openURL(url)
			end
		else
			device.state = device.rawData[2]
			device.setPoint = tonumber(device.rawData[1] or 0)
			device.mode = tostring(device.rawData[3])
			device.untilDate = tostring(device.rawData[4] or "n/a")

			function device.updateSetPoint(setPoint, mode, untilDate)
				return TimedCommand(domoticz,
					'SetSetPoint:' .. tostring(device.id),
					tostring(setPoint) .. '#' ..
					tostring(mode) .. '#' ..
					tostring(untilDate) , 'setpoint')
			end
			function device.setMode(mode, dParm, action, ooc)

				local function checkTimeAndReturnISO(tm)
					local now = domoticz.time
					local iso8601Pattern = "(%d+)-(%d+)-(%d+)T(%d+):(%d+):(%d+)Z"
					local iso8601Format = "%Y-%m-%dT%TZ"

					local function inFuture(tmISO)
						local function makeFutureISOTime(str, hours)
							local xyear, xmonth, xday, xhour, xminute, xseconds = str:match(iso8601Pattern)
							local seconds = os.time({year = xyear, month = xmonth, day = xday, hour = xhour, min = xminute, sec = xseconds})
							local offset = (seconds + ( hours or 0 ) *  3600)
							return os.date(iso8601Format,offset),  offset
						end

						local _, epoch = makeFutureISOTime(tmISO)
						return epoch >= now.dDate and tmISO
					end

					if type(tm) == 'string' and tm:find(iso8601Pattern) then return inFuture(tm) end                -- Something like '2016-04-29T06:32:58Z'
					if type(tm) == 'table' and tm.getISO then return inFuture(tm.getISO()) end                      -- a dzVents time object
					if type(tm) == 'table' and tm.day then return inFuture(os.date(iso8601Format,os.time(tm))) end  -- a standard time object
					if type(tm) == 'number' and tm > now.dDate then return inFuture(os.date(iso8601Format,tm)) end  -- seconds since epoch
					if type(tm) == 'number' and tm > 0 and tm < ( 365 * 24 * 60 ) then return inFuture(os.date(iso8601Format,now.dDate + ( tm * 60 ))) end  -- seconds since epoch + tm

					domoticz.log('dParm ' .. tostring(dParm) .. ' cannot be processed. (it will be ignored)',utils.LOG_ERROR)
					return false -- not a time as we know it
				end

				local function isValid(mode)
					for _, value in pairs(domoticz) do
						local res =  type(value) == 'string' and value == mode
						if res then return res end
					end
					return mode == 'Busted' or false 
				end

				local function binary(num, default)
					if num == 0 or num == 1 then return num else return default end
				end

				if isValid( tostring(mode) ) then -- is it in the list of valid modes ?
					local dParm = dParm and checkTimeAndReturnISO(dParm) -- if there is a dParm then check if valid and make a valid ISO date
					dParm = ( dParm and '&until=' .. dParm ) or '' 
					local action = ( action and '&action=' .. binary(action, 1) ) or '&action=1' 
					local ooc = ( ooc and '&ooc=' .. binary(ooc, 0) ) or '&ooc=0'
					
					local url = domoticz.settings['Domoticz url'] ..
								'/json.htm?type=command&param=switchmodal&idx=' .. device.id ..
								"&status=" .. mode ..
								dParm .. 
								action ..
								ooc
					return domoticz.openURL(url)
				else
					utils.log('Unknown status for setMode requested: ' .. tostring(mode),utils.LOG_ERROR)
				end
			end
		end
	end
}
