# Multi-threaded example
#
# Starts a Queue on a thread to write log messages and shuts down properly
#
# Author: Dnpwwo, 2019
#
"""
<plugin key="MultiThread" name="Multi-threaded example" author="dnpwwo" version="1.0.0" >
    <description>
        <h2>Multi-threaded example</h2><br/>
        Starts a Queue on a thread to write log messages and shuts down properly<br/>
    </description>
    <params>
        <param field="Mode6" label="Debug" width="150px">
            <options>
                <option label="None" value="0"  default="true" />
                <option label="Python Only" value="2"/>
                <option label="Basic Debugging" value="62"/>
                <option label="Basic+Messages" value="126"/>
                <option label="Connections Only" value="16"/>
                <option label="Connections+Queue" value="144"/>
                <option label="All" value="-1"/>
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz
import sys,os
import threading
import queue

class BasePlugin:
    
    def __init__(self):
        self.messageQueue = queue.Queue()
        self.messageThread = threading.Thread(name="QueueThread", target=BasePlugin.handleMessage, args=(self,))

    def handleMessage(self):
        try:
            Domoticz.Debug("Entering message handler")
            while True:
                Message = self.messageQueue.get(block=True)
                if Message is None:
                    Domoticz.Debug("Exiting message handler")
                    self.messageQueue.task_done()
                    break

                if (Message["Type"] == "Log"):
                    Domoticz.Log("handleMessage: '"+Message["Text"]+"'.")
                elif (Message["Status"] == "Error"):
                    Domoticz.Status("handleMessage: '"+Message["Text"]+"'.")
                elif (Message["Type"] == "Error"):
                    Domoticz.Error("handleMessage: '"+Message["Text"]+"'.")
                self.messageQueue.task_done()
        except Exception as err:
            Domoticz.Error("handleMessage: "+str(err))
    
    def onStart(self):
        if Parameters["Mode6"] != "0":
            Domoticz.Debugging(int(Parameters["Mode6"]))
            DumpConfigToLog()
        self.messageThread.start()
        Domoticz.Heartbeat(2)

    def onHeartbeat(self):
        self.messageQueue.put({"Type":"Log", "Text":"Heartbeat test message"})

    def onStop(self):
        # Not needed in an actual plugin
        for thread in threading.enumerate():
            if (thread.name != threading.current_thread().name):
                Domoticz.Log("'"+thread.name+"' is running, it must be shutdown otherwise Domoticz will abort on plugin exit.")

        # signal queue thread to exit
        self.messageQueue.put(None)
        Domoticz.Log("Clearing message queue...")
        self.messageQueue.join()

        # Wait until queue thread has exited
        Domoticz.Log("Threads still active: "+str(threading.active_count())+", should be 1.")
        while (threading.active_count() > 1):
            for thread in threading.enumerate():
                if (thread.name != threading.current_thread().name):
                    Domoticz.Log("'"+thread.name+"' is still running, waiting otherwise Domoticz will abort on plugin exit.")
            time.sleep(1.0)

global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

# Generic helper functions
def stringOrBlank(input):
    if (input == None): return ""
    else: return str(input)

def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            Domoticz.Debug( "'" + x + "':'" + str(Parameters[x]) + "'")
    Domoticz.Debug("Device count: " + str(len(Devices)))
    for x in Devices:
        Domoticz.Debug("Device:           " + str(x) + " - " + str(Devices[x]))
        Domoticz.Debug("Device ID:       '" + str(Devices[x].ID) + "'")
        Domoticz.Debug("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Debug("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
    return
