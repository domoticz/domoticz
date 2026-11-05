"""
<plugin key="EpexPredictor" name="EPEX Predictor (day-ahead electricity prices)" author="Domoticz" version="1.0.0" wikilink="https://github.com/b3nn0/EpexPredictor" externallink="https://epexpredictor.batzill.com/docs">
    <description>
        <h2>EPEX Predictor</h2><br/>
        Brings the day-ahead electricity prices (EPEX spot, with a forecast for the hours that are not published yet)
        from an EpexPredictor server into Domoticz: the current and next price, today's average, lowest and highest
        price, the cheapest period of the next 24 hours and a price level, plus a ready-made page at
        <b>#/Custom/EpexPredictor</b> with a price chart.<br/><br/>
        The public server <b>https://epexpredictor.batzill.com</b> is open and needs no key. You can also host the
        project yourself (for example with Docker compose) and enter its address below, for example
        <b>http://192.168.1.20:8000</b>.
    </description>
    <params>
        <param field="ServerUrl" label="Server address" width="320px" required="true" default="https://epexpredictor.batzill.com">
            <description>
                Address of the EpexPredictor server: a full URL, or host:port or an IP address (plain http is assumed for those).
            </description>
        </param>
        <param field="Region" label="Region" width="130px">
            <options>
                <option label="Germany (DE)" value="DE"/>
                <option label="Austria (AT)" value="AT"/>
                <option label="Belgium (BE)" value="BE"/>
                <option label="France (FR)" value="FR"/>
                <option label="Netherlands (NL)" value="NL" default="true"/>
                <option label="Sweden SE1" value="SE1"/>
                <option label="Sweden SE2" value="SE2"/>
                <option label="Sweden SE3" value="SE3"/>
                <option label="Sweden SE4" value="SE4"/>
                <option label="Denmark DK1" value="DK1"/>
                <option label="Denmark DK2" value="DK2"/>
                <option label="Spain (ES)" value="ES"/>
                <option label="Portugal (PT)" value="PT"/>
            </options>
        </param>
        <param field="PriceUnit" label="Price unit" width="120px">
            <description>
                Choose this once. Changing it later keeps the old history of the price devices in the old unit.
            </description>
            <options>
                <option label="ct/kWh" value="CT_PER_KWH" default="true"/>
                <option label="EUR/kWh" value="EUR_PER_KWH"/>
                <option label="EUR/MWh" value="EUR_PER_MWH"/>
            </options>
        </param>
        <param field="Surcharge" type="number" label="Surcharge (ct/kWh)" min="-100" max="1000" step="0.1" default="0" width="100px">
            <description>
                Fixed amount added to every price (energy supplier margin, grid fees). The server always takes this in ct/kWh.
            </description>
        </param>
        <param field="TaxPercent" type="number" label="Tax (%)" min="0" max="100" step="0.1" default="0" width="100px">
            <description>
                Tax percentage added on top of the price including the surcharge, for example 21 for Dutch VAT.
            </description>
        </param>
        <param field="CheapHours" type="number" label="Cheap period (hours)" min="0.25" max="12" step="0.25" default="3" width="100px">
            <description>
                Length of the period to find in the next 24 hours with the lowest average price, for example the time needed to charge a car or run a boiler.
            </description>
        </param>
        <group label="Advanced">
            <param field="Hourly" type="boolean" label="Hourly average prices" default="false">
                <description>
                    Use hourly instead of quarter hourly prices, if your energy supplier bills per hour.
                </description>
            </param>
            <param field="Timezone" label="Time zone" width="200px" default="">
                <description>
                    Time zone name for the clock times, for example Europe/Amsterdam. Leave empty to use the time zone of the region.
                </description>
            </param>
            <param field="Refresh" type="number" label="Refresh (s)" min="60" max="86400" step="60" default="900" width="100px">
                <description>
                    How often the prices are fetched from the server. Tomorrow's prices are published around 13:00, the forecast changes slowly. The current price is updated every few seconds without a request.
                </description>
            </param>
            <param field="Debug" type="boolean" label="Debug logging" default="false">
                <description>
                    Logs every request to the server. The hardware log level must also include status messages.
                </description>
            </param>
        </group>
    </params>
</plugin>
"""
import json
import os
import queue
import shutil
import sys
import threading
import time
from datetime import datetime, timedelta, timezone

import DomoticzEx as Domoticz

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import epex_api  # noqa: E402
import epex_ledger  # noqa: E402
import epex_logic  # noqa: E402

VERSION = "1.0.0"
PAGE_NAME = "EpexPredictor.html"
DEVICE_ID = "EPEX"

(U_NOW, U_NEXT, U_AVG, U_MIN, U_MAX, U_TOMORROW, U_CHEAP, U_LEVEL, U_CHEAP_ON, U_KNOWN, U_ALERT, U_ERROR) = range(1, 13)
DATA_DIR = os.path.join(_HERE, "epex-data")
ACCURACY_DAYS = 14  # days of forecast versus real prices sent to the page
ERROR_BACKOFF = 120  # seconds before a failed request is retried


def _text(name, default=""):
    value = Parameters.get(name, default)
    return default if value is None else str(value).strip()


def _float(name, default):
    try:
        return float(_text(name, default))
    except (TypeError, ValueError):
        return float(default)


def _int(name, default):
    return int(_float(name, default))


def _bool(name):
    return _text(name, "false").lower() in ("true", "1", "yes")


class BasePlugin:
    def __init__(self):
        self.client = None
        self.thread = None
        self.jobs = queue.Queue()
        self.results = queue.Queue()
        self.stopping = False
        self.inflight = False
        self.next_fetch = 0
        self.slots = []
        self.known_until = None
        self.fetched_at = 0
        self.error = ""
        self.connected = False
        self.last_key = None
        self.debug = False
        self.hwid = "0"
        self.snapshot_due = False
        self.unit_label = "ct/kWh"
        self.tz_name = ""
        self.info = {}
        self.ledger = None

    # ------------------------------------------------------------------ logging
    def log(self, text):
        Domoticz.Status(text)

    def dbg(self, text):
        if self.debug:
            Domoticz.Status("[debug] " + text)

    # ------------------------------------------------------------------ settings
    def region(self):
        value = _text("Region", "NL").upper()
        return value if value in epex_api.REGIONS else "NL"

    def unit(self):
        value = _text("PriceUnit", "CT_PER_KWH")
        return value if value in epex_api.UNITS else "CT_PER_KWH"

    def timezone(self):
        return _text("Timezone") or epex_api.REGION_TIMEZONE.get(self.region(), "Europe/Berlin")

    def decimals(self):
        return {"CT_PER_KWH": 2, "EUR_PER_KWH": 4, "EUR_PER_MWH": 2}[self.unit()]

    # ------------------------------------------------------------------ lifecycle
    def onStart(self):
        self.debug = _bool("Debug")
        self.hwid = str(Parameters.get("HardwareID", "0"))
        self.unit_label = epex_api.UNITS[self.unit()]
        self.tz_name = self.timezone()
        Domoticz.Heartbeat(10)
        self.client = epex_api.EpexClient(_text("ServerUrl", epex_api.DEFAULT_HOST))
        self._install_page()
        self._ensure_devices()
        self.ledger = epex_ledger.Ledger.load(self._ledger_path(), self._ledger_signature())
        self.stopping = False
        self.inflight = False
        self.next_fetch = 0
        self.jobs = queue.Queue()
        self.results = queue.Queue()
        self.thread = threading.Thread(target=self._worker, name="EpexWorker", daemon=True)
        self.thread.start()
        self.log("EPEX Predictor %s started (hardware %s, %s, %s)" % (VERSION, self.hwid, self.client.base, self.region()))

    def onStop(self):
        self.stopping = True
        self.jobs.put(None)
        if self.thread:
            self.thread.join(5)
        self._remove_page()
        self._remove_export()
        self.log("EPEX Predictor stopped")

    def onHeartbeat(self):
        self._drain_results()
        now = time.time()
        if not self.inflight and now >= self.next_fetch:
            self.inflight = True
            self.next_fetch = now + max(60, _int("Refresh", 900))
            self.jobs.put(self._fetch_params())
        self._update_devices()
        if self.snapshot_due:
            self.snapshot_due = False
            self._push_snapshot()

    # ------------------------------------------------------------------ worker thread (HTTP only, no Domoticz calls)
    def _fetch_params(self):
        """Everything the worker needs, read here on the main thread so the worker never touches Domoticz."""
        params = {
            "region": self.region(), "unit": self.unit(), "surcharge": _float("Surcharge", 0),
            "tax_percent": _float("TaxPercent", 0), "hourly": _bool("Hourly"), "timezone": self.tz_name,
        }
        params["start_ts"] = self._day_start_param(self.tz_name)
        self.dbg("GET %s/prices region=%s unit=%s tz=%s startTs=%s" % (
            self.client.base, params["region"], params["unit"], self.tz_name, params["start_ts"]))
        return params

    def _worker(self):
        jobs, results = self.jobs, self.results
        while not self.stopping:
            job = jobs.get()
            if job is None:
                break
            try:
                results.put((job, self._fetch(job), None))
            except Exception as err:  # noqa: BLE001
                results.put((job, None, err))

    def _fetch(self, p):
        info = self.info or self.client.get_info()
        if info.get("regions") and p["region"] not in info["regions"]:
            raise epex_api.EpexError("region %s is not offered by this server (%s)" % (p["region"], ", ".join(info["regions"])), 422)
        payload = self.client.get_prices(
            region=p["region"], unit=p["unit"], surcharge=p["surcharge"], tax_percent=p["tax_percent"],
            hourly=p["hourly"], timezone=p["timezone"], hours=-1, start_ts=p["start_ts"])
        return payload, info

    def _day_start_param(self, tz_name):
        """Midnight today in the output time zone, so the chart can show the whole day."""
        try:
            from zoneinfo import ZoneInfo
            now = datetime.now(ZoneInfo(tz_name))
        except Exception as err:  # noqa: BLE001
            Domoticz.Error("EPEX Predictor: time zone %s is not available (%r), install the Python tzdata package; the chart will not start at midnight" % (tz_name, err))
            return None
        return now.replace(hour=0, minute=0, second=0, microsecond=0).strftime("%Y-%m-%dT%H:%M:%S")

    # ------------------------------------------------------------------ results (main thread)
    def _drain_results(self):
        while True:
            try:
                job, result, err = self.results.get_nowait()
            except queue.Empty:
                return
            self.inflight = False
            if err is not None:
                self._on_error(err)
                continue
            try:
                self._on_prices(result)
            except Exception as exc:  # noqa: BLE001
                Domoticz.Error("EPEX Predictor: failed to process prices: %r" % exc)

    def _on_error(self, err):
        was_ok = self.connected or not self.error
        self.connected = False
        self.error = str(err)
        self.next_fetch = time.time() + ERROR_BACKOFF
        if was_ok:
            Domoticz.Error("EPEX Predictor: %s" % self.error)
        self._write_unit(U_ALERT, 4, "Server error: " + self.error[:120])
        self.snapshot_due = True

    def _on_prices(self, result):
        payload, info = result
        self.info = info or self.info
        slots, known = epex_logic.parse_slots(payload)
        if not slots:
            self._on_error(epex_api.EpexError("the server returned no prices for region %s" % self.region(), 200))
            return
        self.slots, self.known_until = slots, known
        self.fetched_at = time.time()
        self.connected = True
        self.error = ""
        self.last_key = None  # force a device update
        self._write_unit(U_ALERT, 1, "Connected to %s" % self.client.base)
        self._write_export()
        self._record_forecast()
        self.log("EPEX Predictor: %d prices (%s to %s), known until %s" % (
            len(slots), slots[0][0].strftime("%d-%m %H:%M"), slots[-1][0].strftime("%d-%m %H:%M"),
            known.strftime("%d-%m %H:%M") if known else "?"))
        self.snapshot_due = True

    # ------------------------------------------------------------------ forecast versus real prices
    def _ledger_path(self):
        return os.path.join(DATA_DIR, "epex_ledger_%s.json" % self.hwid)

    def _ledger_signature(self):
        """Forecasts made with other settings cannot be compared with the new prices, so a change starts a new ledger."""
        return "%s|%s|%s|%s|%s" % (self.region(), self.unit(), _float("Surcharge", 0), _float("TaxPercent", 0), _bool("Hourly"))

    def _record_forecast(self):
        if self.ledger is None:
            return
        signature = self._ledger_signature()
        if self.ledger.signature != signature:
            self.log("EPEX Predictor: settings changed, the forecast comparison starts again")
            self.ledger = epex_ledger.Ledger(signature)
        if self.known_until is None:
            self.dbg("the server gave no knownUntil, the forecast comparison cannot tell published prices from forecast")
        if self.ledger.update(self.slots, self.known_until, int(time.time())):
            try:
                self.ledger.save(self._ledger_path())
            except OSError as exc:
                Domoticz.Error("EPEX Predictor: cannot save %s: %r" % (self._ledger_path(), exc))
        acc = self.accuracy(self._now())
        yesterday = (self._now().date() - timedelta(days=1)).isoformat()
        for entry in acc["days"]:
            if entry["date"] == yesterday:
                self._write_unit(U_ERROR, 0, self._fmt(entry["mae"]))

    def accuracy(self, now):
        """Forecast versus real price of the last ACCURACY_DAYS days, see epex_logic.accuracy."""
        pairs = []
        if self.ledger is not None:
            first = now.replace(hour=0, minute=0, second=0, microsecond=0) - timedelta(days=ACCURACY_DAYS)
            pairs = self.ledger.pairs(int(first.timestamp()))
        # the real zone, not now.tzinfo (a fixed offset), so days that contain a DST switch are grouped right
        result = epex_logic.accuracy(pairs, self._zone() or now.tzinfo, _float("CheapHours", 3))
        result["slots"] = [[ts, round(fc, 5), round(act, 5)] for ts, fc, act in pairs]
        return result

    # ------------------------------------------------------------------ devices
    def _unit(self, unit):
        if DEVICE_ID not in Devices:
            return None
        units = Devices[DEVICE_ID].Units
        return units[unit] if unit in units else None

    def _create(self, unit, name, **kw):
        if self._unit(unit) is not None:
            return
        Domoticz.Unit(Name=name, DeviceID=DEVICE_ID, Unit=unit, Used=1, **kw).Create()

    def _price(self, unit, name):
        wanted = {"Custom": "1;" + self.unit_label}
        existing = self._unit(unit)
        if existing is None:
            self._create(unit, name, Type=243, Subtype=31, Options=wanted)
        elif dict(existing.Options or {}).get("Custom") != wanted["Custom"]:
            # the price unit setting changed, so the axis label of the device has to follow
            existing.Options = wanted
            existing.Update(Log=False, UpdateOptions=True)

    def _ensure_devices(self):
        self._price(U_NOW, "EPEX price now")
        self._price(U_NEXT, "EPEX price next")
        self._price(U_AVG, "EPEX price average today")
        self._price(U_MIN, "EPEX price lowest today")
        self._price(U_MAX, "EPEX price highest today")
        self._price(U_TOMORROW, "EPEX price average tomorrow")
        self._create(U_CHEAP, "EPEX cheapest period", Type=243, Subtype=19)
        self._create(U_LEVEL, "EPEX price level", Type=243, Subtype=22)
        self._create(U_CHEAP_ON, "EPEX cheap period active", Type=244, Subtype=73, Switchtype=0)
        self._create(U_KNOWN, "EPEX prices known until", Type=243, Subtype=19)
        self._create(U_ALERT, "EPEX connection", Type=243, Subtype=22)
        self._price(U_ERROR, "EPEX forecast error (MAE)")

    def _write_unit(self, unit, nvalue, svalue):
        u = self._unit(unit)
        if u is None:
            return
        if u.nValue == nvalue and u.sValue == svalue:
            return
        u.nValue = nvalue
        u.sValue = svalue
        u.Update(Log=False)

    def _fmt(self, value):
        return ("%." + str(self.decimals()) + "f") % value

    def _zone(self):
        """The output time zone, None when the system has no time zone database."""
        try:
            from zoneinfo import ZoneInfo
            return ZoneInfo(self.tz_name)
        except Exception:  # noqa: BLE001
            return None

    def _now(self):
        """Now in the output time zone, as a datetime with a fixed offset so that adding hours is a real duration.

        The slots only carry a fixed offset that is wrong after a DST switch, so the zone itself decides the offset.
        """
        zone = self._zone()
        if zone is not None:
            d = datetime.now(zone)
            return d.replace(tzinfo=timezone(d.utcoffset()))
        if not self.slots:
            return datetime.now().astimezone()
        return datetime.now(self.slots[0][0].tzinfo)

    def state(self, now=None):
        """Everything derived from the cached prices for the given moment (also used by the page snapshot)."""
        now = now or self._now()
        slots = self.slots
        idx = epex_logic.current_index(slots, now)
        out = {"now": None, "next": None, "today": None, "tomorrow": None, "cheap": None, "level": None, "idx": idx}
        if not slots:
            return out
        today = now.date()
        out["today"] = epex_logic.day_stats(slots, today)
        out["tomorrow"] = epex_logic.day_stats(slots, today + timedelta(days=1))
        if idx is not None:
            out["now"] = slots[idx][1]
            if idx + 1 < len(slots):
                out["next"] = slots[idx + 1][1]
            out["level"] = epex_logic.price_level(slots, slots[idx][0], slots[idx][1])
        hours = _float("CheapHours", 3)
        out["cheap"] = epex_logic.cheapest_window(slots, now, hours) if hours > 0 else None
        return out

    def _update_devices(self):
        if not self.slots:
            return
        now = self._now()
        st = self.state(now)
        key = (st["idx"], st["today"] and st["today"]["count"], st["tomorrow"] and st["tomorrow"]["count"],
               st["cheap"] and st["cheap"]["start"], st["cheap"] and st["cheap"]["active"])
        if key == self.last_key:
            return
        if self.last_key is not None:
            self.snapshot_due = True  # a new slot started
        self.last_key = key
        if st["now"] is None:
            self._write_unit(U_ALERT, 3, "No price for the current time, the server data is outdated")
            return
        self._write_unit(U_NOW, 0, self._fmt(st["now"]))
        if st["next"] is not None:
            self._write_unit(U_NEXT, 0, self._fmt(st["next"]))
        if st["today"]:
            self._write_unit(U_AVG, 0, self._fmt(st["today"]["avg"]))
            self._write_unit(U_MIN, 0, self._fmt(st["today"]["min"]))
            self._write_unit(U_MAX, 0, self._fmt(st["today"]["max"]))
        if st["tomorrow"] and st["tomorrow"]["complete"]:
            self._write_unit(U_TOMORROW, 0, self._fmt(st["tomorrow"]["avg"]))
        cheap = st["cheap"]
        if cheap:
            self._write_unit(U_CHEAP, 0, "%s - %s (avg %s %s)" % (cheap["start"].strftime("%a %H:%M"), cheap["end"].strftime("%H:%M"),
                                                                  self._fmt(cheap["avg"]), self.unit_label))
            self._write_unit(U_CHEAP_ON, 1 if cheap["active"] else 0, "On" if cheap["active"] else "Off")
        if st["level"]:
            label, _ = st["level"]
            self._write_unit(U_LEVEL, epex_logic.LEVEL_ALERT[label], label)
        if self.known_until:
            self._write_unit(U_KNOWN, 0, self.known_until.strftime("%a %d-%m %H:%M"))

    # ------------------------------------------------------------------ custom page
    def _templates_dir(self):
        candidates = []
        startup = Parameters.get("StartupFolder")
        if startup:
            candidates.append(os.path.join(startup, "www"))
        candidates.append(os.path.join(os.path.abspath(os.path.join(_HERE, "..", "..")), "www"))
        for www in candidates:
            if os.path.isdir(www):
                return os.path.join(www, "templates")
        return os.path.join(candidates[0], "templates")

    def _install_page(self):
        src = os.path.join(_HERE, PAGE_NAME)
        dest = os.path.join(self._templates_dir(), PAGE_NAME)
        if not os.path.isfile(src):
            Domoticz.Error("EPEX Predictor: page template %s missing" % src)
            return
        try:
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            shutil.copy2(src, dest)
            self.log("EPEX Predictor: page installed, open it at #/Custom/EpexPredictor, prices for scripts at /templates/%s" % self.export_name())
        except OSError as exc:
            Domoticz.Error("EPEX Predictor: cannot install page: %r" % exc)

    def _remove_page(self):
        dest = os.path.join(self._templates_dir(), PAGE_NAME)
        try:
            if os.path.isfile(dest):
                os.remove(dest)
        except OSError as exc:
            Domoticz.Error("EPEX Predictor: cannot remove page: %r" % exc)

    # ------------------------------------------------------------------ JSON export for other software (Node-RED, scripts)
    def export_name(self):
        return "epex_prices_%s.json" % self.hwid

    def _export_path(self):
        return os.path.join(self._templates_dir(), self.export_name())

    def build_export(self):
        """Prices with the surcharge and tax applied, in the configured unit, for other software to plan with."""
        ku = self.known_until
        length = epex_logic.slot_seconds(self.slots) if self.slots else 0
        return {
            "source": "EpexPredictor", "plugin_version": VERSION, "server": self.client.base if self.client else "",
            "region": self.region(), "unit": self.unit_label, "timezone": self.tz_name, "slot_seconds": length,
            "surcharge_ct_per_kwh": _float("Surcharge", 0), "tax_percent": _float("TaxPercent", 0),
            "updated": datetime.fromtimestamp(self.fetched_at).astimezone().isoformat(timespec="seconds") if self.fetched_at else None,
            "known_until": ku.isoformat() if ku else None,
            "prices": [{"start": s.isoformat(), "ts": int(s.timestamp()), "price": round(p, 5),
                        "known": bool(ku and s <= ku)} for s, p in self.slots],
        }

    def _write_export(self):
        path = self._export_path()
        try:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            tmp = path + ".tmp"
            with open(tmp, "w", encoding="utf-8") as fh:
                json.dump(self.build_export(), fh, separators=(",", ":"))
            os.replace(tmp, path)
            self.dbg("export written: %s" % path)
        except OSError as exc:
            Domoticz.Error("EPEX Predictor: cannot write %s: %r" % (path, exc))

    def _remove_export(self):
        try:
            if os.path.isfile(self._export_path()):
                os.remove(self._export_path())
        except OSError as exc:
            Domoticz.Error("EPEX Predictor: cannot remove export: %r" % exc)

    # ------------------------------------------------------------------ web socket channel
    def _send(self, payload):
        payload.setdefault("hwid", self.hwid)
        try:
            # a JSON string, as Domoticz turns a Python None into the text "None" when it converts a dict itself
            Domoticz.WebSocketSend(json.dumps(payload, separators=(",", ":")))
        except Exception as exc:  # noqa: BLE001
            self.dbg("websocket send failed: %r" % exc)

    def _push_snapshot(self):
        now = self._now()
        st = self.state(now)

        def day(d):
            return d and {"avg": d["avg"], "min": d["min"], "max": d["max"], "complete": d["complete"]}

        cheap = st["cheap"]
        self._send({
            "type": "snapshot", "version": VERSION, "ts": int(time.time()),
            "server": self.client.base if self.client else "", "server_version": self.info.get("version"), "region": self.region(), "unit": self.unit_label,
            "decimals": self.decimals(), "timezone": self.tz_name, "hourly": _bool("Hourly"),
            "cheap_hours": _float("CheapHours", 3), "slot_seconds": epex_logic.slot_seconds(self.slots) if self.slots else 0,
            "surcharge": _float("Surcharge", 0), "tax": _float("TaxPercent", 0),
            "known_until": self.known_until.isoformat() if self.known_until else None,
            "fetched_at": int(self.fetched_at), "connected": self.connected, "error": self.error,
            "now": st["now"], "next": st["next"], "today": day(st["today"]), "tomorrow": day(st["tomorrow"]),
            "level": st["level"] and {"label": st["level"][0], "percentile": st["level"][1]},
            "cheap": cheap and {"start": cheap["start"].isoformat(), "end": cheap["end"].isoformat(), "avg": cheap["avg"], "active": cheap["active"]},
            "prices": [[int(s.timestamp()), round(p, 5)] for s, p in self.slots],
            "accuracy": self.accuracy(now),
        })

    def onWebSocketMessage(self, Data):
        try:
            msg = json.loads(Data) if isinstance(Data, str) else Data
        except ValueError:
            return
        if not isinstance(msg, dict):
            return
        target = msg.get("hwid")
        if target is not None and str(target) != self.hwid:
            return
        cmd = msg.get("cmd")
        if cmd == "snapshot":
            self._push_snapshot()
        elif cmd == "refresh":
            if time.time() - self.fetched_at > 60 and not self.inflight:
                self.next_fetch = 0
                self.error = ""
            self._send({"type": "ack", "cmd": cmd})
        else:
            self._send({"type": "error", "error": "unknown command"})

    # ------------------------------------------------------------------ device commands
    def onCommand(self, DeviceID, Unit, Command, Level, Color):
        if Unit == U_CHEAP_ON:
            # read only, put the displayed state back
            st = self.state()
            on = bool(st["cheap"] and st["cheap"]["active"])
            self._write_unit(U_CHEAP_ON, 1 if on else 0, "On" if on else "Off")


global _plugin
_plugin = BasePlugin()


def onStart():
    _plugin.onStart()


def onStop():
    _plugin.onStop()


def onHeartbeat():
    _plugin.onHeartbeat()


def onCommand(DeviceID, Unit, Command, Level, Color):
    _plugin.onCommand(DeviceID, Unit, Command, Level, Color)


def onWebSocketMessage(Data):
    _plugin.onWebSocketMessage(Data)
