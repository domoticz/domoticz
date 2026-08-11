# Dashboard Dynamic

> **Note:** This page is maintained in the [Domoticz GitHub repository](https://github.com/domoticz/domoticz/tree/development/docs). Please do not edit it directly on the Wiki.

**Revision:** 2026-05-11\
**Minimum build:** 17950

---

## Enabling Dashboard Dynamic

Dashboard Dynamic is opt-in per user. To enable it, go to **My Profile** (top-right menu) and check **Use Dynamic Dashboard**, then save. The **Dashboard** menu item will now open the dynamic dashboard instead of the classic one.

Administrators can also enable or disable it for any user via **Setup → Users**.

---

## Overview

Dashboard Dynamic is a fully modular, drag-and-drop dashboard system for Domoticz. It replaces the fixed-layout classic dashboard with a flexible, personalized workspace where users can freely compose, resize, and arrange widgets to their liking.

Key characteristics:

- Per-user persistent layouts stored in the Domoticz database
- Multiple named dashboards per user
- Real-time device updates via the existing WebSocket (livesocket)
- Fully theme-aware using CSS custom properties (`--dz-*` variables)
- Responsive across screen sizes

---

## Getting Started

Navigate to the **Dashboard** menu item. On first visit, a starter layout is created automatically containing a Clock, Sun Info, and Activity Log widget.

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
| ⓘ (info icon) | Opens the Dynamic Dashboard wiki page |
| Dashboard title | Click to rename inline |
| **Dashboards** | Switch between dashboards, create a new one, or open Dashboard Settings |
| **Edit** | Access layout actions (Set as Default, Export/Import, Duplicate, Delete, Clear) |
| **Done Editing** | Save changes and exit edit mode |
| **Add Widget** | Open the widget library panel |
| **Save** | Manually save current layout |
| **Cancel** | Discard unsaved changes (button turns red when there are unsaved changes) |

**Dashboards dropdown:**

| Item | Action |
|------|--------|
| *(dashboard name)* | Switch to that dashboard (prompts to discard unsaved changes if needed) |
| **New Dashboard** | Creates a new dashboard with an auto-generated name; rename it via the title field |
| **Dashboard Settings…** | Open kiosk, standby and swipe settings |

**Edit dropdown:**

| Item | Action |
|------|--------|
| **Export / Import** | Open the export/import dialog |
| **Reset to Favorites** | Populate the dashboard from your favorite devices |
| **Set as Default** | Mark this dashboard as the default (shown with a checkmark when active) |
| **Duplicate** | Clone the current dashboard |
| **Delete** | Delete the current dashboard |
| **Clear All Widgets** | Remove all widgets from the current dashboard |

**Keyboard shortcuts:**

- `Ctrl+E` — toggle edit mode
- `Ctrl+S` — save layout
- `Ctrl+L` — toggle widget library panel
- `Escape` — close widget library (if open), otherwise exit edit mode; also stops Kiosk mode

### Widget Library

Click **Add Widget** to open the slide-in library panel on the right. Widgets are grouped by category. Use the search box to filter. Click a widget's **+** button to add it to the grid.

### Widget Actions (Edit Mode)

Each widget shows a header bar in edit mode with:

- **Drag handle** — drag the title bar to reposition
- **Configure** (gear icon) — open widget settings
- **Duplicate** (copy icon) — clone the widget; the clone starts dragging immediately
- **Remove** (× icon) — delete the widget

Resize any widget by dragging the bottom-right corner handle.

> **Note:** All interactive elements inside widget content (buttons, links, inputs) are non-clickable in Edit Mode — only the header chrome buttons (configure, duplicate, remove) remain active. This prevents accidental device control while rearranging the layout.

---

## Managing Dashboards

### Multiple Dashboards

Dashboard management is done directly from the Edit Mode toolbar:

| Action | How |
|--------|-----|
| **Create** | Dashboards → New Dashboard (auto-names; rename inline) |
| **Rename** | Click the dashboard title in the toolbar |
| **Switch** | Dashboards dropdown → pick a name |
| **Set as Default** | Edit → Set as Default |
| **Duplicate** | Edit → Duplicate |
| **Delete** | Edit → Delete |
| **Copy deep-link** | Edit Mode → click the chain icon (`🔗`) next to the dashboard title |

**Dashboard Settings** (Edit → Dashboard Settings… or Dashboards → Dashboard Settings…) contains only the Kiosk, Screen Standby and Swipe Navigation configuration — no layout management.

### Deep-linking to a specific dashboard

You can open any dashboard directly via the URL, bypassing the normal "last viewed / default" lookup:

| URL | Opens |
|-----|-------|
| `#/Dashboard` | Last viewed (localStorage) → default → first dashboard (existing behaviour) |
| `#/Dashboard?id=<uuid>` | The dashboard with the matching UUID |
| `#/Dashboard?name=<name>` | The dashboard whose name matches (case-insensitive) — e.g. `#/Dashboard?name=Smart%20Meter%20(P1)` |

**Getting the UUID:** in Edit Mode, click the chain icon (`🔗`) immediately to the right of the dashboard title in the toolbar. The full deep-link URL is copied to the clipboard with a toast confirmation. Paste it into a browser bookmark, a wall-tablet shortcut, a chat message, etc.

Switching dashboards through the dropdown does **not** update the URL — the deep-link only takes effect on page load.

### Kiosk / Auto-Swipe Mode

Automatically cycles through a set of dashboards at a fixed interval — useful for wall tablets and kiosk displays.

**Controls:**

- **Play/Stop button** in the topbar (visible when 2+ dashboards exist)
- **Escape** key stops kiosk mode
- A thin progress bar at the bottom of the screen shows time remaining until the next switch

**Settings** (in Dashboard Settings → Kiosk section):

| Setting | Default | Description |
|---------|---------|-------------|
| Enable on load | off | Automatically start kiosk when page loads |
| Interval (seconds) | 30 | Time to display each dashboard (5–3600) |
| Loop | on | Return to first dashboard after the last |
| Dashboards to cycle | all | Check specific dashboards to include; leave all unchecked to cycle all |

Settings are saved in browser `localStorage` (`dd_kiosk`).

### Screen Standby Mode

Dims or blanks the screen after a period of inactivity — prevents burn-in on wall-mounted displays. Any mouse movement, touch, keystroke, or click wakes the screen.

**Settings** (in Dashboard Settings → Screen Standby section):

| Setting | Default | Description |
|---------|---------|-------------|
| Enable standby | off | Enable inactivity dimming |
| Inactivity timeout | 5 min | Minutes before screen dims (1–60) |
| Full blackout | off | Completely black screen instead of dimmed |
| Opacity when dimmed | 5% | Screen brightness when dimmed (0–30%) |

Settings are saved in browser `localStorage` (`dd_standby`).

### Swipe Navigation

When enabled, swiping left or right on a touch device (tablet/phone) moves to the next or previous dashboard, using the dashboard order. The gesture is ignored while editing or when a dialog is open, and does not interfere with vertical scrolling.

**Note:** Swipe navigation has no effect while in Edit Mode, when a dialog (such as Dashboard Settings) is open, or when only one dashboard exists.

**Settings** (in Dashboard Settings → Swipe Navigation section):

| Setting | Default | Description |
|---------|---------|-------------|
| Swipe left/right to switch dashboard | off | Enable horizontal swipe navigation on touch devices |

Settings are saved in browser `localStorage` (`dd_swipe`).

### Per-Device Dashboard Selection

Each browser/device independently remembers which dashboard it last viewed. When you switch dashboards on a phone, tablet, or desktop, that selection is saved in the browser's `localStorage` — so the next time that specific device opens Domoticz, it returns to the same dashboard automatically. This means a wall tablet can always open to a "Living Room" dashboard while your phone opens to a different one, without any server-side configuration.

The active dashboard selection is stored per-origin in `localStorage` key `dd_active_dashboard`.

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

## Bar Indicators

Classic tab device widgets (Utility, Weather, Temperature) support optional color-coded bar indicators — a thin progress bar rendered below each device's last-update timestamp. Bars provide an at-a-glance value status using configurable color ranges (e.g. green for normal values, red for high).

### Configuring a Bar

1. Open a device's **Edit** dialog (Admin permission required) on the Utility, Weather, or Temperature tab.
2. Click the **bar chart icon** (![bar icon]) in the top-right corner of the dialog form.
3. In the **Bar Ranges** popup, define one or more ranges:
   - **From / To** — numeric bounds for this range segment
   - **Color** — the fill color for this segment (color picker)
   - Click **+** to add; drag or use the trash icon to remove
4. Click **Save** in the popup, then **Update** in the Edit dialog to persist.

Ranges are stored in the device's `Color` database field as a keyed JSON object, so each sensor type on a multi-sensor device (e.g. Temp+Hum+Baro) stores its bar configuration independently. Editing the bar from the Temperature tab configures the temperature bar; editing from the Weather tab configures the barometer bar — they do not overwrite each other.

### Supported Device Types

**Utility tab** — all numeric sensor types: Electric usage, kWh energy, Percentage, Gas/Counter, Custom Sensor, Lux, Voltage, Current, Setpoint, Air Quality, Pressure, Distance, Weight, Sound Level, Waterflow, Fan, Leaf Wetness, Soil Moisture, A/D, Power, Energy, Current/Energy, Radiator 1.

**Weather tab** — all six device types: Barometer (value: pressure in hPa), Rain (value: mm), Wind (value: speed), UV (value: UVI), Visibility (value: distance), Radiation (value: from Data field).

**Temperature tab** — temperature sensors (value: °C/°F), humidity sensors (value: %), Temp+Humidity, Temp+Humidity+Baro, Wind+Temp/Chill.

### Bar Display

Configured bars appear:
- On the respective tab (Utility / Weather / Temperature)
- On the classic dashboard for favorited devices (both desktop and mobile views)

The bar fills proportionally across the defined ranges and renders a linear gradient through the configured colors.

---

## Widgets

Widgets are grouped into the following categories: **Info**, **Charts & Data**, **Energy**, **Devices**, **Controls**, **Weather**, **Information**, **System**, **Custom Content**.

---

### Clock
**Category:** Custom Content

Displays the current local time and date. Updates every second.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Show seconds | on | Include seconds in the time display |
| 24-hour format | on | Use 24h clock; off for 12h with AM/PM |
| Show date | on | Show the date line below the time |
| Timezone | local | Timezone for the displayed time; search by city or region name |
| Show panel background | on | Show the widget panel background |

---

### Sun Info
**Category:** Weather

Shows today's sunrise and sunset times, derived from the configured location in Domoticz settings.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Show panel background | on | Show the widget panel background |

---

### Barometer
**Category:** Weather

Displays barometric pressure and weather forecast string from a barometer device.

**Configuration:**

| Field | Description |
|-------|-------------|
| Barometer device | Any Temp+Baro or Temp+Hum+Baro device |
| Custom Title | Optional override |
| Show panel background | Show the widget panel background (default: on) |

Live: updates on WebSocket `device_update` for the configured device.

---

### Weather
**Category:** Weather

Displays current weather conditions with an animated weather scene background (sun, clouds, rain, snow, thunderstorm with lightning bolt, moon/stars at night).

**Configuration:**

| Field | Description |
|-------|-------------|
| Temperature device | Any Temp / Temp+Hum / Temp+Hum+Baro device |
| Wind device | A Wind type device |
| Barometer device | A Temp+Baro or Temp+Hum+Baro device |
| Display style | **Style 1** (default) or **Style 2** (see below) |
| Show panel background | Show the widget panel background (default: on) |

**Style 1:** Data-dense view with the animated weather scene as a background. Shows temperature, wind speed/direction, barometer, humidity, and forecast string as individual rows.

**Style 2:** Full-height immersive scene card. The animated scene fills the widget (sun rays / moon + stars / clouds / rain / snow / thunderstorm with lightning bolt), with large temperature and forecast string centered over it. Includes dew point, humidity, and barometer. Night mode activates automatically between sunset and sunrise.

Live: updates instantly from WebSocket device updates.

---

### Stat Counter
**Category:** Charts & Data

Displays a single large KPI number from any device. Clicking the value navigates to the device log.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Any used device |
| Label | — | Optional custom label (defaults to device name) |
| Show panel background | on | Show the widget panel background |

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
| Show panel background | Show the widget panel background (default: on) |

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

Column or area chart for counter and energy devices (kWh, Gas, Water, P1, generic counter). Automatically detects the device type and display unit.

**Configuration:**

| Field | Description |
|-------|-------------|
| Device | Counter / kWh / Gas / Water / P1 Meter device |
| Chart type | See table below |
| Custom title | Optional override |
| Bar color | Color for single-series counter charts (gas, water, non-P1 kWh). Defaults to cyan `#03befc`, matching the P1 "Usage" series. Ignored for P1 dual-series charts (which use fixed usage/return colors) |
| Show panel background | Show the widget panel background (default: on) |

**Chart types:**

| Type | Description |
|------|-------------|
| Short Log | High-resolution bars covering the last 2–3 days. For P1 meters: area chart in Watts. For all other types (water, gas, kWh): column bars in the device's native unit. Chart title shows the actual day span (e.g. "Last 3 Days") |
| Today (hourly) | Hourly bars over the rolling last 24 hours, matching the Domoticz log page's "Today" view |
| Last week | Daily bars |
| Last month | Daily bars |
| Last year | Monthly bars |
| Compare years | Multiple years overlaid as grouped bars per month |

P1 Smart Meters with return capability display import and export as two separate series.

**Zooming:** Drag horizontally on any chart to zoom in on a time range. A *Reset zoom* button appears to return to the full view.

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
| Show panel background | Show the widget panel background (default: on) |

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
| Show panel background | Show the widget panel background (default: on) |

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
| Show panel background | on | Show the widget panel background |

The gauge is rendered as a pure SVG arc — no Highcharts required. The fill animates smoothly on value changes. Clicking the value or unit text navigates to the device log.

Live: refreshes every 30 seconds and on WebSocket `device_update` for the configured device.

---

### Custom Chart
**Category:** Charts & Data

Free-form Highcharts chart where you can combine up to 10 sensors (temperature, counter, humidity, rain, wind, etc.) on a single chart with a shared time range.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Time range | Last 24h | Last 24h / Last 7 days / Last month / Last year |
| Title | — | Optional chart title |
| Show legend | on | Toggle chart legend |
| Device 1–10 | — | Up to 10 device pickers; each added device becomes one series |

The sensor type and y-axis unit are detected automatically from the device type. Series with matching units share a y-axis; incompatible units get separate axes.

---

### kWh Summary
**Category:** Energy

Compact stat card showing current power (W) and today's energy total (kWh). Clicking the current power (W) value navigates to the device log.

**Configuration:**

| Field | Description |
|-------|-------------|
| Device | Energy / Counter type device |
| Title | Optional override (defaults to device name) |
| Color | Token color: import (orange) / export (green) / solar (yellow) / gas (red) |
| Show panel background | Show the widget panel background (default: on) |

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
| Show panel background | Show the widget panel background (default: on) |

Live: refreshes every 60 seconds and on WebSocket device updates.

---

### Water Summary
**Category:** Energy

Compact stat card showing today's water usage, year-to-date total, and optionally today's cost.

**Configuration:**

| Field | Description |
|-------|-------------|
| Device | Water meter device |
| Title | Optional override (defaults to device name) |
| Show panel background | Show the widget panel background (default: on) |

Displays today's usage (in Liters or m³ depending on device configuration), the year-to-date total derived from the annual graph, and the day's water cost if the price is configured (price = 1000 is treated as "not configured" and hidden).

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
| Show panel background | Show the widget panel background (default: on) |

Displays an up-arrow (exporting) or down-arrow (importing) with current watts, coloured green for export and orange for import. Sub-line shows today's totals for both directions.

Live: refreshes every 30 seconds and on WebSocket device updates.

---

### Battery Status
**Category:** Energy

Battery energy storage widget showing today's imported/exported kWh, net, and live SOC / watts / voltage. Clicking any value navigates to the corresponding device log.

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
| Show panel background | Show the widget panel background (default: on) |

When **Auto mode** is on, device IDs are read automatically from `getenergydashboarddevices` — zero extra configuration if the Energy Dashboard is already set up.

Live: refreshes every 60 seconds.

---

### Energy Dashboard
**Category:** Energy

Full energy overview widget — combines all cards (Weather, Grid, Solar, Gas, Battery) and the self-sufficiency balance bar in one resizable widget. Lifted directly from `forecast.html`. Clicking any value on a card navigates to the corresponding device log.

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

Compact bar showing energy self-sufficiency percentage and today's balance stats (house consumption, solar yield, battery net). Device IDs are read automatically from the Energy Dashboard settings.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Show gauge bar | on | Green→red percentage bar |
| Refresh interval | 60s | Seconds between data refresh |

Self-sufficiency formula:
```
batNet           = batCharge - batDischarge
houseConsumption = p1Import + solar - p1Export - batNet
selfSufficiency  = (solar + batDischarge) / houseConsumption × 100
```
Result is clamped to 0–100%. Battery discharge counts as local generation — energy stored earlier from solar and discharged later correctly reduces grid dependence.

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
| Show panel background | on | Show the widget panel background |

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

Shows all devices belonging to a specific Domoticz plan (room), including scenes and groups. Supports two display modes selectable in settings.

**Grid mode (default):** Each device is rendered using its native widget tile — the same tiles used in the classic dashboard.

**List mode:** Displays all devices as a compact vertical list. Interactive device types are fully controllable:

| Type | Behaviour |
|------|-----------|
| Switch / Scene | Toggle button; icon tinted accent when device is On. Clicking the icon navigates to the device log |
| Dimmer | Label + slider icon + power toggle. Slider icon opens inline dim slider; power icon toggles On/Off. Clicking the icon navigates to the device log |
| Selector switch | Shows current level; click opens inline level picker. Clicking the icon navigates to the device log |
| Blind | Up / Close buttons, plus Stop if the device supports it. Clicking the icon navigates to the device log |
| Group | On and Off buttons as a compact pair. Clicking the icon navigates to the device log |
| Security Panel | Disarm / Arm Home / Arm Away buttons. Clicking the icon navigates to the device log |
| Sensor / Utility | Read-only row: icon, device name, current value and unit. Clicking the icon navigates to the device log. P1 smart meters show two lines: usage + actual power on the first line, return energy on the second |

**Configuration:**

| Field | Description |
|-------|-------------|
| Room / Plan | Select the Domoticz plan to display |
| List layout | Toggle between grid (default) and list mode |
| Custom Title | Optional override |

Live: WebSocket `device_update` / `scene_update` + 60s polling fallback.

---

### Setpoint
**Category:** Controls

Display and control a Domoticz setpoint device with +/− step buttons and a click-to-edit popup.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Setpoint type device |
| Title | device name | Optional override |
| Show panel background | on | Show the widget panel background |

Step, min, max, and unit are read automatically from the device. Clicking the value opens a number input popup (pre-filled with current value) for direct entry. Commands use `setdevice&idx=X&setpoint=Y`. Live: WebSocket device updates re-fetch the current value.

---

### Thermostat
**Category:** Controls

Combined widget showing current temperature from a sensor alongside setpoint controls. Ideal for room thermostats.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Temperature sensor | — | Temp / Temp+Hum sensor |
| Setpoint device | — | Setpoint device |
| Title | — | Optional override |
| Show panel background | on | Show the widget panel background |

Step, min, max, and unit are read automatically from the setpoint device. Displays the current temperature large and prominent, with +/− setpoint controls below. Both devices are fetched in a single API call.

---

### Thermostat6
**Category:** Controls

Widget for the Thermostat6 device type. Displays all available measured values (Temp, Humidity + status, Barometer) and setpoint controls. Only rows for values present on the device are shown.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Device | — | Thermostat6 device |
| Title | device name | Optional override |
| Show panel background | on | Show the widget panel background |

Step, min, and max are read automatically from the device.

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
| Show panel background | on | Show the widget panel background |

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
| Show panel background | on | Show the widget panel background |

**Getting a Google Calendar ICS URL:**
In Google Calendar → Settings → click the calendar → *Integrate calendar* → copy the **Secret address in iCal format** (ends in `.ics`).

**Using the Google Calendar JSON API:**
Requires a public calendar and a Google API key:
```
https://www.googleapis.com/calendar/v3/calendars/{calendarId}/events
  ?key={apiKey}&orderBy=startTime&singleEvents=true&timeMin={now}
```

The widget auto-detects `.ics` vs JSON API URLs.

**Recurring events:** ICS recurring events (RRULE) are expanded into their upcoming occurrences (60-day lookahead). Supported: daily / weekly / monthly / yearly frequencies with `INTERVAL`, `COUNT`, `UNTIL`, `BYDAY` (weekly) and `BYMONTHDAY`; excluded dates (`EXDATE`) and single occurrences that were moved or cancelled (`RECURRENCE-ID`) are honored.

**CORS handling:** The widget fetches calendar data using a fallback chain:

1. Direct request (works for CORS-friendly URLs)
2. Domoticz server-side proxy (`json.htm?type=command&param=fetchurl&url=<encoded>`) — avoids CORS entirely for any URL reachable from the Domoticz server
3. Public CORS proxies (corsproxy.io / codetabs) as last resort

A helpful error message is shown if all methods fail.

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

One-click buttons to trigger scenes and control switch devices, displayed in a grid or list layout.

**Supported device types:**

| Type | Behaviour |
|------|-----------|
| Switch | Toggle button; icon tinted when device is On |
| Push On / Push Off | Dedicated On or Off button; no toggle, no state indicator |
| Dimmer | Label + slider icon + power toggle. Click the slider icon to open an inline horizontal dim slider; drag to set level. Click the power button to toggle On/Off. Icon tinted when device is On |
| Selector switch | Shows current level text; click opens an inline level picker |
| Blind | Up / Close buttons, plus optional Stop button (shown automatically when the device supports it) |
| Group | On and Off buttons shown as a compact pair |
| Scene | One-click activate button |
| Security Panel | Navigates to the Domoticz security panel page. Current arm status shown on the right; icon tinted when armed |

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| List layout | off | Display actions as a vertical list (label left, icon/status right) instead of a grid |
| Show panel background | on | Show the widget panel background |

**Action list editor:**

Use the action list to add, reorder, and remove actions. Drag the grip handle (⠿) to reorder. Click the pencil icon to rename; click the trash icon to delete.

To add an action:

1. Select **Device** or **Scene** from the type dropdown
2. Pick the device or scene from the searchable picker (only switch-type devices are shown)
3. For **selector switches**: the level is *not* chosen here — it is selected at click-time in the widget
4. Optionally enter a custom label (auto-filled from device/scene name if blank)
5. Click **+** to add

**In the widget:**

- Click a switch button to toggle it; the icon turns accent-coloured when the device is On
- Click a dimmer's slider icon to reveal the inline horizontal slider; drag to set dim level; click anywhere outside to dismiss without sending a command
- Click a selector button to open the inline level picker; the current level is shown beneath the label; the active level is highlighted
- Blind Up/Stop/Close buttons are shown as a compact group with the label on the left
- Group On/Off buttons are shown as a compact pair with the label on the left
- Click a Security Panel button to open the security panel page; the current arm status is shown to the right of the label

---

### Quick Stat
**Category:** Custom Content

Compact status panel showing current values for any mix of devices — temperature, switches, kWh meters, humidity, and more. Useful as a at-a-glance status overview.

Each row shows an icon, device name (or custom label), current value, and unit. Switch/light states are highlighted in accent colour when On. Clicking the icon navigates to the device log.

**Display modes:**

- **Grid mode (default):** Devices shown as small cards in a responsive grid (auto-fill, min 110px)
- **List mode:** Devices shown as full-width rows — icon left, name, value + unit right

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| List layout | off | Display as vertical list instead of a grid |
| Show panel background | on | Show the widget panel background |
| Devices | — | Add any mix of devices using the device list editor. Drag the grip handle to reorder; pencil to rename; trash to remove |

Live: updates instantly from WebSocket `device_update` events for all configured devices.

---

### Text Note
**Category:** Custom Content

Displays custom text with configurable appearance. Useful for section headers, labels, or status messages. The content is rendered as sanitized HTML (DOMPurify with a strict allow-list), so a safe subset of inline markup — `<b>`, `<i>`, `<span>`, `<div>`, `<a>`, basic tables, headings, lists, and scoped `<style>` blocks — works for icons, color highlights, and small layouts. Scripts, iframes, and event handlers are stripped. An optional divider line can be shown alongside the text, making it easy to create visual section separators.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Content | — | The text or sanitized HTML to display |
| Font | Default (theme) | Default (theme), Arial, Verdana, Tahoma, Trebuchet MS, Helvetica Neue, Georgia, Times New Roman, Palatino, Courier New, Lucida Console, Impact, System UI |
| Size | 14 px | Font size in pixels (8–72) |
| Horizontal align | Center | Left / Center / Right |
| Vertical align | Middle | Top / Middle / Bottom — independent of horizontal alignment; the divider follows the chosen side |
| Style | Normal | Normal / Bold / Italic / Bold + Italic / Underline |
| Text color | rgba(255,255,255,1) | Color + opacity picker |
| Background color | rgba(0,0,0,0) | Color + opacity picker (default: transparent) |
| Show divider line | off | Show a horizontal line alongside the text |
| Divider color | Accent color | Color + opacity picker; leave empty to use the theme accent color |

---

### HTML Widget
**Category:** Custom Content

Renders arbitrary HTML inside a sandboxed `<iframe>`. Full documents (`<html>…</html>`) and plain snippets are both supported.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Widget Title | — | Optional label overlaid at the top of the widget |
| HTML Content | — | Full HTML document or a snippet |
| Allow backend API access | off | Adds `allow-same-origin` to the sandbox, enabling scripts to call `json.htm` and other Domoticz API endpoints |

**Security:** By default the iframe runs with `sandbox="allow-scripts"` — scripts execute in isolation and cannot access cookies, localStorage, or the parent page. Enabling **Allow backend API access** lifts the origin restriction so scripts can make authenticated requests to the Domoticz backend. Only enable this for HTML you wrote yourself.

---

### Camera Feed
**Category:** Controls

Shows a live snapshot image from a Domoticz-configured camera.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Camera | — | Pick from configured cameras |
| Refresh interval | 5s | Seconds between snapshot refreshes |
| Show camera name | on | Display the camera name in the widget header |

---

### Remote Image
**Category:** Custom Content

Displays a remote image from a URL, with optional auto-refresh.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Image URL | — | Full URL to the image |
| Caption | — | Optional label shown over the image |
| Fit | Contain | Contain (full image visible) / Cover (fill area, crop) / Stretch to fill |
| Refresh interval | 0 | Seconds between reloads; 0 = no auto-refresh |
| Show panel background | on | Show the widget panel background |

---

### Website Embed
**Category:** Custom Content

Embeds any external website or web app in an iframe.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| URL | — | The page to embed (https:// recommended) |
| Title | — | Optional widget header label |
| Allow scripts | off | Enable JavaScript in the embedded page (trusted sources only) |
| Auto-reload | 0 | Seconds between iframe reloads; 0 = off |

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

### Timeout Monitor
**Category:** System

Shows all Domoticz devices that have not reported within their expected timeout interval — a quick way to spot sensors that have gone silent or lost power.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Title | *(empty)* | Optional title override |
| Sort by | Last update | Sort by last update time (oldest first) or device name (A–Z) |
| Refresh interval | 300s | Seconds between refresh |

When no devices are timed out, nothing is shown in the list. The device list shows each device name and how long ago it last reported (e.g. "3h ago").

---

### kWh Top Consumers
**Category:** System

Lists kWh-metered devices ranked by today's energy consumption, highest first. Useful for spotting which appliances are using the most energy on any given day.

Each row shows the device name, current power draw (W), and today's total (kWh). The list updates instantly via WebSocket when any tracked device reports — including devices currently off-screen due to the row limit, which may move up the ranking in real time.

**Excluded automatically:**
- Devices from P1 Smart Meter hardware (L1/L2/L3 phase sensors, both serial and LAN variants)
- Devices configured as type **Return / Energy Generated** (e.g. solar export meters)
- Devices that have timed out

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Title | *(empty)* | Optional title override |
| Max devices to show | 20 | Maximum number of rows displayed |
| Exclude device IDX | *(empty)* | Semicolon-separated list of device IDX values to hide (e.g. `42;107`) |
| Refresh interval | 300s | Seconds between full refresh from the backend |
| Show panel background | on | Show the widget panel background |

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

Shows Domoticz version, build number, and hardware count.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Show panel background | on | Show the widget panel background |

---

## Live Update Reference

| Widget | Update method |
|--------|--------------|
| Clock | 1-second interval |
| Device / Scene | Instant — WebSocket |
| Stat Counter | Instant — WebSocket (no HTTP) |
| Weather | Instant — WebSocket per sensor |
| Barometer | Instant — WebSocket |
| Activity Log | Instant — WebSocket |
| Favorites / Room | Instant — WebSocket + 60s fallback |
| Temperature Graph | 60s debounced reload on device update |
| Counter/Energy Chart | 60s debounced reload on device update |
| Wind / Rain Chart | 60s debounced reload |
| kWh Summary | 30s interval + instant WebSocket |
| P1 Electricity | 30s interval + instant WebSocket |
| Gas Summary | 60s interval + instant WebSocket |
| Water Summary | 60s interval + instant WebSocket |
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
| Timeout Monitor | Configurable interval (default 5 min) |
| kWh Top Consumers | Configurable interval (default 5 min) + instant WebSocket per tracked device |
| Custom Chart | 60s debounced reload on config/range change |
| Quick Stat | Instant — WebSocket per device |
| Sun Info / Text Note / HTML / Image | Static |
| Website Embed | Configurable auto-reload interval (default off) |
| Camera Feed | Configurable interval (default 5s) |

---

