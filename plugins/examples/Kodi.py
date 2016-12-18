#           Kodi Plugin
#
#           Author:     Dnpwwo, 2016
#
"""
<plugin key="Kodi" name="Kodi Players" author="dnpwwo" version="1.0.1" wikilink="http://www.domoticz.com/wiki/plugins/Kodi.html" externallink="https://kodi.tv/">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="9090"/>
        <param field="Mode1" label="MAC Address" width="150px" required="false"/>
        <param field="Mode2" label="Shutdown Command" width="100px">
            <options>
                <option label="Hibernate" value="Hibernate"/>
                <option label="Suspend" value="Suspend"/>
                <option label="Shutdown" value="Shutdown"/>
                <option label="Ignore" value="Ignore" default="true" />
            </options>
        </param>
        <param field="Mode3" label="Notifications" width="75px">
            <options>
                <option label="True" value="True"/>
                <option label="False" value="False"  default="true" />
            </options>
        </param>
        <param field="Mode6" label="Debug" width="75px">
            <options>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
            </options>
        </param>
    </params>
</plugin>
"""
import Domoticz
import json

isConnected = False
nextConnect = 3
oustandingPings = 0
canShutdown = False
canSuspend = False
canHibernate = False

# player status
playerState = 0
playerID = -1
mediaLevel=0
mediaDescrption = ""
percentComplete = 0

# other
playlistName = ""
playlistPos = 0

# Domoticz call back functions
def onStart():
    global playerState, mediaLevel
    if Parameters["Mode6"] == "Debug":
        Domoticz.Debugging(1)
    if (len(Devices) == 0):
        Domoticz.Device(Name="Status",  Unit=1, Type=17,  Switchtype=17).Create()
        Domoticz.Device(Name="Source",  Unit=2, Type=244, Subtype=62, Switchtype=18, Image=12, Options="LevelActions:fHx8fA==;LevelNames:T2ZmfFZpZGVvfE11c2ljfFRWIFNob3dzfExpdmUgVFY=;LevelOffHidden:ZmFsc2U=;SelectorStyle:MA==").Create()
        Domoticz.Device(Name="Volume",  Unit=3, Type=244, Subtype=73, Switchtype=7,  Image=8).Create()
        Domoticz.Device(Name="Playing", Unit=4, Type=244, Subtype=73, Switchtype=7,  Image=12).Create()
        Domoticz.Log("Devices created.")
    else:
        if (1 in Devices): playerState = Devices[1].nValue
        if (2 in Devices): mediaLevel = Devices[2].nValue
    DumpConfigToLog()
    Domoticz.Transport("TCP/IP", Parameters["Address"], Parameters["Port"])
    Domoticz.Protocol("JSON")
    Domoticz.Heartbeat(10)
    Domoticz.Connect()
    return True

def onConnect(Status, Description):
    global isConnected, playerState
    if (Status == 0):
        isConnected = True
        Domoticz.Log("Connected successfully to: "+Parameters["Address"]+":"+Parameters["Port"])
        playerState = 1
        Domoticz.Send('{"jsonrpc":"2.0","method":"System.GetProperties","params":{"properties":["canhibernate","cansuspend","canshutdown"]},"id":1007}')
        Domoticz.Send('{"jsonrpc":"2.0","method":"Application.GetProperties","id":1011,"params":{"properties":["volume","muted"]}}')
        Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1001}')
    else:
        isConnected = False
        Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"])
        Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Parameters["Address"]+":"+Parameters["Port"]+" with error: "+Description)
        # Turn devices off in Domoticz
        for Key in Devices:
            UpdateDevice(Key, 0, Devices[Key].sValue)
    return True

def onMessage(Data):
    global oustandingPings
    global canShutdown, canSuspend, canHibernate
    global playerState, playerID, mediaLevel, mediaDescrption, percentComplete, playlistName, playlistPos
    
    Response = json.loads(Data)
    if ('error' in Response):
        # Kodi has signalled and error
        if (Response["id"] == 1010):
            Domoticz.Log("Addon execution request failed: "+Data)
        elif (Response["id"] == 2002):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":1,"item":{"directory":"special://profile/playlists/music/'+playlistName+'.xsp\", "media":"music"}},"id":2003}')
        elif (Response["id"] == 2003):
            Domoticz.Error("Playlist '"+playlistName+"' could not be added, probably does not exist.")
        else:
            Domoticz.Error("Unhandled error response: "+Data)
    elif ('id' not in Response):
        # Events do not have an 'id' because we didn't request them
        if (Response["method"] == "Application.OnVolumeChanged"):
            Domoticz.Debug("Application.OnVolumeChanged recieved.")
            if (Response["params"]["data"]["muted"] == True):
                UpdateDevice(3, 0, int(Response["params"]["data"]["volume"]))
            else:
                UpdateDevice(3, 2, int(Response["params"]["data"]["volume"]))
        elif (Response["method"] == "Player.OnStop") or(Response["method"] == "System.OnWake"):
            Domoticz.Debug("Player.OnStop recieved.")
            ClearDevices()
        elif (Response["method"] == "Player.OnPlay"):
            Domoticz.Debug("Player.OnPlay recieved, Player ID: "+str(playerID))
            playerID = Response["params"]["data"]["player"]["playerid"]
            if (Response["params"]["data"]["item"]["type"] == "picture"):
                playerState = 6
            elif (Response["params"]["data"]["item"]["type"] == "episode"):
                playerState = 4
            elif (Response["params"]["data"]["item"]["type"] == "channel"):
                playerState = 4
            elif (Response["params"]["data"]["item"]["type"] == "movie"):
                playerState = 4
            elif (Response["params"]["data"]["item"]["type"] == "song"):
                playerState = 5
            elif (Response["params"]["data"]["item"]["type"] == "musicvideo"):
                playerState = 4
            else:
                playerState = 9
            if (playerID != -1):
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
        elif (Response["method"] == "Player.OnPause"):
            Domoticz.Debug("Player.OnPause recieved, Player ID: "+str(playerID))
            playerState = 2
            SyncDevices()
        elif (Response["method"] == "Player.OnSeek"):
            if (playerID != -1):
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(playerID) + ',"properties":["live","percentage","speed"]}}')
            else:
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1005}')
            Domoticz.Debug("Player.OnSeek recieved, Player ID: "+str(playerID))
        elif (Response["method"] == "System.OnQuit") or (Response["method"] == "System.OnSleep") or (Response["method"] == "System.OnRestart"):
            Domoticz.Debug("System.OnQuit recieved.")
            ClearDevices()
        else:
            Domoticz.Debug("Unhandled unsolicited response: "+Data)
    else:
        Domoticz.Debug(str(Response["id"])+" response received: "+Data)
        # Responses to requests made by the plugin
        if (Response["id"] == 1001):    # PING call when nothing is playing
            if playerState == 0: playerState = 1
            oustandingPings = oustandingPings - 1
            if (len(Response["result"]) > 0) and ('playerid' in Response["result"][0]):
                playerID = Response["result"][0]["playerid"]
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
            else:
                ClearDevices()
        elif (Response["id"] == 1002):    # PING call when something is playing
            # {"id":1002,"jsonrpc":"2.0","result":{"live":false,"percentage":0.044549804180860519409,"speed":1}}
            oustandingPings = oustandingPings - 1
            if ("live" in Response["result"]) and (Response["result"]["live"] == True):
                mediaLevel = 40
            if (playerState == 2) and ("speed" in Response["result"]) and (Response["result"]["speed"] != 0):
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
            if ("speed" in Response["result"]) and (Response["result"]["speed"] == 0):
                playerState = 2
            if ("percentage" in Response["result"]):
                percentComplete = round(Response["result"]["percentage"])
        elif (Response["id"] == 1003):
            if ("item" in Response["result"]):
                playerState = 9
                mediaLevel = 0
                if ("type" in Response["result"]["item"]):
                    mediaType = Response["result"]["item"]["type"]
                if (mediaType == "song"):
                    playerState = 5
                    mediaLevel = 20
                elif (mediaType == "movie"):
                    playerState = 4
                    mediaLevel = 10
                elif (mediaType == "unknown"):
                    playerState = 4
                    mediaLevel = 10
                elif (mediaType == "episode"):
                    playerState = 4
                    mediaLevel = 30
                elif (mediaType == "channel"):
                    playerState = 4
                    mediaLevel = 40
                elif (mediaType == "picture"):
                    playerState = 6
                    mediaLevel = 50
                else:
                    Domoticz.Error( "Unknown media type encountered: "+str(mediaType))
                    playerState = 9

                mediaDescrption = ""
                if ("artist" in Response["result"]["item"]) and (len(Response["result"]["item"]["artist"])):
                    mediaDescrption = mediaDescrption + Response["result"]["item"]["artist"][0] + ", "
                if ("album" in Response["result"]["item"]) and (Response["result"]["item"]["album"] != ""):
                    mediaDescrption = mediaDescrption + "("+Response["result"]["item"]["album"] + ") "
                if ("showtitle" in Response["result"]["item"]) and (Response["result"]["item"]["showtitle"] != ""):
                    mediaDescrption = mediaDescrption + Response["result"]["item"]["showtitle"] + ", "
                mediaDescrption = mediaDescrption + ": "
                if ("season" in Response["result"]["item"]) and (Response["result"]["item"]["season"] > 0) and ("episode" in Response["result"]["item"]) and (Response["result"]["item"]["episode"] > 0):
                    mediaDescrption = mediaDescrption + "[S" + str(Response["result"]["item"]["season"]) + "E" + str(Response["result"]["item"]["episode"]) + "] "
                if ("title" in Response["result"]["item"]) and (Response["result"]["item"]["title"] != ""):
                    mediaDescrption = mediaDescrption + Response["result"]["item"]["title"] + ", "
                if ("channel" in Response["result"]["item"]) and (Response["result"]["item"]["channel"] != ""):
                    mediaDescrption = mediaDescrption + "("+Response["result"]["item"]["channel"] + ") "
                    mediaLevel = 30
                if ("label" in Response["result"]["item"]) and (Response["result"]["item"]["label"] != ""):
                    if (mediaDescrption.find(Response["result"]["item"]["label"]) == -1):
                        mediaDescrption = mediaDescrption + Response["result"]["item"]["label"] + ", "
                if ("year" in Response["result"]["item"]) and (Response["result"]["item"]["year"] > 0):
                    mediaDescrption = mediaDescrption + " (" + str(Response["result"]["item"]["year"]) + ")"

                # Now tidy up and compress the string
                mediaDescrption = mediaDescrption.lstrip(":")
                mediaDescrption = mediaDescrption.rstrip(", :")
                mediaDescrption = mediaDescrption.replace("  ", " ")
                mediaDescrption = mediaDescrption.replace(", :", ":")
                mediaDescrption = mediaDescrption.replace(", (", " (")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace(", ", ",")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace(" (", "(")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace(") ", ")")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace(": ", ":")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace(" [", "[")
                if (len(mediaDescrption) > 40): mediaDescrption = mediaDescrption.replace("] ", "]")
                mediaDescrption = mediaDescrption.replace(",(", "(")
                while (mediaDescrption.rfind(")") != -1) and (len(mediaDescrption) > 40):
                    mediaDescrption = mediaDescrption.replace(mediaDescrption[mediaDescrption.rfind("("):mediaDescrption.rfind(")")+1],"")
                
                if (mediaType != "picture"):
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(playerID) + ',"properties":["live","percentage","speed"]}}')
            else:
                ClearDevices()
        elif (Response["id"] == 1006):
            if (Response["result"] != "OK"):
                Domoticz.Error("Remote command unsuccessful, response: '"+str(Response["result"])+"'")
        elif (Response["id"] == 1007):
            canShutdown = Response["result"]["canshutdown"]
            canSuspend = Response["result"]["cansuspend"]
            canHibernate = Response["result"]["canhibernate"]
            Domoticz.Debug("Kodi shutdown options response processed.")
        elif (Response["id"] == 1008):
            if (Response["result"] == "OK"):
                Domoticz.Log("Shutdown command accepted.")
            else:
                Domoticz.Error("Shutdown command unsuccerssful response: '"+str(Response["result"])+"'")
        elif (Response["id"] == 1010):
            Domoticz.Log("Executed Addon successfully")
        elif (Response["id"] == 1011):
            if (Response["result"]["muted"] == False):
                UpdateDevice(3, 2, Response["result"]["volume"])
            else:
                UpdateDevice(3, 0, Response["result"]["volume"])
        elif (Response["id"] == 2000):
            if (Response["result"] != "OK"):
                Domoticz.Error("Playlist Clear command unsuccessful, response: '"+str(Response["result"])+"'")
        elif (Response["id"] == 2002):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"playlistid":0,"position":'+playlistPos+'}},"id":2004}')
        elif (Response["id"] == 2003):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"playlistid":1,"position":'+playlistPos+'}},"id":2004}')
        elif (Response["id"] == 2004):
            if (Response["result"] != "OK"):
                Domoticz.Error("Playlist Open command unsuccessful, response: '"+str(Response["result"])+"'")
        elif (Response["id"] == 2100):
            if ('favourites' in Response["result"]) and ('limits' in Response["result"]) and ('total' in Response["result"]["limits"]):
                iFavCount = Response["result"]["limits"]["total"]
                if (playlistPos < 0): playlistPos = 0
                if (playlistPos >= iFavCount): playlistPos = iFavCount - 1
                i = 0
                for Favourite in Response["result"]["favourites"]:
                    Domoticz.Debug("Favourites #"+str(i)+" is '"+Favourite["title"]+"', type '"+Favourite["type"]+"'.")
                    if (i == playlistPos):
                        if (Favourite["type"] == "media"):
                            Domoticz.Log("Favourites #"+str(i)+" has path '"+Favourite["path"]+"' and will be played.")
                            Domoticz.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"file":"'+Favourite["path"]+'"}},"id":2101}')
                            break
                        else:
                            Domoticz.Log("Requested Favourite ('"+Favourite["title"]+"') is not playable, next playable item will be selected.")
                            playlistPos = playlistPos + 1
                    i = i + 1
            else:
                Domoticz.Log("No Favourites returned.");
        elif (Response["id"] == 2101):
            Domoticz.Log( "2101: To be handled: "+Data)
        else:
            Domoticz.Debug("Unknown Response: "+Data)
    SyncDevices()
    return True

def onCommand(Unit, Command, Level, Hue):
    global isConnected
    global playerID, mediaLevel, playlistName, playlistPos

    Domoticz.Debug("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level) + ", Connected: " + str(isConnected))

    Command = Command.strip()
    action, sep, params = Command.partition(' ')
    action = action.capitalize()

    if (isConnected == False):
        TurnOn()
    else:
        if (action == 'On'):
            if (Unit == 3):  # Volume control
                Domoticz.Send('{"jsonrpc":"2.0","method":"Application.SetMute","params":{"mute":false}}')
            elif (Unit == 4):   # Position control
                if (playerID != -1):
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(playerID) + ',"play":true}}')
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
            else:   
                if (Parameters["Mode1"] == ""):
                    Domoticz.Log( "'On' command ignored, No MAC address configured.")
        elif (action == 'Set'):
            if (params.capitalize() == 'Level') or (Command.lower() == 'Volume'):
                if (Unit == 2):  # Source selector
                    mediaLevel = Level
                    # valid windows name list http://kodi.wiki/view/JSON-RPC_API/v6#GUI.Window
                    if (mediaLevel == 10): Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"music"}}')
                    if (mediaLevel == 20): Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"videos"}}')
                    if (mediaLevel == 30): Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"video"}}')
                    if (mediaLevel == 40): Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"pictures"}}')
                    if (mediaLevel == 50): Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"pvr"}}')
                    SyncDevices()
                elif (Unit == 3):   # Volume control
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Application.SetVolume","params":{"volume":' + str(Level) + '}}')
                elif (Unit == 4):   # Position control
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Player.Seek","params":{"playerid":' + str(playerID) + ',"value":'+str(Level)+'}}')
                else:
                    Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
        elif (action == 'Play') or (action == 'Pause'):
            if (playerID != -1):
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(playerID) + ',"play":false}}')
        elif (action == 'Stop'):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
        elif (action == 'Trigger'):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
            Domoticz.Send('{"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":0},"id":2000}')
            Domoticz.Send('{"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":1},"id":2000}')
            action, sep, params = params.partition(' ')
            action = action.capitalize()
            playlistName = ""
            playlistPos = 0
            if (action == 'Playlist'):
                # Command formats: 'Trigger Playlist ActionMovies',  'Trigger Playlist ActionMovies 15'
                playlistName, sep, params = params.partition(' ')
                if (params.isdigit()):
                    playlistPos = int(params)
                Domoticz.Send('{"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":0,"item":{"directory":"special://profile/playlists/music/'+playlistName+'.xsp\", "media":"music"}},"id":2002}')
            elif (action == 'Favorites') or (action == 'Favourites'):
                # Command formats: 'Trigger Favorites', 'Trigger Favorites 3'
                if (params.isdigit()):
                    playlistPos = int(params)
                Domoticz.Send('{"jsonrpc":"2.0","method":"Favourites.GetFavourites","params":{"properties":["path"]},"id":2100}')
            else:
                Domoticz.Error( "Trigger, Unknown target: "+str(action)+", expected Playlist/Favorites.")
        elif (action == 'Run'):
            Domoticz.Send('{"jsonrpc":"2.0","method":"Addons.ExecuteAddon","params":{"addonid":"' + params + '"},"id":1010}')
        elif (action == 'Off'):
            if (Unit == 1) or (Unit == 2):  # Source Selector or Status
                Domoticz.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
                Domoticz.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"home"}}')
                TurnOff()
            elif (Unit == 3):  # Volume control
                Domoticz.Send('{"jsonrpc":"2.0","method":"Application.SetMute","params":{"mute":true}}')
            elif (Unit == 4):   # Position control
                if (playerID != -1):
                    Domoticz.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(playerID) + ',"play":false}}')
            else:
                Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
        else:
            Domoticz.Error("Unhandled action '"+action+"' ignored, options are On/Set/Play/Pause/Stop/Trigger/Run/Off")

    return True

def onNotification(Data):
    Domoticz.Log("Notification: " + str(Data))
    return

def onHeartbeat():
    global isConnected, nextConnect, oustandingPings, playerID
    if (isConnected == True):
        if (oustandingPings > 6):
            Domoticz.Disconnect()
            nextConnect = 0
        else:
            if (playerID == -1):
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1001}')
            else:
                Domoticz.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(playerID) + ',"properties":["live","percentage","speed"]}}')
            oustandingPings = oustandingPings + 1
    else:
        # if not connected try and reconnected every 3 heartbeats
        oustandingPings = 0
        nextConnect = nextConnect - 1
        if (nextConnect <= 0):
            nextConnect = 3
            Domoticz.Connect()
    return True

def onDisconnect():
    global isConnected
    isConnected = False
    Domoticz.Log("Device has disconnected")
    for Key in Devices:
        UpdateDevice(Key, 0, Devices[Key].sValue)
    return

def onStop():
    Domoticz.Log("onStop called")
    return True

# Generic helper functions
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

# Device specific functions
def SyncDevices():
    global playerState, playerID, mediaLevel, mediaDescrption, percentComplete
    # Make sure that the Domoticz devices are in sync (by definition, the device is connected)
    if (1 in Devices):
        UpdateDevice(1, playerState, mediaDescrption)
    if (2 in Devices) and (Devices[2].nValue != mediaLevel):
        UpdateDevice(2, mediaLevel, str(mediaLevel))
    if (4 in Devices):
        if (playerState == 4) or (playerState == 5):
            UpdateDevice(4, 2, str(percentComplete))
        else:
            UpdateDevice(4, 0, str(percentComplete))
    return

def UpdateDevice(Unit, nValue, sValue):
    # Make sure that the Domoticz device still exists (they can be deleted) before updating it 
    if (Unit in Devices):
        if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue):
            Devices[Unit].Update(nValue, str(sValue))
            Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
    return

def ClearDevices():
    global playerState, playerID, mediaLevel, mediaDescrption, percentComplete
    # Stop everything and make sure things are synced
    playerID = -1
    playerState = 1
    mediaDescrption = ""
    percentComplete = 0
    SyncDevices()
    return

def TurnOn():
    if (Parameters["Mode1"] == ""):
        Domoticz.Error("Kodi can not be turned on, No MAC address configured.")
    else:
        import struct, socket
        # Sourced from: http://forum.kodi.tv/showthread.php?tid=3450&pid=16741#pid16741
        addr_byte = Parameters["Mode1"].split(':')      # MAC Address expected '00:01:03:d8:7b:76'
        hw_addr = struct.pack('bbbbbb', int(addr_byte[0],16),int(addr_byte[1],16),int(addr_byte[2],16),int(addr_byte[3],16),int(addr_byte[4],16),int(addr_byte[5],16))

        # build the wake-on-lan "magic packet"...
        msg = '\xff' * 6 + hw_addr * 16

        # ...and send it to the broadcast address using udp
        s = socket.socket(socket.af_inet, socket.sock_dgram)
        s.setsockopt(socket.sol_socket, socket.so_broadcast, 1)
        s.sendto(msg, ('<broadcast>', 7))
        s.close()
       
    return

def TurnOff():
    global canShutdown, canSuspend, canHibernate
    if (Parameters["Mode2"] == "Ignore"):
        Domoticz.Log("'Off' command ignored as configured.")
    elif (Parameters["Mode2"] == "Hibernate"):
        if (canHibernate == True):
            Domoticz.Send('{"jsonrpc":"2.0","method":"System.Hibernate","id":1008}')
        else:
            Domoticz.Error("Configured Shutdown option: 'Hibernate' not support by attached Kodi.")
    elif (Parameters["Mode2"] == "Suspend"):
        if (canHibernate == True):
            Domoticz.Send('{"jsonrpc":"2.0","method":"System.Suspend","id":1008}')
        else:
            Domoticz.Error("Configured Shutdown option: 'Suspend' not support by attached Kodi.")
    elif (Parameters["Mode2"] == "Shutdown"):
        if (canHibernate == True):
            Domoticz.Send('{"jsonrpc":"2.0","method":"System.Shutdown","id":1008}')
        else:
            Domoticz.Error("Configured Shutdown option: 'Shutdown' not support by attached Kodi.")
    else:
        Domoticz.Error("Unknown Shutdown option, ID:"+str(Parameters["Mode2"])+".")
    return
