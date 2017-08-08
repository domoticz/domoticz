#           Kodi Plugin
#
#           Author:     Dnpwwo, 2016 - 2017
#
"""
<plugin key="Kodi" name="Kodi Players" author="dnpwwo" version="1.8.0" wikilink="http://www.domoticz.com/wiki/plugins/Kodi.html" externallink="https://kodi.tv/">
    <params>
        <param field="Address" label="IP Address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Port" width="30px" required="true" default="9090"/>
        <param field="Mode1" label="Icon" width="100px">
            <options>
                <option label="Blue" value="Kodi" default="true" />
                <option label="Black" value="KodiBlack"/>
                <option label="Round" value="KodiRound"/>
            </options>
        </param>
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
        <param field="Mode4" label="Notifier Name" width="100px" default="KodiNotify"/>
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
import sys
import json

class BasePlugin:
    KodiConn = None
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

    def onStart(self):
        if Parameters["Mode6"] == "Debug":
            Domoticz.Debugging(1)
            DumpConfigToLog()
        
        if ('Kodi'  not in Images): Domoticz.Image('Kodi Icons.zip').Create()
        if ('KodiBlack' not in Images): Domoticz.Image('KodiBlack Icons.zip').Create()
        if ('KodiRound' not in Images): Domoticz.Image('KodiRound Icons.zip').Create()

        if (len(Devices) == 0):
            Domoticz.Device(Name="Status",  Unit=1, Type=17,  Switchtype=17).Create()
            Options = {"LevelActions": "||||", 
                       "LevelNames": "Off|Video|Music|TV Shows|Live TV",
                       "LevelOffHidden": "false",
                       "SelectorStyle": "1"}
            Domoticz.Device(Name="Source",  Unit=2, TypeName="Selector Switch", Switchtype=18, Image=12, Options=Options).Create()
            Domoticz.Device(Name="Volume",  Unit=3, Type=244, Subtype=73, Switchtype=7, Image=8).Create()
            Domoticz.Device(Name="Playing", Unit=4, Type=244, Subtype=73, Switchtype=7, Image=12).Create()
            Domoticz.Log("Devices created.")
        if (1 in Devices):
            UpdateImage(1)
            self.playerState = Devices[1].nValue
        if (2 in Devices):
            UpdateImage(2)
            self.mediaLevel = Devices[2].nValue
        if (4 in Devices):
            UpdateImage(4)
            self.mediaLevel = Devices[2].nValue
        self.KodiConn = Domoticz.Connection(Name="KodiConn", Transport="TCP/IP", Protocol="JSON", Address=Parameters["Address"], Port=Parameters["Port"])
        self.KodiConn.Connect()
        if (Parameters["Mode3"] == "True"):
            Domoticz.Notifier(Parameters["Mode4"])
        Domoticz.Heartbeat(10)
        return True

    def onConnect(self, Connection, Status, Description):
        if (Status == 0):
            Domoticz.Log("Connected successfully to: "+Connection.Address+":"+Connection.Port)
            self.playerState = 1
            self.KodiConn.Send('{"jsonrpc":"2.0","method":"System.GetProperties","params":{"properties":["canhibernate","cansuspend","canshutdown"]},"id":1007}')
            self.KodiConn.Send('{"jsonrpc":"2.0","method":"Application.GetProperties","id":1011,"params":{"properties":["volume","muted"]}}')
            self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1001}')
        else:
            Domoticz.Log("Failed to connect ("+str(Status)+") to: "+Connection.Address+":"+Connection.Port)
            Domoticz.Debug("Failed to connect ("+str(Status)+") to: "+Connection.Address+":"+Connection.Port+" with error: "+Description)
            # Turn devices off in Domoticz
            for Key in Devices:
                UpdateDevice(Key, 0, Devices[Key].sValue)
        return True

    def onMessage(self, Connection, Data):
        strData = Data.decode("utf-8", "ignore")
        Response = json.loads(strData)
        if ('error' in Response):
            # Kodi has signalled and error
            if (Response["id"] == 1010):
                Domoticz.Log("Addon execution request failed: "+Data)
            elif (Response["id"] == 2002):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":1,"item":{"directory":"special://profile/playlists/music/'+self.playlistName+'.xsp\", "media":"music"}},"id":2003}')
            elif (Response["id"] == 2003):
                Domoticz.Error("Playlist '"+self.playlistName+"' could not be added, probably does not exist.")
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
                self.ClearDevices()
            elif (Response["method"] == "Player.OnPlay"):
                Domoticz.Debug("Player.OnPlay recieved, Player ID: "+str(self.playerID))
                self.playerID = Response["params"]["data"]["player"]["playerid"]
                if (Response["params"]["data"]["item"]["type"] == "picture"):
                    self.playerState = 6
                elif (Response["params"]["data"]["item"]["type"] == "episode"):
                    self.playerState = 4
                elif (Response["params"]["data"]["item"]["type"] == "channel"):
                    self.playerState = 4
                elif (Response["params"]["data"]["item"]["type"] == "movie"):
                    self.playerState = 4
                elif (Response["params"]["data"]["item"]["type"] == "song"):
                    self.playerState = 5
                elif (Response["params"]["data"]["item"]["type"] == "musicvideo"):
                    self.playerState = 4
                else:
                    self.playerState = 9
                if (self.playerID != -1):
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(self.playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
            elif (Response["method"] == "Player.OnPause"):
                Domoticz.Debug("Player.OnPause recieved, Player ID: "+str(self.playerID))
                self.playerState = 2
                self.SyncDevices()
            elif (Response["method"] == "Player.OnSeek"):
                if (self.playerID != -1):
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(self.playerID) + ',"properties":["live","percentage","speed"]}}')
                else:
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1005}')
                Domoticz.Debug("Player.OnSeek recieved, Player ID: "+str(self.playerID))
            elif (Response["method"] == "System.OnQuit") or (Response["method"] == "System.OnSleep") or (Response["method"] == "System.OnRestart"):
                Domoticz.Debug("System.OnQuit recieved.")
                self.ClearDevices()
            else:
                Domoticz.Debug("Unhandled unsolicited response: "+strData)
        else:
            Domoticz.Debug(str(Response["id"])+" response received: "+strData)
            # Responses to requests made by the plugin
            if (Response["id"] == 1001):    # PING call when nothing is playing
                if self.playerState == 0: self.playerState = 1
                self.oustandingPings = self.oustandingPings - 1
                if (len(Response["result"]) > 0) and ('playerid' in Response["result"][0]):
                    self.playerID = Response["result"][0]["playerid"]
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(self.playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
                else:
                    self.ClearDevices()
            elif (Response["id"] == 1002):    # PING call when something is playing
                # {"id":1002,"jsonrpc":"2.0","result":{"live":false,"percentage":0.044549804180860519409,"speed":1}}
                self.oustandingPings = self.oustandingPings - 1
                if ("live" in Response["result"]) and (Response["result"]["live"] == True):
                    self.mediaLevel = 40
                if (self.playerState == 2) and ("speed" in Response["result"]) and (Response["result"]["speed"] != 0):
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(self.playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
                if ("speed" in Response["result"]) and (Response["result"]["speed"] == 0):
                    self.playerState = 2
                if ("percentage" in Response["result"]):
                    self.percentComplete = round(Response["result"]["percentage"])
            elif (Response["id"] == 1003):
                if ("item" in Response["result"]):
                    self.playerState = 9
                    self.mediaLevel = 0
                    if ("type" in Response["result"]["item"]):
                        mediaType = Response["result"]["item"]["type"]
                    if (mediaType == "song"):
                        self.playerState = 5
                        self.mediaLevel = 20
                    elif (mediaType == "movie"):
                        self.playerState = 4
                        self.mediaLevel = 10
                    elif (mediaType == "unknown"):
                        self.playerState = 4
                        self.mediaLevel = 10
                    elif (mediaType == "episode"):
                        self.playerState = 4
                        self.mediaLevel = 30
                    elif (mediaType == "channel"):
                        self.playerState = 4
                        self.mediaLevel = 40
                    elif (mediaType == "picture"):
                        self.playerState = 6
                        self.mediaLevel = 50
                    else:
                        Domoticz.Error( "Unknown media type encountered: "+str(mediaType))
                        self.playerState = 9

                    self.mediaDescrption = ""
                    if ("artist" in Response["result"]["item"]) and (len(Response["result"]["item"]["artist"])):
                        self.mediaDescrption = self.mediaDescrption + Response["result"]["item"]["artist"][0] + ", "
                    if ("album" in Response["result"]["item"]) and (Response["result"]["item"]["album"] != ""):
                        self.mediaDescrption = self.mediaDescrption + "("+Response["result"]["item"]["album"] + ") "
                    if ("showtitle" in Response["result"]["item"]) and (Response["result"]["item"]["showtitle"] != ""):
                        self.mediaDescrption = self.mediaDescrption + Response["result"]["item"]["showtitle"] + ", "
                    self.mediaDescrption = self.mediaDescrption + ": "
                    if ("season" in Response["result"]["item"]) and (Response["result"]["item"]["season"] > 0) and ("episode" in Response["result"]["item"]) and (Response["result"]["item"]["episode"] > 0):
                        self.mediaDescrption = self.mediaDescrption + "[S" + str(Response["result"]["item"]["season"]) + "E" + str(Response["result"]["item"]["episode"]) + "] "
                    if ("title" in Response["result"]["item"]) and (Response["result"]["item"]["title"] != ""):
                        self.mediaDescrption = self.mediaDescrption + Response["result"]["item"]["title"] + ", "
                    if ("channel" in Response["result"]["item"]) and (Response["result"]["item"]["channel"] != ""):
                        self.mediaDescrption = self.mediaDescrption + "("+Response["result"]["item"]["channel"] + ") "
                        self.mediaLevel = 30
                    if ("label" in Response["result"]["item"]) and (Response["result"]["item"]["label"] != ""):
                        if (self.mediaDescrption.find(Response["result"]["item"]["label"]) == -1):
                            self.mediaDescrption = self.mediaDescrption + Response["result"]["item"]["label"] + ", "
                    if ("year" in Response["result"]["item"]) and (Response["result"]["item"]["year"] > 0):
                        self.mediaDescrption = self.mediaDescrption + " (" + str(Response["result"]["item"]["year"]) + ")"

                    # Now tidy up and compress the string
                    self.mediaDescrption = self.mediaDescrption.lstrip(":")
                    self.mediaDescrption = self.mediaDescrption.rstrip(", :")
                    self.mediaDescrption = self.mediaDescrption.replace("  ", " ")
                    self.mediaDescrption = self.mediaDescrption.replace(", :", ":")
                    self.mediaDescrption = self.mediaDescrption.replace(", (", " (")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace(", ", ",")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace(" (", "(")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace(") ", ")")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace(": ", ":")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace(" [", "[")
                    if (len(self.mediaDescrption) > 40): self.mediaDescrption = self.mediaDescrption.replace("] ", "]")
                    self.mediaDescrption = self.mediaDescrption.replace(",(", "(")
                    while (self.mediaDescrption.rfind(")") != -1) and (len(self.mediaDescrption) > 40):
                        self.mediaDescrption = self.mediaDescrption.replace(self.mediaDescrption[self.mediaDescrption.rfind("("):self.mediaDescrption.rfind(")")+1],"")
                    
                    if (mediaType != "picture"):
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(self.playerID) + ',"properties":["live","percentage","speed"]}}')
                else:
                    self.ClearDevices()
            elif (Response["id"] == 1006):
                if (Response["result"] != "OK"):
                    Domoticz.Error("Remote command unsuccessful, response: '"+str(Response["result"])+"'")
            elif (Response["id"] == 1007):
                self.canShutdown = Response["result"]["canshutdown"]
                self.canSuspend = Response["result"]["cansuspend"]
                self.canHibernate = Response["result"]["canhibernate"]
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
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"playlistid":0,"position":'+self.playlistPos+'}},"id":2004}')
            elif (Response["id"] == 2003):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"playlistid":1,"position":'+self.playlistPos+'}},"id":2004}')
            elif (Response["id"] == 2004):
                if (Response["result"] != "OK"):
                    Domoticz.Error("Playlist Open command unsuccessful, response: '"+str(Response["result"])+"'")
            elif (Response["id"] == 2100):
                if ('favourites' in Response["result"]) and ('limits' in Response["result"]) and ('total' in Response["result"]["limits"]):
                    iFavCount = Response["result"]["limits"]["total"]
                    if (self.playlistPos < 0): self.playlistPos = 0
                    if (self.playlistPos >= iFavCount): self.playlistPos = iFavCount - 1
                    i = 0
                    for Favourite in Response["result"]["favourites"]:
                        Domoticz.Debug("Favourites #"+str(i)+" is '"+Favourite["title"]+"', type '"+Favourite["type"]+"'.")
                        if (i == self.playlistPos):
                            if (Favourite["type"] == "media"):
                                Domoticz.Log("Favourites #"+str(i)+" has path '"+Favourite["path"]+"' and will be played.")
                                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"file":"'+Favourite["path"]+'"}},"id":2101}')
                                break
                            else:
                                Domoticz.Log("Requested Favourite ('"+Favourite["title"]+"') is not playable, next playable item will be selected.")
                                self.playlistPos = self.playlistPos + 1
                        i = i + 1
                else:
                    Domoticz.Log("No Favourites returned.");
            elif (Response["id"] == 2101):
                Domoticz.Log( "2101: To be handled: "+strData)
            else:
                Domoticz.Debug("Unknown Response: "+strData)
        self.SyncDevices()
        return True

    def onCommand(self, Unit, Command, Level, Hue):
        Domoticz.Log("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level) + ", Connected: " + str(self.KodiConn.Connected()))

        Command = Command.strip()
        action, sep, params = Command.partition(' ')
        action = action.capitalize()

        if (self.KodiConn.Connected() == False):
            self.TurnOn()
        else:
            if (action == 'On'):
                if (Unit == 3):  # Volume control
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Application.SetMute","params":{"mute":false}}')
                elif (Unit == 4):   # Position control
                    if (self.playerID != -1):
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(self.playerID) + ',"play":true}}')
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetItem","id":1003,"params":{"playerid":' + str(self.playerID) + ',"properties":["artist","album","year","channel","showtitle","season","episode","title"]}}')
                else:   
                    if (Parameters["Mode1"] == ""):
                        Domoticz.Log( "'On' command ignored, No MAC address configured.")
            elif (action == 'Set'):
                if (params.capitalize() == 'Level') or (Command.lower() == 'Volume'):
                    if (Unit == 2):  # Source selector
                        self.mediaLevel = Level
                        # valid windows name list http://kodi.wiki/view/JSON-RPC_API/v6#GUI.Window
                        if (self.mediaLevel == 10): self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"music"}}')
                        if (self.mediaLevel == 20): self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"videos"}}')
                        if (self.mediaLevel == 30): self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"video"}}')
                        if (self.mediaLevel == 40): self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"pictures"}}')
                        if (self.mediaLevel == 50): self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"pvr"}}')
                        self.SyncDevices()
                    elif (Unit == 3):   # Volume control
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Application.SetVolume","params":{"volume":' + str(Level) + '}}')
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Application.SetMute","params":{"mute":false}}')
                    elif (Unit == 4):   # Position control
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.Seek","params":{"playerid":' + str(self.playerID) + ',"value":'+str(Level)+'}}')
                    else:
                        Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
            elif (action == 'Play') or (action == 'Pause'):
                if (self.playerID != -1):
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(self.playerID) + ',"play":false}}')
            elif (action == 'Stop'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
            elif (action == 'Trigger'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":0},"id":2000}')
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Playlist.Clear","params":{"playlistid":1},"id":2000}')
                action, sep, params = params.partition(' ')
                action = action.capitalize()
                self.playlistName = ""
                self.playlistPos = 0
                if (action == 'Playlist'):
                    # Command formats: 'Trigger Playlist ActionMovies',  'Trigger Playlist ActionMovies 15'
                    self.playlistName, sep, params = params.partition(' ')
                    if (params.isdigit()):
                        self.playlistPos = int(params)
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Playlist.Add","params":{"playlistid":0,"item":{"directory":"special://profile/playlists/music/'+self.playlistName+'.xsp\", "media":"music"}},"id":2002}')
                elif (action == 'Favorites') or (action == 'Favourites'):
                    # Command formats: 'Trigger Favorites', 'Trigger Favorites 3'
                    if (params.isdigit()):
                        self.playlistPos = int(params)
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Favourites.GetFavourites","params":{"properties":["path"]},"id":2100}')
                else:
                    Domoticz.Error( "Trigger, Unknown target: "+str(action)+", expected Playlist/Favorites.")
            elif (action == 'Run'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Addons.ExecuteAddon","params":{"addonid":"' + params + '"},"id":1010}')
            elif (action == 'Off'):
                if (Unit == 1) or (Unit == 2):  # Source Selector or Status
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"stop"},"id":1006}')
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"GUI.ActivateWindow","params":{"window":"home"}}')
                    self.TurnOff()
                elif (Unit == 3):  # Volume control
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Application.SetMute","params":{"mute":true}}')
                elif (Unit == 4):   # Position control
                    if (self.playerID != -1):
                        self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.PlayPause","params":{"playerid":' + str(self.playerID) + ',"play":false}}')
                else:
                    Domoticz.Error( "Unknown Unit number in command "+str(Unit)+".")
            elif (action == 'Home'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.Home","params":{},"id":1006}')
            elif (action == 'Up'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.Up","params":{},"id":1006}')
            elif (action == 'Down'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.Down","params":{},"id":1006}')
            elif (action == 'Left'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.Left","params":{},"id":1006}')
            elif (action == 'Right'):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.Right","params":{},"id":1006}')
            else:
                # unknown so just assume its a keypress
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"Input.ExecuteAction","params":{"action":"'+action.lower()+'"},"id":1006}')

        return True

    def onNotification(self, Name, Subject, Text, Status, Priority, Sound, ImageFile):
        Domoticz.Log("Notification: " + Name + "," + Subject + "," + Text + "," + Status + "," + str(Priority) + "," + Sound + "," + ImageFile)
        if Parameters["Mode3"] == "True":
            import xmbcclient3
            packet = xmbcclient3.PacketNOTIFICATION(title=str(Name),  message=str(Text), icon_type=xmbcclient3.ICON_PNG, icon_file=str(ImageFile))
            udpBcastConn = Domoticz.Connection(Name="UDP Broadcast Connection", Transport="UDP/IP", Protocol="None", Address=Parameters["Address"], Port=str(9777))
            for a in range ( 0, packet.num_packets() ):
                try:
                    udpBcastConn.Send(packet.get_udp_message(a+1))
                except:
                    return

    def onHeartbeat(self):
        if (self.KodiConn.Connected()):
            if (self.oustandingPings > 6):
                self.KodiConn.Disconnect()
                self.nextConnect = 0
            else:
                if (self.playerID == -1):
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetActivePlayers","id":1001}')
                else:
                    self.KodiConn.Send('{"jsonrpc":"2.0","method":"Player.GetProperties","id":1002,"params":{"playerid":' + str(self.playerID) + ',"properties":["live","percentage","speed"]}}')
                self.oustandingPings = self.oustandingPings + 1
        else:
            # if not connected try and reconnected every 3 heartbeats
            self.oustandingPings = 0
            self.nextConnect = self.nextConnect - 1
            if (self.nextConnect <= 0):
                self.nextConnect = 3
                self.KodiConn.Connect()
        return True

    def onDisconnect(self, Connection):
        Domoticz.Log("Device has disconnected")
        return

    def onStop(self):
        Domoticz.Log("onStop called")
        return True

    def TurnOn(self):
        if (Parameters["Mode1"] == ""):
            Domoticz.Error("Kodi can not be turned on, No MAC address configured.")
        else:
            import struct, socket
            # Took some parts of:https://github.com/bentasker/Wake-On-Lan-Python/blob/master/wol.py for python3 support
            mac_address = Parameters["Mode1"].replace(':','')
            # Pad the synchronization stream.
            data = ''.join(['FFFFFFFFFFFF', mac_address * 20])
            send_data = b''
            # Split up the hex values and pack.
            for i in range(0, len(data), 2):
                send_data = b''.join([send_data, struct.pack('B', int(data[i: i + 2], 16))])
               
            # Broadcast it to the LAN.
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.sendto(send_data, ('<broadcast>', 7))
           
        return

    def TurnOff(self):
        if (Parameters["Mode2"] == "Ignore"):
            Domoticz.Log("'Off' command ignored as configured.")
        elif (Parameters["Mode2"] == "Hibernate"):
            if (self.canHibernate == True):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"System.Hibernate","id":1008}')
            else:
                Domoticz.Error("Configured Shutdown option: 'Hibernate' not support by attached Kodi.")
        elif (Parameters["Mode2"] == "Suspend"):
            if (self.canSuspend == True):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"System.Suspend","id":1008}')
            else:
                Domoticz.Error("Configured Shutdown option: 'Suspend' not support by attached Kodi.")
        elif (Parameters["Mode2"] == "Shutdown"):
            if (self.canShutdown == True):
                self.KodiConn.Send('{"jsonrpc":"2.0","method":"System.Shutdown","id":1008}')
            else:
                Domoticz.Error("Configured Shutdown option: 'Shutdown' not support by attached Kodi.")
        else:
            Domoticz.Error("Unknown Shutdown option, ID:"+str(Parameters["Mode2"])+".")
        return

    def SyncDevices(self):
        # Make sure that the Domoticz devices are in sync (by definition, the device is connected)
        if (1 in Devices):
            UpdateDevice(1, self.playerState, self.mediaDescrption)
        if (2 in Devices) and (Devices[2].nValue != self.mediaLevel):
            UpdateDevice(2, self.mediaLevel, str(self.mediaLevel))
        if (4 in Devices):
            if (self.playerState == 4) or (self.playerState == 5):
                UpdateDevice(4, 2, str(self.percentComplete))
            else:
                UpdateDevice(4, 0, str(self.percentComplete))
        return

    def ClearDevices(self):
        # Stop everything and make sure things are synced
        self.playerID = -1
        self.playerState = 1
        self.mediaDescrption = ""
        self.percentComplete = 0
        self.SyncDevices()
        return
        
global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile):
    global _plugin
    _plugin.onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

# Generic helper functions
def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            Domoticz.Debug( "'" + x + "':'" + str(Parameters[x]) + "'")
    Domoticz.Debug("Settings count: " + str(len(Settings)))
    for x in Settings:
        Domoticz.Debug( "'" + x + "':'" + str(Settings[x]) + "'")
    Domoticz.Debug("Image count: " + str(len(Images)))
    for x in Images:
        Domoticz.Debug( "'" + x + "':'" + str(Images[x]) + "'")
    Domoticz.Debug("Device count: " + str(len(Devices)))
    for x in Devices:
        Domoticz.Debug("Device:           " + str(x) + " - " + str(Devices[x]))
        Domoticz.Debug("Device ID:       '" + str(Devices[x].ID) + "'")
        Domoticz.Debug("Device Name:     '" + Devices[x].Name + "'")
        Domoticz.Debug("Device nValue:    " + str(Devices[x].nValue))
        Domoticz.Debug("Device sValue:   '" + Devices[x].sValue + "'")
        Domoticz.Debug("Device LastLevel: " + str(Devices[x].LastLevel))
        Domoticz.Debug("Device Image:     " + str(Devices[x].Image))
    return
 
def UpdateDevice(Unit, nValue, sValue):
    # Make sure that the Domoticz device still exists (they can be deleted) before updating it 
    if (Unit in Devices):
        if (Devices[Unit].nValue != nValue) or (Devices[Unit].sValue != sValue):
            Devices[Unit].Update(nValue=nValue, sValue=str(sValue))
            Domoticz.Log("Update "+str(nValue)+":'"+str(sValue)+"' ("+Devices[Unit].Name+")")
    return

# Synchronise images to match parameter in hardware page
def UpdateImage(Unit):
    if (Unit in Devices) and (Parameters["Mode1"] in Images):
        Domoticz.Debug("Device Image update: '" + Parameters["Mode1"] + "', Currently "+str(Devices[Unit].Image)+", should be "+str( Images[Parameters["Mode1"]].ID))
        if (Devices[Unit].Image != Images[Parameters["Mode1"]].ID):
            Devices[Unit].Update(nValue=Devices[Unit].nValue, sValue=str(Devices[Unit].sValue), Image=Images[Parameters["Mode1"]].ID)
    return
