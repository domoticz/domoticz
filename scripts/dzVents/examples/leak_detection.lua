-- Water leak detection
--
-- assumptions:
-- need 2 devices : 
-- a Water Flow devices name "Water_Flow"
-- a Dummy Device type percentage "Leakage_Percent"
--
-- 1 / leakage "open valve"
-- Every minute if a non-zero water flow is present, it increments a counter (Leakage_Percent)
-- If the water flow is zero is that it leaks more continuously.
-- A notification can be put on "Leakage_Percent" if >80% (80% = 80 minutes of continuous flow)
--
-- 2 / "micro continuous flow" (drip)
-- in 24 hours one must have at least 2 hours without flow (detection 0.5 liters / hour is 4.5m3 / year)
-- if not, "Leakage_Percent" is forced at 100%.

local FLOW_DEVICE = 'Water_Flow' -- Flow device
local LEAK_DEVICE = 'Leakage_Percent' -- percent dummy device


return {
    active = true,
    on = {
        ['timer'] = 'every minute'
    },
    data = {
		time_0_flow = { initial = 0 },
		total_time = { initial = 0 },
    },
    execute = function(domoticz)
    	-- Flow in liter/minute
        local flow = tonumber(domoticz.devices[FLOW_DEVICE].rawData[1])
        -- Dummy device in %
        local leakage = domoticz.devices[LEAK_DEVICE]
        local time_with_flow = tonumber(leakage.rawData[1])
        local new_time_with_flow = time_with_flow
        
-- 1 / leakage "open valve"
        if (flow > 0) then
           domoticz.data.time_0_flow = 0  -- there is a flow
           new_time_with_flow = new_time_with_flow + 1 -- time with flow
           if (new_time_with_flow > 100) then
             	new_time_with_flow = 100
           end
        else
           new_time_with_flow = 0
           domoticz.data.time_0_flow = domoticz.data.time_0_flow + 1 -- time without flow
        end
        
-- 2 / flight type "micro continuous flow" (drip)
        domoticz.data.total_time = domoticz.data.total_time + 1 -- time without since last 2 hours with no flow
        
        if (domoticz.data.time_0_flow > 120) then
            -- 2 hours with no flow
            domoticz.data.total_time = 0
        elseif (domoticz.data.total_time > (60*24)) then
        	-- 24 heures since last 2 hours with no flow
        	new_time_with_flow = 100
        end
-- log
        domoticz.log(new_time_with_flow .. ' minutes with flow ')
        domoticz.log(domoticz.data.time_0_flow .. ' minutes without flow ')
        domoticz.log(domoticz.data.total_time .. ' minutes without 2hrs without flow ')
 
 -- update dummy device %       
        if (time_with_flow ~= new_time_with_flow) then
         leakage.update(0,new_time_with_flow)
        end
    end
}
