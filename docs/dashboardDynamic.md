# Dashboard Dynamic

**Revision:** 2026-04-05
**Minimum build:** 17584

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
| **Dashboard Settings…** | Open kiosk and standby settings |

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

**Dashboard Settings** (Edit → Dashboard Settings… or Dashboards → Dashboard Settings…) contains only the Kiosk and Screen Standby configuration — no layout management.

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

### Weather
**Category:** Weather

Displays current weather conditions with an animated weather scene background (sun, clouds, rain, snow, moon/stars at night).

**Configuration:**

| Field | Description |
|-------|-------------|
| Temperature device | Any Temp / Temp+Hum / Temp+Hum+Baro device |
| Wind device | A Wind type device |
| Barometer device | A Temp+Baro or Temp+Hum+Baro device |
| Display style | **Style 1** (default) or **Style 2** (see below) |
| Show panel background | Show the widget panel background (default: on) |

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
| Title | device name | Optional override |

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

**Getting a Google Calendar ICS URL:**
In Google Calendar → Settings → click the calendar → *Integrate calendar* → copy the **Secret address in iCal format** (ends in `.ics`).

**Using the Google Calendar JSON API:**
Requires a public calendar and a Google API key:
```
https://www.googleapis.com/calendar/v3/calendars/{calendarId}/events
  ?key={apiKey}&orderBy=startTime&singleEvents=true&timeMin={now}
```

The widget auto-detects `.ics` vs JSON API URLs.

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
| Selector switch | Shows current level text; click opens an inline level picker |
| Blind | Up / Close buttons, plus optional Stop button (shown automatically when the device supports it) |
| Scene | One-click activate button |

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
- Click a selector button to open the inline level picker; the current level is shown beneath the label; the active level is highlighted
- Blind Up/Stop/Close buttons are shown as a compact group with the label on the left

---

### Text Note
**Category:** Custom Content

Displays custom text with configurable appearance. Useful for section headers, labels, or status messages.

**Configuration:**

| Field | Default | Description |
|-------|---------|-------------|
| Content | — | The text to display |
| Font | Default (theme) | Default (theme), Arial, Verdana, Tahoma, Trebuchet MS, Helvetica Neue, Georgia, Times New Roman, Palatino, Courier New, Lucida Console, Impact, System UI |
| Size | 14 px | Font size in pixels (8–72) |
| Alignment | Center | Left / Center / Right |
| Style | Normal | Normal / Bold / Italic / Bold + Italic / Underline |
| Text color | rgba(255,255,255,1) | Color + opacity picker |
| Background color | rgba(0,0,0,0) | Color + opacity picker (default: transparent) |

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
| Sun Info / Text Note / HTML / Image | Static |
| Website Embed | Configurable auto-reload interval (default off) |
| Camera Feed | Configurable interval (default 5s) |

---

