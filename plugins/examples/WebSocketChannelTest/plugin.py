"""
<plugin key="WSChannelTest" name="WebSocket Channel Test" author="domoticz" version="1.0.5">
    <params>
        <param field="Mode1" label="Tag" width="120px" default="A"/>
        <param field="Mode2" label="Web port" width="80px" default="8080"/>
    </params>
</plugin>

EXAMPLE PLUGIN — shipping location and run location
----------------------------------------------------
The tracked copy of this file lives under:
    plugins/examples/WebSocketChannelTest/

Domoticz does NOT scan plugins/examples/; it only discovers plugins at
    plugins/<Name>/plugin.py
To use this example, copy the entire WebSocketChannelTest/ folder to the
plugins/ directory (e.g. plugins/WebSocketChannelTest/) and restart Domoticz.
Any name may be used for <Name>; the plugin key ("WSChannelTest") is what
matters for the WebSocket channel topic.

This plugin is run from plugins/WebSocketChannelTest/ (two directory levels
below the Domoticz root, identical depth to plugins/Domoticz-MeshCore-Plugin).
The domoticz_root computation therefore uses exactly 2x ".." — the same idiom
MeshCore uses — so that www/templates/ resolves to the correct destination.
"""
import Domoticz
import filecmp
import json
import os
import shutil
import tempfile
import urllib.request


class BasePlugin:
    def __init__(self):
        self.beat = 0
        self.tag = "A"

    def onStart(self):
        self.tag = Parameters.get("Mode1", "A")
        Domoticz.Log("WSChannelTest v%s started, tag=%s hwid=%s" % (PLUGIN_VERSION, self.tag, Parameters["HardwareID"]))

        plugin_dir    = os.path.dirname(os.path.abspath(__file__))
        template      = os.path.join(plugin_dir, "wschanneltest.html")
        domoticz_root = os.path.abspath(os.path.join(plugin_dir, "..", ".."))
        dest_dir      = os.path.join(domoticz_root, "www", "templates")
        dest          = os.path.join(dest_dir, "wschanneltest.html")

        os.makedirs(dest_dir, exist_ok=True)
        if os.path.isfile(dest) and filecmp.cmp(template, dest, shallow=False):
            Domoticz.Log("WSChannelTest: custom page already up to date")
        else:
            tmp_path = None
            try:
                fd, tmp_path = tempfile.mkstemp(prefix=".wschanneltest-", dir=dest_dir)
                os.close(fd)
                shutil.copy2(template, tmp_path)
                os.replace(tmp_path, dest)
                tmp_path = None
                Domoticz.Log("WSChannelTest: custom page installed: %s" % dest)
            except Exception as e:
                if tmp_path is not None:
                    try:
                        os.remove(tmp_path)
                    except OSError:
                        pass
                if os.path.isfile(dest):
                    Domoticz.Log("WSChannelTest: concurrent install by sibling, dest present")
                else:
                    Domoticz.Error("WSChannelTest: install FAILED, dest missing: %r" % e)

        Domoticz.WebSocketSend({"type": "started", "tag": self.tag})

    def onHeartbeat(self):
        self.beat += 1
        Domoticz.WebSocketSend({"type": "tick", "tag": self.tag, "beat": self.beat})
        # Send the raw-string payload form every 5th heartbeat (~50s cadence) so that
        # a test client connecting at any point can observe it within a bounded window.
        if self.beat % 5 == 0:
            Domoticz.WebSocketSend("raw-string-from-%s-beat%d" % (self.tag, self.beat))

    def onWebSocketMessage(self, Data):
        Domoticz.Log("WSChannelTest got inbound: %s" % str(Data))
        try:
            payload = json.loads(Data) if isinstance(Data, str) else Data
        except Exception:
            payload = {"raw": str(Data)}
        if isinstance(payload, dict) and payload.get("cmd") == "ping":
            Domoticz.WebSocketSend({
                "type": "pong",
                "tag": self.tag,
                "hwid": Parameters["HardwareID"],
                "ts": payload.get("ts"),
            })
        else:
            Domoticz.WebSocketSend({"type": "echo", "tag": self.tag, "received": payload})

    def _count_sibling_instances(self):
        """Query the local Domoticz JSON API and return the number of OTHER enabled
        instances of this plugin (Type==94, Extra=="WSChannelTest", idx != this hwid).

        Returns -1 if the query fails — callers must treat -1 as "unknown / be safe".
        """
        port = Parameters.get("Mode2", "8080") or "8080"
        this_hwid = str(Parameters["HardwareID"])
        url = "http://127.0.0.1:%s/json.htm?type=command&param=gethardware" % port
        try:
            with urllib.request.urlopen(url, timeout=5) as resp:
                data = json.loads(resp.read().decode("utf-8"))
        except Exception as exc:
            Domoticz.Debug("WSChannelTest: could not query hardware list (%s) — skipping template removal to be safe" % exc)
            return -1

        result = data.get("result", [])
        siblings = [
            h for h in result
            if h.get("Type") == 94
            and h.get("Extra") == "WSChannelTest"
            and str(h.get("idx", "")) != this_hwid
            and h.get("Enabled") in ("true", True)
        ]
        return len(siblings)

    def onStop(self):
        Domoticz.Log("WSChannelTest v%s stopping, tag=%s hwid=%s" % (PLUGIN_VERSION, self.tag, Parameters["HardwareID"]))

        plugin_dir    = os.path.dirname(os.path.abspath(__file__))
        domoticz_root = os.path.abspath(os.path.join(plugin_dir, "..", ".."))
        tpl_dir       = os.path.join(domoticz_root, "www", "templates")
        dest          = os.path.join(tpl_dir, "wschanneltest.html")

        sibling_count = self._count_sibling_instances()
        if sibling_count < 0:
            Domoticz.Log("WSChannelTest: sibling query failed, keeping template")
            return
        if sibling_count > 0:
            Domoticz.Log("WSChannelTest: %d sibling(s) still running, keeping template" % sibling_count)
            return

        try:
            if os.path.isfile(dest):
                os.remove(dest)
                Domoticz.Log("WSChannelTest: custom page removed")
            else:
                Domoticz.Log("WSChannelTest: custom page not present, nothing to remove")
        except Exception as e:
            Domoticz.Error("WSChannelTest: remove FAILED %s : %r" % (dest, e))


PLUGIN_VERSION = "1.0.5"
_plugin = BasePlugin()


def onStart():                    _plugin.onStart()
def onStop():                     _plugin.onStop()
def onHeartbeat():                _plugin.onHeartbeat()
def onWebSocketMessage(Data):     _plugin.onWebSocketMessage(Data)
