"""
<plugin key="HttpHeaderTest" name="HTTP Header Parsing Test" author="domoticz" version="1.0.0">
    <params>
        <param field="Address" label="Test server address" width="200px" required="true" default="127.0.0.1"/>
        <param field="Port" label="Test server port" width="80px" required="true" default="18099"/>
    </params>
</plugin>

EXAMPLE PLUGIN - shipping location and run location
---------------------------------------------------
The tracked copy of this file lives under:
    plugins/examples/HttpHeaderTest/

Domoticz does NOT scan plugins/examples/; it only discovers plugins at
    plugins/<Name>/plugin.py
To use it, copy the HttpHeaderTest/ folder to the plugins/ directory and
restart Domoticz.  test/python/test_plugin_http_headers.py does that copy
automatically when it starts its own Domoticz instance.

What it does
------------
Drives the regression test for the HTTP response header parser in
hardware/plugins/PluginProtocols.cpp (CPluginProtocolHTTP::ExtractHeaders).

The plugin repeatedly connects to the test server, sends a GET and reports
what it received for the *previous* response in the query string of the next
request.  That way the test server learns the parse result of every case
without needing to read the Domoticz log:

    conn 1 : GET /report?r=start            -> server replies with case 1
    conn 2 : GET /report?r=<result case 1>  -> server replies with case 2
    ...

If the parser hangs or throws, no further connection arrives (or the result
is reported as 'nomessage') and the test server flags the case that broke.
"""
import Domoticz

from urllib.parse import quote

RESULT_START = "start"
RESULT_NOMESSAGE = "nomessage"


class BasePlugin:
    def __init__(self):
        self.conn = None
        self.pending = RESULT_START
        self.finished = False

    def onStart(self):
        Domoticz.Heartbeat(5)
        self.connect()

    def onStop(self):
        self.conn = None

    def connect(self):
        if self.finished or self.conn is not None:
            return
        self.conn = Domoticz.Connection(Name="HdrTest", Transport="TCP/IP", Protocol="HTTP",
                                        Address=Parameters["Address"], Port=Parameters["Port"])
        self.conn.Connect()

    def onConnect(self, Connection, Status, Description):
        if Status != 0:
            Domoticz.Log("HTTPTEST connect failed (%d): %s" % (Status, Description))
            self.conn = None
            return
        report = self.pending
        # Overwritten by onMessage, so a response that never gets parsed reports itself
        self.pending = RESULT_NOMESSAGE
        Connection.Send({"Verb": "GET",
                         "URL": "/report?r=" + quote(report, safe=""),
                         "Headers": {"Host": Parameters["Address"] + ":" + Parameters["Port"],
                                     "Connection": "close",
                                     "User-Agent": "Domoticz/HttpHeaderTest"}})

    def onMessage(self, Connection, Data):
        headers = Data.get("Headers", {}) or {}
        lower = {}
        for key, value in headers.items():
            if isinstance(value, list):
                value = ",".join([str(item) for item in value])
            lower[str(key).lower()] = str(value)

        body = Data.get("Data", b"")
        if isinstance(body, bytes):
            body = body.decode("utf-8", "ignore")

        case = lower.get("x-test", "?")
        result = "|".join(["case=" + case,
                           "status=" + str(Data.get("Status", "?")),
                           "body=" + body,
                           "nospace=" + lower.get("x-nospace", "-"),
                           "empty=" + lower.get("x-empty", "-"),
                           "headers=" + str(len(lower))])
        Domoticz.Log("HTTPTEST " + result)
        self.pending = result
        if case == "done":
            self.finished = True
        Connection.Disconnect()

    def onDisconnect(self, Connection):
        self.conn = None
        self.connect()

    def onHeartbeat(self):
        self.connect()


_plugin = BasePlugin()


def onStart():
    _plugin.onStart()


def onStop():
    _plugin.onStop()


def onConnect(Connection, Status, Description):
    _plugin.onConnect(Connection, Status, Description)


def onMessage(Connection, Data):
    _plugin.onMessage(Connection, Data)


def onDisconnect(Connection):
    _plugin.onDisconnect(Connection)


def onHeartbeat():
    _plugin.onHeartbeat()
