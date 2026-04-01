# Dashboard 2.0

**Revision:** 2026-03-31
**Branch:** css-theme-restructure
**Minimum build:** 17584

---

## Overview

Dashboard 2.0 is a fully modular, drag-and-drop dashboard system for Domoticz. It replaces the fixed-layout classic dashboard with a flexible, personalized workspace where users can freely compose, resize, and arrange widgets to their liking.

Key characteristics:
- Grid-based layout powered by **GridStack.js**
- Per-user persistent layouts stored in the Domoticz database
- Multiple named dashboards per user
- Real-time device updates via the existing WebSocket (livesocket)
- Fully theme-aware using CSS custom properties (`--dz-*` variables)
- Responsive across screen sizes

---

## Getting Started

Navigate to the **Dashboard 2** menu item. On first visit, a starter layout is created automatically containing a Clock, Sun Info, and Activity Log widget.

### Toolbar (Compact Mode)

In view mode, a small floating pill bar appears in the top-right corner of the page. It contains:
- **Dashboard name** — click to open the switcher dropdown if multiple dashboards exist
- **Pencil icon** — enter Edit Mode
- **Play icon** — start/stop Kiosk mode (visible when 2+ dashboards exist)

The bar fades to 50% opacity when not hovered to stay out of the way.

### Edit Mode

Press the pencil button or **Ctrl+E** to enter Edit Mode. The full toolbar appears at the top:

| Button | Action |
|--------|--------|
| Dashboard title | Click to rename inline |
| **Dashboards** | Switch between or manage dashboards |
| **Done Editing** | Save changes and exit edit mode |
| **Add Widget** | Open the widget library panel |
| **Save** | Manually save current layout |
| **Export / Import** | Open the export/import dialog |
| **Cancel** | Discard unsaved changes |

**Keyboard shortcuts:**
- `Ctrl+E` — toggle edit mode
- `Ctrl+S` — save layout
- `Escape` — exit edit mode (saves if dirty); also stops Kiosk mode

### Widget Library

Click **Add Widget** to open the slide-in library panel on the right. Widgets are grouped by category. Use the search box to filter. Click a widget's **+** button to add it to the grid.

### Widget Actions (Edit Mode)

Each widget shows a header bar in edit mode with:
- **Drag handle** — drag the title bar to reposition
- **Configure** (gear icon) — open widget settings
- **Duplicate** (copy icon) — clone the widget; the clone starts dragging immediately
- **Remove** (× icon) — delete the widget

Resize any widget by dragging the bottom-right corner handle.

---

## Managing Dashboards

### Multiple Dashboards

Click **Dashboards → Manage Dashboards...** to open the dashboard manager where you can:
- Create new dashboards
- Rename existing dashboards
- Set a dashboard as default
- Copy a dashboard
- Delete dashboards
- Configure **Kiosk / Auto-Swipe** settings
- Configure **Screen Standby** settings

### Kiosk / Auto-Swipe Mode

Automatically cycles through a set of dashboards at a fixed interval — useful for wall tablets and kiosk displays.

**Controls:**
- **Play/Stop button** in the topbar (visible when 2+ dashboards exist)
- **Escape** key stops kiosk mode
- A thin progress bar at the bottom of the screen shows time remaining until the next switch

**Settings** (in Dashboard Manager → Kiosk section):
| Setting | Default | Description |
|---------|---------|-------------|
| Enable on load | off | Automatically start kiosk when page loads |
| Interval (seconds) | 30 | Time to display each dashboard (5–3600) |
| Loop | on | Return to first dashboard after the last |
| Dashboards to cycle | all | Check specific dashboards to include; leave all unchecked to cycle all |

Settings are saved in browser `localStorage` (`db2_kiosk`).

### Screen Standby Mode

Dims or blanks the screen after a period of inactivity — prevents burn-in on wall-mounted displays. Any mouse movement, touch, keystroke, or click wakes the screen.

**Settings** (in Dashboard Manager → Screen Standby section):
| Setting | Default | Description |
|---------|---------|-------------|
| Enable standby | off | Enable inactivity dimming |
| Inactivity timeout | 5 min | Minutes before screen dims (1–60) |
| Full blackout | off | Completely black screen instead of dimmed |
| Opacity when dimmed | 5% | Screen brightness when dimmed (0–30%) |

Settings are saved in browser `localStorage` (`db2_standby`).

### Export / Import

The **Export / Import** button opens a modal dialog with two tabs:

**Export tab:**
- *Clipboard* — generates JSON and lets you copy it to the clipboard for sharing (e.g. in forum posts)
- *Download* — downloads the dashboard as a `.json` file
- Choose to export the **current dashboard** or **all dashboards**

The exported JSON includes the Domoticz build revision (`domoticzRevision`) so recipients know the minimum version needed.

**Import tab:**
- *Clipboard* — paste JSON directly from the clipboard
- *File* — select a `.json` file from disk
- Choose to import as a **new dashboard** or **replace current**
- If the imported dashboard was created on a newer Domoticz build, a warning is shown (but import still proceeds)

---

## Widgets

Widgets are grouped into the following categories: **Info**, **Charts & Data**, **Energy**, **Devices**, **Controls**, **Weather**, **Information**, **System**, **Custom Content**.

---

### Clock
**Category:** Info

Displays the current local time and date. No configuration required. Updates every second.

---

### Sun Info
**Category:** Info

Shows today's sunrise and sunset times, derived from the configured location in Domoticz settings. No configuration required.

---

### Weather
**Category:** Charts & Data

Displays current weather conditions with an animated weather scene background (sun, clouds, rain, snow, moon/stars at night).

**Configuration:**
| Field | Description |
|-------|-------------|
| Temperature device | Any Temp / Temp+Hum / Temp+Hum+Baro device |
| Wind device | A Wind type device |
| Barometer device | A Temp+Baro or Temp+Hum+Baro device |
| Display style | **Style 1** (default) or **Style 2** (see below) |

**Style 1:** Data-dense view with the animated weather scene as a background. Shows temperature, wind speed/direction, barometer, humidity, and forecast string as individual rows.

**Style 2:** Full-height immersive scene card. The animated scene fills the widget (sun rays / moon + stars / clouds / rain / snow), with large temperature and forecast string centered over it. Includes dew point, humidity, and barometer. Night mode activates automatically between sunset and sunrise.

Live: updates instantly from WebSocket device updates.

---

### Stat Counter
**Category:** Charts & Data

Displays a single large KPI number from any device.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Any used device |
| Label | Optional custom label (defaults to device name) |

Live: updates instantly from WebSocket `device_update` events.

---

### Temperature Graph
**Category:** Charts & Data

Highcharts chart showing temperature history.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Temperature / Temp+Hum type device |
| Chart type | See table below |
| Custom title | Optional override |

**Chart types:**
| Type | Description |
|------|-------------|
| Last 24h | Hourly temperature spline |
| Last 7 days | Daily temperature spline |
| Last month | Daily temperature spline |
| Last year | Monthly temperature spline |
| Dew Point | Temperature + dew point dual-line |
| Temp vs Humidity | Temperature and humidity with dual y-axis |
| Comfort Zone | Temperature/humidity scatter plot with comfort zone bands |

---

### Counter / Energy Chart
**Category:** Charts & Data

Column chart for counter and energy devices (kWh, Gas, Water, P1, generic counter). Automatically detects the device type and display unit.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Counter / kWh / Gas / Water / P1 Meter device |
| Chart type | See table below |
| Custom title | Optional override |

**Chart types:**
| Type | Description |
|------|-------------|
| Today | Hourly bars |
| Last week | Daily bars |
| Last month | Daily bars |
| Last year | Monthly bars |
| Compare months | Multiple years overlaid as grouped bars |

P1 Smart Meters with return capability display import and export as two separate series.

---

### Wind Chart
**Category:** Charts & Data

Wind data charts for a Wind type device.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Wind type device |
| Chart type | See table below |
| Title | Optional override |

**Chart types:**
| Type | Description |
|------|-------------|
| Last 24h | Speed + gust line chart. Tooltips show Beaufort scale notation (Calm, Light Breeze, Strong Breeze, etc.) |
| Wind Direction | Wind rose (polar chart) showing frequency per direction sector |
| Speed Frequency | Histogram of wind speed occurrences |
| Last Month | Daily average speed bars |
| Last Year | Monthly average speed bars |

---

### Rain Chart
**Category:** Charts & Data

Rain data charts for a Rain type device.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Rain type device |
| Chart type | See table below |
| Title | Optional override |

**Chart types:**
| Type | Description |
|------|-------------|
| Last 24h | Rain rate (mm/h) line + cumulative total area, dual y-axis |
| Last week | Daily total bars |
| Last month | Daily total bars |
| Last year | Monthly total bars |

---

### Gauge
**Category:** Charts & Data

Circular arc gauge showing the current value of any numeric device against a configurable min/max range with colour-coded thresholds.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Any numeric device |
| Title | device name | Optional override |
| Min | 0 | Minimum scale value |
| Max | 100 | Maximum scale value |
| Unit | % | Display unit |
| Warn threshold | 50 | Value where colour shifts from first to second zone |
| Critical threshold | 80 | Value where colour shifts from second to third zone |
| Threshold mode | low-is-good | **low-is-good** (e.g. CPU load): green → yellow → red as value rises. **high-is-good** (e.g. battery %): red → yellow → green as value rises |

The gauge is rendered as a pure SVG arc — no Highcharts required. The fill animates smoothly on value changes.

Live: refreshes every 30 seconds and on WebSocket `device_update` for the configured device.

---

### Custom Chart
**Category:** Charts & Data

Free-form Highcharts chart where you can combine any number of sensors (temperature, counter, humidity, rain, wind, etc.) on a single chart with a shared time range.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Series (JSON) | `[]` | JSON array defining each series (see format below) |
| Time range | day | Last 24h / Last 7 days / Last month / Last year |
| Title | — | Optional chart title |
| Left axis label | — | Label for the left y-axis |
| Right axis label | — | Label for the right y-axis |
| Show legend | on | Toggle chart legend |

**Series JSON format:**
```json
[
  { "idx": 123, "sensor": "temp",    "label": "Living Room", "color": "#ff6600", "axis": 1 },
  { "idx": 456, "sensor": "counter", "label": "Power (W)",   "color": "#43a4d3", "axis": 2 },
  { "idx": 789, "sensor": "humidity","label": "Humidity",    "color": "#66bb6a", "axis": 1 }
]
```

| Series field | Required | Description |
|---|---|---|
| `idx` | yes | Domoticz device index |
| `sensor` | no | `temp`, `counter`, `humidity`, `rain`, `wind`, `lux`, `uv`, `setpoint` — default `temp` |
| `label` | no | Series name in legend (defaults to "Series N") |
| `color` | no | Hex color (e.g. `"#ff6600"`); omit to use Highcharts defaults |
| `axis` | no | `1` = left y-axis (default), `2` = right y-axis |

A helpful error message is shown if the JSON is invalid. All series are fetched in parallel.

---

### kWh Summary
**Category:** Energy

Compact stat card showing current power (W) and today's energy total (kWh).

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Energy / Counter type device |
| Title | Optional override (defaults to device name) |
| Color | Token color: import (orange) / export (green) / solar (yellow) / gas (red) |

Live: refreshes every 30 seconds and on WebSocket device updates.

---

### Gas Summary
**Category:** Energy

Compact stat card showing today's gas usage and cumulative total.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Gas type device |
| Title | Optional override |

Live: refreshes every 60 seconds and on WebSocket device updates.

---

### P1 Electricity
**Category:** Energy

Dedicated widget for Dutch P1 smart meters showing current import/export power and today's totals.

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | P1 Electricity Meter device |
| Title | Optional override |
| Show price | Show today's cost (if reported by meter) |

Displays an up-arrow (exporting) or down-arrow (importing) with current watts, coloured green for export and orange for import. Sub-line shows today's totals for both directions.

Live: refreshes every 30 seconds and on WebSocket device updates.

---

### Battery Status
**Category:** Energy

Battery energy storage widget showing today's imported/exported kWh, net, and live SOC / watts / voltage.

**Configuration:**
| Field | Description |
|-------|-------------|
| Auto mode | Read device IDs from the Energy Dashboard settings |
| Battery energy in | Manual: import kWh device |
| Battery energy out | Manual: export kWh device |
| Battery SOC | Manual: state-of-charge % sensor |
| Battery watts | Manual: current power device |
| Battery voltage | Manual: voltage sensor |
| Title | Optional override |

When **Auto mode** is on, device IDs are read automatically from `getenergydashboarddevices` — zero extra configuration if the Energy Dashboard is already set up.

Live: refreshes every 60 seconds.

---

### Energy Dashboard
**Category:** Energy

Full energy overview widget — combines all cards (Weather, Grid, Solar, Gas, Battery) and the self-sufficiency balance bar in one resizable widget. Lifted directly from `forecast.html`.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Show weather card | on | Weather conditions |
| Show solar card | on | Solar power and yield |
| Show gas card | on | Gas usage |
| Show battery card | on | Battery status |
| Show balance bar | on | Self-sufficiency stats |
| Refresh interval | 60s | Seconds between data refresh |

Device IDs are read automatically from the Energy Dashboard settings (`getenergydashboarddevices`).

---

### Self-Sufficiency
**Category:** Energy

Compact bar showing energy self-sufficiency percentage and today's balance stats (house consumption, solar yield, battery net).

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Auto mode | on | Read device IDs from Energy Dashboard settings |
| Show sun times | on | Show current time, sunrise, sunset |
| Show gauge bar | on | Green→red percentage bar |
| Refresh interval | 60s | Seconds |

Self-sufficiency formula:
```
batNet           = batCharge - batDischarge
houseConsumption = p1Import + solar - p1Export - batNet
selfSufficiency  = (1 − max(0, netGrid) / houseConsumption) × 100
```
Result is clamped to 0–100%.

---

### Text Sensor
**Category:** Devices

Displays the text value (`Data` field) of a Domoticz Text sensor device.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Text sensor device |
| Show title | on | Show device name as header |
| Font size | 14px | Text size in pixels |
| Refresh interval | 60s | Seconds between refresh |

Content preserves whitespace and line breaks. Live: re-fetches immediately on WebSocket `device_update` for the configured device.

---

### Device
**Category:** Devices

Embeds any single Domoticz device using its native widget (the same widget used in the classic dashboard).

**Configuration:**
| Field | Description |
|-------|-------------|
| Device | Any device |

The background is transparent in view mode. Live: instant WebSocket updates.

---

### Scene / Group
**Category:** Devices

Embeds a Domoticz scene or group with its native widget.

**Configuration:**
| Field | Description |
|-------|-------------|
| Scene or Group | Any defined scene or group |

Live: WebSocket `scene_update` events.

---

### Favorites
**Category:** Devices

Shows all favorite devices grouped by category, reproducing the classic dashboard tabs in a single widget.

**Configuration:**
| Field | Description |
|-------|-------------|
| Category filter | All / Switches & Lights / Temperature / Weather / Utility |
| Limit to room/plan | Optional plan ID |
| Show category tabs | Toggle tab bar |
| Custom Title | Optional override |

Live: WebSocket `device_update` / `scene_update` + 60s polling fallback.

---

### Room / Plan
**Category:** Devices

Shows all devices belonging to a specific Domoticz plan (room).

**Configuration:**
| Field | Description |
|-------|-------------|
| Plan / Room ID | Numeric ID of the plan |
| Custom Title | Optional override |

Live: WebSocket `device_update` / `scene_update`.

---

### Setpoint
**Category:** Controls

Display and control a Domoticz setpoint device with +/− step buttons and a click-to-edit popup.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Setpoint type device |
| Step | 0.5 | Increment per button press |
| Min | 5 | Minimum allowed value |
| Max | 35 | Maximum allowed value |
| Unit | °C | Display unit |
| Title | device name | Optional override |

Clicking the value opens a number input popup (pre-filled with current value) for direct entry. Commands use `setdevice&idx=X&setpoint=Y`. Live: WebSocket device updates re-fetch the current value.

---

### Thermostat
**Category:** Controls

Combined widget showing current temperature from a sensor alongside setpoint controls. Ideal for room thermostats.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Temperature device | — | Temp / Temp+Hum sensor |
| Setpoint device | — | Setpoint device |
| Step | 0.5 | Setpoint increment |
| Min | 5 | Minimum setpoint |
| Max | 35 | Maximum setpoint |
| Unit | °C | Display unit |
| Title | — | Optional override |

Displays the current temperature large and prominent, with +/− setpoint controls below. Both devices are fetched in a single API call.

---

### Thermostat6
**Category:** Controls

Widget for the Thermostat6 device type. Displays all available measured values (Temp, Humidity + status, Barometer) and setpoint controls. Only rows for values present on the device are shown.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Thermostat6 device |
| Step | 0.5 | Setpoint step |
| Min | 5 | Minimum setpoint |
| Max | 35 | Maximum setpoint |
| Title | device name | Optional override |

---

### Weather Forecast
**Category:** Weather

Multi-day weather forecast using the [Open-Meteo API](https://open-meteo.com) (free, no API key required). Location is read automatically from Domoticz settings.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Days | 7 | Forecast days to display (1–14) |
| Show wind | on | Wind speed row |
| Show precipitation | on | Precipitation row |
| Title | Weather Forecast | Optional override |

Displays a horizontal grid of day columns, each with a weather icon (mapped from WMO weather codes), temperature range, precipitation (mm), and wind speed (km/h).

Data is cached for 30 minutes to avoid excessive API calls.

---

### RSS Feed
**Category:** Information

Displays items from any RSS or Atom feed.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Feed URL | — | Full URL of the RSS/Atom feed |
| Max items | 5 | Items to display (1–20) |
| Refresh interval | 300s | Seconds between fetches |
| Show images | on | Show thumbnail images |
| Show date | on | Show relative timestamp |
| Open in new tab | on | Links open in a new browser tab |
| Title | feed title | Optional override |

Fetches feeds via the [rss2json.com](https://api.rss2json.com) proxy (free tier, no API key needed) which solves CORS limitations.

**Single item mode** (`Max items: 1`): Shows the latest item with a large image, headline, and excerpt.

**Multi-item mode**: Shows a scrollable list with thumbnails, truncated titles, and relative timestamps ("2h ago", "3d ago"). Each item has a link icon to open the original article.

---

### Google Calendar
**Category:** Information

Displays upcoming events from a Google Calendar or any ICS-compatible calendar, grouped as Today / Tomorrow / upcoming days.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Calendar URL | — | Public ICS URL or Google Calendar API endpoint |
| Max events | 7 | Maximum upcoming events to show |
| Show time | on | Show event start time |
| Refresh interval | 900s | Seconds between fetches (minimum 60) |
| Title | Calendar | Optional override |

**Getting a Google Calendar ICS URL:**
In Google Calendar → Settings → click the calendar → *Integrate calendar* → copy the **Secret address in iCal format** (ends in `.ics`).

**Using the Google Calendar JSON API:**
Requires a public calendar and a Google API key:
```
https://www.googleapis.com/calendar/v3/calendars/{calendarId}/events
  ?key={apiKey}&orderBy=startTime&singleEvents=true&timeMin={now}
```

The widget auto-detects `.ics` vs JSON API URLs. CORS: Google's ICS URLs are CORS-friendly; self-hosted calendar servers may not be (a helpful error message is shown).

Past events today are shown in muted color. All-day events display without a time.

---

### System Log
**Category:** System

Displays recent Domoticz system log entries with filter toggles.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Max entries | 50 | Log lines to display |
| Show Normal | on | Show Normal (white) entries |
| Show Status | on | Show Status (blue) entries |
| Show Error | on | Show Error (red) entries |
| Auto-refresh | 10s | Seconds between refresh (0 = off) |
| Title | System Log | Optional override |

Toggle buttons in the widget header show the count per level (e.g. `N 42`, `S 8`, `E 2`) and can be clicked to show/hide each level. The Error button is coloured red when errors are present. A refresh button is also available.

---

### Moon Phase
**Category:** Information

Shows the current moon phase with a large emoji graphic, phase name, illumination percentage, and next full/new moon date. Calculated entirely in JavaScript — no external API required.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Show phase name | on | Display phase label (e.g. "Waxing Gibbous") |
| Show next event | on | Next full moon or new moon date |
| Show illumination | on | Illumination percentage |
| Title | Moon | Optional override |

Phase names: New Moon 🌑, Waxing Crescent 🌒, First Quarter 🌓, Waxing Gibbous 🌔, Full Moon 🌕, Waning Gibbous 🌖, Last Quarter 🌗, Waning Crescent 🌘.

Updates daily.

---

### Quick Actions
**Category:** Custom Content

A row of one-click buttons to trigger scenes or toggle switches.

**Configuration:**

An action list editor lets you add, reorder, and remove actions:
| Field | Description |
|-------|-------------|
| Type | Device (toggle) or Scene (activate) |
| Device/Scene | Picked from a dropdown |
| Label | Button label (auto-filled from device/scene name if blank) |

---

### Text Note
**Category:** Custom Content

Displays custom text with configurable appearance. Useful for section headers, labels, or status messages.

**Configuration:**
| Field | Description |
|-------|-------------|
| Content | The text to display |
| Font size | 8–72 px |
| Font | Arial, Verdana, Georgia, Courier New, etc. |
| Alignment | Left / Center / Right |
| Text color | Color picker |
| Text opacity | 0–100% |
| Background color | Color picker |
| Background opacity | 0–100% |

---

### HTML Widget
**Category:** Custom Content

Renders arbitrary HTML inside a sandboxed `<iframe>`. Full documents and snippets are both supported.

**Configuration:**
| Field | Description |
|-------|-------------|
| Widget Title | Optional label in the header |
| HTML Content | Full HTML or a snippet |

Security: `sandbox="allow-scripts"` — scripts run in isolation but cannot access the parent page.

---

### Camera Feed
**Category:** Custom Content

Shows a live MJPEG or snapshot image from a Domoticz-configured camera.

**Configuration:**
| Field | Description |
|-------|-------------|
| Camera | Pick from configured cameras |

---

### Remote Image
**Category:** Custom Content

Displays a remote image from a URL, with optional auto-refresh.

**Configuration:**
| Field | Description |
|-------|-------------|
| Image URL | Full URL to the image |
| Refresh interval | 0 = no refresh, otherwise seconds |
| Object fit | Contain / Cover / Fill |

---

### IFrame Embed
**Category:** Custom Content

Embeds any external website or web app in an iframe.

**Configuration:**
| Field | Description |
|-------|-------------|
| URL | The page to embed |
| Allow interaction | Enable pointer events |

Note: Many external sites block iframe embedding via `X-Frame-Options`. Works best with local network pages.

---

### Battery Monitor
**Category:** System

Shows all Domoticz devices whose battery level is at or below a configurable threshold — a maintenance overview so you know which sensors need fresh batteries.

**Configuration:**
| Field | Default | Description |
|-------|---------|-------------|
| Threshold | 25% | Show devices at or below this battery level |
| Show all | off | When on, shows every device with a battery (above threshold shown in muted style) |
| Sort by | level | Sort by battery level ascending, or by device name |
| Refresh interval | 300s | Seconds between refresh (5 minutes) |
| Title | Battery Monitor | Optional override |

Battery level colours: ≤10% red, ≤25% orange, ≤50% yellow, >50% green. When nothing is below the threshold, a green "All batteries OK" message is shown. Devices without a battery (level = 255 in Domoticz) are always excluded.

---

### Activity Log
**Category:** System

Shows the most recently updated devices, ordered by last update time.

**Configuration:**
| Field | Description |
|-------|-------------|
| Max items | 5–50 items (default 15) |

Live: reloads on every WebSocket device update and polls every 15 seconds.

---

### System Status
**Category:** System

Shows Domoticz version, build number, and hardware count. No configuration required.

---

## Live Update Reference

| Widget | Update method |
|--------|--------------|
| Clock | 1-second interval |
| Device / Scene | Instant — WebSocket |
| Stat Counter | Instant — WebSocket (no HTTP) |
| Weather | Instant — WebSocket per sensor |
| Activity Log | Instant — WebSocket |
| Favorites / Room | Instant — WebSocket + 60s fallback |
| Temperature Graph | 60s debounced reload on device update |
| Counter/Energy Chart | 60s debounced reload on device update |
| Wind / Rain Chart | 60s debounced reload |
| kWh Summary | 30s interval + instant WebSocket |
| P1 Electricity | 30s interval + instant WebSocket |
| Gas Summary | 60s interval + instant WebSocket |
| Battery Status | 60s interval |
| Energy Dashboard | 60s interval |
| Self-Sufficiency | Configurable interval (default 60s) |
| Text Sensor | Configurable interval + instant WebSocket |
| Setpoint / Thermostat | 30s interval + WebSocket |
| Thermostat6 | 30s interval + WebSocket |
| System Log | Configurable interval (default 10s) |
| Weather Forecast | 30-minute interval (cached) |
| RSS Feed | Configurable interval (default 5 min) |
| Google Calendar | Configurable interval (default 15 min) |
| Moon Phase | Daily |
| Gauge | 30s interval + instant WebSocket |
| Battery Monitor | Configurable interval (default 5 min) |
| Custom Chart | 60s debounced reload on config/range change |
| Sun Info / Text / HTML / IFrame / Image | Static |

---

## Technical Notes

### Architecture

- **AngularJS 1.x** with RequireJS (AMD) module loading
- **GridStack.js** for the drag/resize grid
- **Highcharts** for chart widgets
- Layouts stored in the Domoticz database via `savedashboardlayout` / `getdashboardlayout`
- CSS custom properties (`--dz-*`) for full theme compatibility

### Widget Development

Each widget consists of:
1. A JS file in `www/app/dashboard2/widgets/` that registers with `widgetRegistry` and defines an AngularJS directive
2. An HTML template in `www/views/dashboard2/widgets/`
3. Registration in the `define()` deps array of `Dashboard2Controller.js`

The `configSchema` array drives the settings modal automatically — supported field types: `text`, `textarea`, `number`, `boolean`, `select`, `device-picker`, `scene-picker`, `camera-picker`, `color`, `range`, `action-list`.

### Theme Tokens

All new widgets use CSS custom properties defined in `dashboard.css` for colors. Theme authors can override any of these in a custom theme:

| Token | Default | Use |
|-------|---------|-----|
| `--dz-widget-energy-import` | `#ffb300` | Grid import, orange |
| `--dz-widget-energy-export` | `#66bb6a` | Solar / export, green |
| `--dz-widget-energy-price`  | `#c8a0ff` | Electricity price, purple |
| `--dz-widget-energy-solar`  | `#ffd54f` | Solar yield, yellow |
| `--dz-widget-energy-gas`    | `#ff7043` | Gas, red-orange |
| `--dz-widget-energy-battery`| `#43a4d3` | Battery, blue |
| `--dz-widget-stat-surface`  | — | Energy card background |
| `--dz-widget-stat-muted`    | — | Muted label text |
| `--dz-widget-rss-accent`    | — | RSS feed title color |
| `--dz-widget-weather-sun`   | — | Sunny icon color |
| `--dz-widget-weather-rain`  | — | Rainy icon color |
| `--dz-widget-moon-color`    | — | Moon emoji color |

---

## Ideas for Future Widgets

| Widget | Description |
|--------|-------------|
| **Multi-Sensor Summary** | Shows multiple sensors (e.g. all room temperatures) in a compact list/grid |
| **Device History Log** | On/off timeline for a switch — shows when lights were on/off as a 24h bar chart |
| **Countdown Timer** | Counts down to a user-defined time/event with optional color thresholds |
| **Floor Plan Minimap** | Clickable floor plan showing device states overlaid |
| **Electricity Price Ticker** | Dynamic pricing (ENTSO-E / Tibber) with current €/kWh and 24h price chart |
| **Air Quality** | CO₂, PM2.5, VOC levels with color-coded thresholds |
| **Waste Calendar** | Dutch/European waste collection APIs showing next collection date |
| **Alarm Zone Status** | Door/window/motion sensor overview grouped by zone |
| **Heatmap Calendar** | Monthly calendar heatmap (GitHub-style) for energy or temperature |
