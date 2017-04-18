import Domoticz

import DomoticzEvents as DE

#import reloader
#reloader.auto_reload(__name__)

# similar to http://www.domoticz.com/wiki/Smart_Lua_Scripts
# name is PIR <options> <switch controlled>
# options can be day/night/all
# example names:
# Pir day-night Slaapkamer groot
# PIR all Slaapkamer groot
# PIR night Slaapkamer groot

#DomoticzEvents.log(0, "Test")

# Domoticz.Log(0, "Testing")

DE.Log("Changed: " + DE.changed_device_name)

if DE.changed_device_name == "Test":
    #DE.Command(name="Test_Target", action="Off")
    DE.Command("Test_Target", "Off")
