# Test Plugin for Extended Settings
#
# This plugin exercises all new parameter types for testing purposes.
#
"""
<plugin key="TestExtSettings" name="Test Extended Settings Plugin" author="test" version="1.0.0">
    <description>
        <h2>Test Extended Settings</h2><br/>
        This plugin is for testing the extended plugin settings feature.
    </description>
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="100px" required="true" default="8080"/>
        <param field="Interval" type="number" label="Poll Interval (s)" min="5" max="3600" step="5" default="30" width="100px"/>
        <param field="EnableDebug" type="boolean" label="Debug Mode" default="false"/>
        <param field="Brightness" type="slider" label="Default Brightness" min="0" max="100" default="50" width="200px"/>
        <param field="Protocol" label="Protocol" width="150px">
            <options>
                <option label="HTTP" value="http" default="true"/>
                <option label="HTTPS" value="https"/>
            </options>
        </param>
        <param field="Certificate" label="Certificate Path" width="200px" visible_when="Protocol=https" default=""/>
        <param field="ApiKey" label="API Key" password="true" width="200px" default=""/>
        <group label="Advanced Settings">
            <param field="RetryCount" type="number" label="Retry Count" min="0" max="10" default="3" width="100px"/>
            <param field="Timeout" type="number" label="Timeout (s)" min="1" max="60" default="10" width="100px"/>
            <param field="EnableNotifications" type="boolean" label="Enable Notifications" default="true"/>
        </group>
        <param field="Mode6" label="Legacy Debug" width="150px">
            <options>
                <option label="None" value="0" default="true"/>
                <option label="All" value="-1"/>
            </options>
        </param>
    </params>
</plugin>
"""
import DomoticzEx as Domoticz

class BasePlugin:
    def __init__(self):
        return

    def onStart(self):
        Domoticz.Log("onStart called")
        Domoticz.Log("Plugin settings:")
        for key, value in Parameters.items():
            Domoticz.Log(f"  {key} = {value}")

    def onStop(self):
        Domoticz.Log("onStop called")

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()
