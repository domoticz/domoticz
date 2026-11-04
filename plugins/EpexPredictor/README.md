# EPEX Predictor plugin for Domoticz

Brings day-ahead electricity prices into Domoticz from an [EpexPredictor](https://github.com/b3nn0/EpexPredictor)
server: the current and next price, today's average, lowest and highest price, the cheapest period of the next 24 hours,
a price level, and a ready-made page with a price chart. Prices that are not published yet (tomorrow before about 13:00
CET, and the days after) are a model forecast and are marked as such.

The public server `https://epexpredictor.batzill.com` is open (fair use, no key). The server address is a setting, so you
can host the project yourself (for example with Docker compose) and point the plugin at it.

## Setup

1. In Domoticz add hardware of type **EPEX Predictor (day-ahead electricity prices)**.
2. Pick your **Region** (default NL). Optionally enter a **Surcharge** (ct/kWh) and **Tax** (%) so the prices match your
   energy contract. The server applies them, so every price, average and cheap period includes them.
3. Open the page **`#/Custom/EpexPredictor`**.

## Own server (Docker compose)

```
git clone https://github.com/b3nn0/EpexPredictor
cd EpexPredictor
docker compose up -d
```

Then set **Server address** to `http://<ip>:<port>` (the port is in the project's `compose.yaml`). A full URL, `host:port`
or a bare IP address all work (plain `http` is assumed for an IP address, a port or a single word name such as a Docker
service name). Set `EPEXPREDICTOR_ENTSOE_API_KEY` (a free ENTSO-E key) in the compose file for better accuracy, it is
required for the Swedish zones.

## Devices

| Device | Type | Notes |
|---|---|---|
| price now, next | Custom sensor | in the chosen unit, graphs and history come from Domoticz |
| average, lowest, highest today | Custom sensor | |
| average tomorrow | Custom sensor | set once tomorrow is complete |
| cheapest period | Text | for example `Wed 11:15 - 14:15 (avg 25.49 ct/kWh)` |
| cheap period active | Switch | read only, on while now is inside the cheapest period, handy as an automation trigger |
| price level | Alert | Very cheap/Cheap (green), Normal (yellow), Expensive (orange), Very expensive (red), relative to the next 24 hours |
| prices known until | Text | end of the published prices, after it the values are forecast |
| connection | Alert | server unreachable, wrong address, no data |

The current price devices follow the clock every 10 seconds from the cached prices, the server is only asked every
**Refresh** seconds (default 15 minutes).

## Prices for other software (Node-RED, scripts)

After every fetch the plugin writes all prices (surcharge and tax included) to

```
http://<domoticz>:8080/templates/epex_prices_<hardware idx>.json
```

(the exact URL is in the Domoticz log at start). It sits next to the page, so the same Domoticz authentication rules
apply. In Node-RED use an `http request` node (GET, return a parsed JSON object). Format:

```json
{
  "source": "EpexPredictor", "region": "NL", "unit": "ct/kWh", "timezone": "Europe/Amsterdam",
  "slot_seconds": 900, "surcharge_ct_per_kwh": 1.5, "tax_percent": 21.0,
  "updated": "2026-11-04T09:20:44+01:00", "known_until": "2026-11-04T23:45:00+01:00",
  "prices": [
    {"start": "2026-11-04T00:00:00+01:00", "ts": 1793746800, "price": 26.0307, "known": true},
    {"start": "2026-11-05T00:00:00+01:00", "ts": 1793833200, "price": 29.1, "known": false}
  ]
}
```

`known` is `true` for published day-ahead prices and `false` for the model forecast, so a planner can weigh them
differently. `ts` is epoch seconds, `start` has the local UTC offset. The file is removed when the hardware is disabled.

## Settings

| Setting | Meaning |
|---|---|
| Server address | URL, `host:port` or IP of the EpexPredictor server |
| Region | bidding zone: DE, AT, BE, FR, NL, SE1-4, DK1-2, ES, PT |
| Price unit | ct/kWh (default), EUR/kWh or EUR/MWh |
| Surcharge, Tax | added by the server to every price |
| Cheap period (hours) | length of the period searched in the next 24 hours, 0.25 to 12 |
| Hourly average prices | hourly instead of 15 minute prices |
| Time zone | clock times, empty means the zone of the region |
| Refresh | seconds between requests, default 900 |
| Debug logging | logs every request |

## Tests

`python -m unittest discover -s plugins/EpexPredictor/tests` runs the unit tests for the client and the price logic (no
Domoticz needed).

Prices by [energy-charts.info](https://api.energy-charts.info/) and [ENTSO-E](https://www.entsoe.eu/) (CC-BY-4.0),
weather by [Open-Meteo](https://open-meteo.com/).
