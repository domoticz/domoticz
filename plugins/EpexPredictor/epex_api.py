"""Minimal client for the EpexPredictor REST API (https://github.com/b3nn0/EpexPredictor).

Standard library only. The host is configurable so the public instance or a self hosted one (Docker) can be used.
"""
import ipaddress
import json
import urllib.error
import urllib.parse
import urllib.request

DEFAULT_HOST = "https://epexpredictor.batzill.com"

REGIONS = ("DE", "AT", "BE", "FR", "NL", "SE1", "SE2", "SE3", "SE4", "DK1", "DK2", "ES", "PT")
UNITS = {"CT_PER_KWH": "ct/kWh", "EUR_PER_KWH": "EUR/kWh", "EUR_PER_MWH": "EUR/MWh"}

# Default output time zone per bidding zone, so the plugin shows local clock times without extra setup
REGION_TIMEZONE = {
    "DE": "Europe/Berlin", "AT": "Europe/Vienna", "BE": "Europe/Brussels", "FR": "Europe/Paris", "NL": "Europe/Amsterdam",
    "SE1": "Europe/Stockholm", "SE2": "Europe/Stockholm", "SE3": "Europe/Stockholm", "SE4": "Europe/Stockholm",
    "DK1": "Europe/Copenhagen", "DK2": "Europe/Copenhagen", "ES": "Europe/Madrid", "PT": "Europe/Lisbon",
}


class EpexError(Exception):
    """Any failure talking to the EpexPredictor server. status is the HTTP status, 0 for network errors."""

    def __init__(self, message, status=0):
        super().__init__(message)
        self.status = status


def normalize_base_url(host):
    """Turn what a user typed into a base URL.

    Accepts a full URL, "host", "host:port", an IP address or "ip:port". Without a scheme a plain name without port
    is assumed to be a public https site, anything with a port, an IP address or a single label name (a Docker
    service, localhost) is assumed to be plain http.
    """
    text = (host or "").strip().rstrip("/")
    if not text:
        text = DEFAULT_HOST
    if "://" not in text:
        parsed = urllib.parse.urlsplit("//" + text)
        name = parsed.hostname or ""
        try:
            port = parsed.port
        except ValueError:
            port = None
        is_ip = False
        try:
            ipaddress.ip_address(name)
            is_ip = True
        except ValueError:
            pass
        plain = port is not None or is_ip or "." not in name
        text = ("http://" if plain else "https://") + text
    return text.rstrip("/")


class EpexClient:
    def __init__(self, host=DEFAULT_HOST, timeout=30, opener=None):
        self.base = normalize_base_url(host)
        self._timeout = timeout
        self._open = opener or urllib.request.urlopen

    def _get(self, path, params=None):
        url = self.base + path
        query = {k: v for k, v in (params or {}).items() if v is not None and v != ""}
        if query:
            url += "?" + urllib.parse.urlencode(query)
        req = urllib.request.Request(url, headers={"Accept": "application/json", "User-Agent": "Domoticz-EpexPredictor"})
        try:
            with self._open(req, timeout=self._timeout) as resp:
                raw = resp.read().decode("utf-8")
        except urllib.error.HTTPError as err:
            detail = ""
            try:
                payload = json.loads(err.read().decode("utf-8"))
                if isinstance(payload, dict) and payload.get("detail"):
                    detail = ": " + str(payload["detail"])[:160]
            except Exception:  # noqa: BLE001
                pass
            raise EpexError("HTTP %d%s" % (err.code, detail), err.code) from None
        except (urllib.error.URLError, OSError, TimeoutError) as err:
            reason = getattr(err, "reason", None)
            raise EpexError("cannot reach %s (%s)" % (self.base, type(reason or err).__name__), 0) from None
        try:
            return json.loads(raw)
        except ValueError:
            raise EpexError("%s did not return JSON, is this an EpexPredictor server?" % self.base, 200) from None

    def get_info(self):
        """Title, version and supported regions from /openapi.json, {} when the server does not offer it."""
        try:
            doc = self._get("/openapi.json")
            regions = (doc.get("components", {}).get("schemas", {}).get("PriceRegionName", {}) or {}).get("enum") or []
            info = doc.get("info", {})
            return {"title": str(info.get("title", "")), "version": str(info.get("version", "")), "regions": list(regions)}
        except (EpexError, AttributeError):
            return {}

    def get_prices(self, region="DE", unit="CT_PER_KWH", surcharge=0.0, tax_percent=0.0, hourly=False, timezone=None,
                   hours=-1, start_ts=None):
        payload = self._get("/prices", {
            "region": region, "unit": unit, "surcharge": surcharge, "taxPercent": tax_percent,
            "hourly": "true" if hourly else "false", "timezone": timezone, "hours": hours, "startTs": start_ts,
        })
        if not isinstance(payload, dict) or not isinstance(payload.get("prices"), list):
            raise EpexError("unexpected answer from %s, is this an EpexPredictor server?" % self.base, 200)
        return payload
