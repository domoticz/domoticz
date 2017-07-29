# Basic Python Plugin Example
#
# Author: GizMoCuz
#
"""
<plugin key="OptPlug" name="Options PLugin" author="gizmocuz" version="1.0.0" wikilink="http://www.domoticz.com/wiki/plugins/plugin.html" externallink="https://www.google.com/">
    <params>
    </params>
</plugin>
"""
import Domoticz

class BasePlugin:
    enabled = False
    def __init__(self):
        return

    def onStart(self):
        if 2 in Devices:
            Devices[2].Delete()
        Options = {"LevelActions": "||||",
                  "LevelNames": "Off|Video|Music|TV Shows|Live TV",
                  "LevelOffHidden": "false",
                  "SelectorStyle": "1"}
        Domoticz.Device(Name="Source", Unit=2, TypeName="Selector Switch", Options=Options, Used=1).Create()
        Domoticz.Log("onStart called")
        Domoticz.Heartbeat(2)

    def onHeartbeat(self):
        Domoticz.Log("onHeartbeat called - "+str(len(Devices[2].Options["LevelNames"])))
        if (len(Devices[2].Options["LevelNames"]) < 80):
            dictOptions = Devices[2].Options
            dictOptions["LevelNames"] = dictOptions["LevelNames"]+'|[Test] '+str(len(Devices[2].Options["LevelNames"]))
            dictOptions["LevelActions"] = dictOptions["LevelActions"]+'|'
            Devices[2].Update(nValue=0, sValue="Test", Options=dictOptions)
        else:
            Domoticz.Heartbeat(10)
            setSelectorByName(2, '[Test] 62')

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
        Devices[2].Update(1 if Level > 0 else 0, str(Level))

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def setSelectorByName(intId, strName):
    dictOptions = Devices[intId].Options
    listLevelNames = dictOptions['LevelNames'].split('|')
    intLevel = 0
    for strLevelName in listLevelNames:
        if strLevelName == strName:
            Devices[intId].Update(1, str(intLevel))
        intLevel += 10
        
