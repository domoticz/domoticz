# Shelly (local)

> **Note:** This page is maintained in the [Domoticz GitHub repository](https://github.com/domoticz/domoticz/tree/development/docs). Please do not edit it directly on the Wiki.

**Revision:** 2026-11-23

The hardware type **Shelly devices (local)** talks directly to Shelly devices on your LAN. It needs no MQTT broker and no cloud account. One hardware instance manages many Shelly devices.

* Gen1 (block devices): HTTP REST for polling and control, optional CoIoT (CoAP over UDP) for push updates. Basic authentication.
* Gen2, Gen3 and Gen4 (RPC devices, Plus, Pro, Mini and newer): JSON-RPC over HTTP for polling and control, a WebSocket for push updates. Digest authentication.

Both generations answer `GET /shelly` with an identity document. Domoticz uses it to detect the generation, the model and whether a password is required.

## Supported devices

The mapping follows the Shelly component type (Gen2+) or block type (Gen1), not a table of models, so new models usually work without a code change. Components Domoticz does not know yet are skipped and listed once in the log at debug level.

| Area | Gen1 | Gen2+ |
|------|------|-------|
| Relays, switches | relay blocks | `switch:n` |
| Dimmers, lights | `lights[]` white mode | `light:n` |
| Colour (RGB, RGBW, bulbs) | `lights[]` color mode, RGBW2, Bulb | `rgb:n`, `rgbw:n` |
| Covers, rollers | `rollers[]` | `cover:n` |
| Energy meters | `meters[]`, `emeters[]` | `pm1`, `em1`/`em1data`, `em`/`emdata` (3 phase) |
| Inputs | `inputs[]` | `input:n` |
| Thermostats, TRV | `thermostats[]` | `thermostat:n` |
| Sensors | H&T, Door/Window, Flood, Motion, Smoke, Gas | `temperature`, `humidity`, `illuminance`, `smoke`, `flood`, `motion`, `devicepower`, `valve` |

Battery powered devices are supported, see [Sleepy battery devices](#sleepy-battery-devices).

## Adding devices

1. Open **Setup > Hardware** and add hardware of type **Shelly devices (local)**.
2. Fill in the poll interval (seconds, default 30), the optional Gen1 push settings and, when the Shelly devices are protected, the credentials (see [Authentication](#authentication)). These settings are only on the hardware page.
3. Save the hardware. The device list of the hardware is managed with the buttons on its setup page.

Two ways to add a device:

* **Manual IP**: enter the IP address (or host name) and the port (default 80) of the Shelly. Domoticz probes `GET /shelly` and refuses anything that is not a Shelly. An optional name overrides the name configured on the device.
* **Subnet scan**: enter a LAN range in CIDR notation (for example `192.168.1.0/24`) or as a range. The scan sends one unicast `GET /shelly` per address, 24 at a time with a 2 second timeout, and lists the Shelly devices it finds. Nothing is added until you pick the devices, and the name of each found device can be edited before you press Add. The range is limited to 1024 addresses (a prefix of /22 or longer) and only private (10.x.x.x, 172.16-31.x.x, 192.168.x.x), link-local (169.254.x.x) and loopback (127.x.x.x) addresses are scanned. A range must stay inside one of these blocks.

A device is identified by its MAC address. Adding the same device twice is refused. Select a device in the list to change its name (the Domoticz devices that still carry the old name are renamed with it) or its address (the new address must answer with the same MAC). Removing a device from the hardware (or clearing all devices) also deletes its Domoticz devices, including their history. The temperature and humidity sensors use an id that is shared between devices whose MAC addresses end in the same 16 bits, so they are only deleted together with the device that owns that id, or when no other device uses it.

Domoticz needs "Accept new hardware devices" enabled (Settings) to create devices.

## Authentication

* **Gen1**: set a user name and password on the hardware (HTTP basic authentication). Leave both empty when the Shelly has no restriction.
* **Gen2+**: digest authentication. The user is always `admin`, so only the password is used. The user name field is ignored for these devices.

All devices of one hardware instance share one set of credentials. If your devices have different passwords, add one hardware per password. A device that answers 401 is logged as unauthorized and is not marked unreachable. Saving the hardware with an empty password keeps the stored one; use the clear password option to remove it.

### Gen2 user admin

Gen2+ devices have a single user, `admin`. To set or change the password use the Shelly web page of the device (Settings > Authentication) or the RPC method `Shelly.SetAuth`:

```
POST http://<ip>/rpc
{"id":1,"method":"Shelly.SetAuth","params":{"user":"admin","realm":"<device id>","ha1":"<sha256 of admin:<device id>:<password>>"}}
```

The realm is the device id reported by `GET /shelly` (for example `shellyplus1-aabbccddeeff`). Gen1 devices are configured under Internet & Security > Restrict login.

## Device mapping

Every Domoticz device made for a Shelly is identified by a **node key** (the low 24 bits of the MAC) and a **unit** (one byte: `(kind << 4) | slot`, where the slot is the component channel 0..15).

| Kind | Domoticz device | Made from | Controllable |
|------|-----------------|-----------|--------------|
| 1 Switch | Lighting2 on/off switch | `switch:n`, relay | yes |
| 2 Dimmer | Lighting2 with level (dimmer) | `light:n` (slots 0..7), `cct:n` (slots 8..15), white channel of a light | yes |
| 3 Blind | blind with percentage, open/close/stop | `cover:n`, roller | yes |
| 4 Energy | kWh meter (power W and energy counter) | pm1, em1, em, per switch/cover/light meter | no |
| 5 Power | usage W | per phase power where there is no counter | no |
| 6 Voltage | voltage V | meters | no |
| 7 Current | current A | meters | no |
| 8 Frequency | custom sensor Hz | meters | no |
| 9 Power factor | percentage sensor | meters | no |
| 10 Temperature | temperature | the device temperature | no |
| 11 Color | colour switch (RGB, RGBW) | `rgb:n`, `rgbw:n`, Gen1 colour lights and bulbs | yes |
| 12 Thermostat | setpoint plus temperature | `thermostat:n`, Gen1 thermostats | yes |
| 13 Input | read-only contact, or a push button selector | `input:n`, inputs | no |
| 14 Binary | read-only on/off sensor (door/window, motion, flood, smoke, vibration, gas, valve) | sensor components | no |
| 15 Sensor | temperature and humidity, lux, gas ppm, tilt, returned energy | sensor components, meters | no |

### DeviceID and unit scheme

The node key ends up in the DeviceID, in a form that depends on the Domoticz device type:

| Domoticz type | DeviceID |
|---------------|----------|
| Lighting2 (switch, dimmer) | `0` + key as 6 hex digits, unit in the Unit column |
| Thermostat, lux, usage (W) | `0` + key as 6 hex digits, unit in the Unit column |
| GeneralSwitch (blind, input contact, selector, binary sensors) and colour switch | key as 8 hex digits, unit in the Unit column |
| General meters (energy, voltage, current, custom sensors, returned energy) | `(key << 8) \| unit` as 8 hex digits |
| Temperature, humidity, temperature+humidity | the low 16 bits of the key only (a limit of those Domoticz types) |

Two Shelly devices that share the same low 16 bits of the MAC (about 1 in 65536 for random pairs) can both be added, but only the one whose MAC sorts lowest gets the temperature and humidity devices. This is logged.

Because the MAC cannot be recovered from a DeviceID, a command for a device is routed back by comparing the node key with the devices of the hardware.

### What each component becomes

* **Switch and relay**: a Lighting2 switch. A Gen1 device in roller mode gets a blind instead of switches.
* **Dimmer and light**: a dimmer with a level 0..100.
* **CCT light** (`cct:n`, for example the Duo bulb): a dimmer in the Dimmer slots 8..15 (channel plus 8), with on/off and brightness sent with `CCT.Set`. The colour temperature is neither required nor mapped. A `light:n` with a channel above 7 is not mapped.
* **Presence** (Gen4 presence sensors): every `presencezone:N` with a boolean `value` (zone ids 200..207) becomes a read-only motion sensor in the Binary slots 8..15 (name **Presence zone 1..8**). A `presence` component with a boolean `state` becomes one motion sensor for the whole sensor. An `illuminance:0` without a `lux` number (only an `illumination` text) makes no device.
* **Cover and roller**: a blind with open, close, stop and a position (100 is fully open).
* **Meters**: a kWh device with power and the energy counter. A 3 phase meter gives a total plus one set per phase. Voltage, current, frequency and power factor become separate devices.
* **Returned energy**: a meter that reports returned (exported) energy gets an extra Sensor device **Returned energy** (slot 4 and up, one per channel, up to 12) holding the returned counter, and the returned power in watt (0 while importing).
* **Sensors**: temperature and humidity become a temperature+humidity device (or a temperature or humidity device when only one is reported). The temperature of a switch, cover or light is never mapped; the one device temperature is named **Device temperature** when the node switches or meters something and **Temperature** for a pure sensor. Lux, gas ppm and tilt become sensor devices; door/window, motion, flood, smoke, vibration, gas alarm and valve state become read-only binary devices.
* **Inputs and selectors**: an input with a state becomes a read-only contact (slots 0..7). An input configured as a push button (Gen1 momentary types, Gen2 `state` null) becomes a **selector** switch (slots 8..15) that shows the last press: single (10), double (20), triple (30) and long (40), with Off at 0. Gen2 sends presses as `NotifyEvent`; Gen1 reports an event and an event counter in its status, so a repeated press can be told from an old one. Inputs are read-only: writing to them sends nothing to the Shelly.
* **Thermostats**: a thermostat device holding the setpoint and the temperature. Setting the setpoint sends it to the Shelly in degrees Celsius.
* **Colour**: a colour switch. A brightness command only carries the brightness, so the colour comes from the last colour the light reported; a colour command is refused until the first status of the device has been received. A white channel of an RGBW controller in white mode is a dimmer, its colour channel a colour switch, so changing the mode moves between two device sets. When the mode of a Shelly changes, devices that are no longer used are left in place and logged so you can delete them.

Default device names come from the names configured on the Shelly. Names are only used when a device is created; renaming in Domoticz is never overwritten.

## Sleepy battery devices

Battery powered devices (Gen1 H&T, Door/Window, Flood, Motion, and Gen2+ devices with sleep mode) are reachable only for a short time after they wake up. Domoticz:

* reads the sleep mode and wake-up period from the device information and logs it;
* polls a sleepy device at most every 300 seconds, since it only answers while awake;
* holds back its first status until the device information is known, up to 10 minutes;
* treats being unreachable as normal: only a debug message, not an error.

Sensors time out on their own when no update arrives (the standard Domoticz sensor timeout). The battery level and Wi-Fi signal are set on the devices when the status reports them. A sleepy Gen1 device can additionally report through CoIoT (see below) the moment it wakes.

## Push updates

### Gen2+ WebSocket

For every Gen2+ device Domoticz opens `ws://<ip>/rpc` and receives `NotifyStatus` and `NotifyEvent` frames. Values appear within a moment and presses of push buttons are delivered as events. No configuration on the Shelly is needed. The poll interval is the fallback if the socket drops, and the socket is reconnected automatically.

Every Gen2+ device uses two threads while the hardware runs: a manager thread (connect, keepalive, baseline polls, reconnect backoff) and the network thread of the WebSocket. Stopping or removing a device waits for both, which normally takes well under a second but can take a few seconds when a request to an unreachable device is still waiting for its timeout; stopping the hardware stops its devices one after the other.

### Gen1 CoIoT (optional)

Gen1 devices are polled over REST. For lower latency you can enable unicast CoIoT push. The configuration is kept in existing columns of the hardware (shown on the hardware page):

| Setup field | Hardware column | Meaning |
|-------------|-----------------|---------|
| Poll interval | Mode1 | seconds between polls, default 30 |
| CoIoT port | Port | UDP port Domoticz listens on. **0 = off**, REST polling only (the usual port is 5683) |
| CoIoT address | Address | this host as the Shelly devices see it. Empty means the devices are not configured, only listened for |
| CoIoT update period | Mode2 | `coiot_update_period` in seconds written to the devices, clamped to 15..65535, default 15 |
| CoIoT port on the device | Mode3 | port written to the device when it differs from the listen port (a port mapping, for example Docker). 0 = same as Port |

When Address is set, Domoticz configures each Gen1 device through its settings so that it sends CoIoT to `<Address>:<Mode3 or Port>`. The address must be a host name or IPv4 address (an IPv6 literal is refused). All Gen1 nodes of the process share one listen port.

## Docker

* mDNS does not cross the default Docker bridge network, so there is no automatic discovery. Use the subnet scan with your **LAN** CIDR (for example `192.168.1.0/24`), not the container's own subnet. The scan is outbound unicast and works from a bridged container.
* Gen2+ push is outbound from the container and needs nothing extra.
* For Gen1 CoIoT push map UDP 5683 (or your chosen port) into the container, for example `-p 5683:5683/udp`, set Address to the **Docker host's** LAN address, and set Mode3 to the host side port when it differs from the container port. Without the mapping the devices keep working by REST polling.

## Limits

The mapping is tested against recorded device fixtures and a mock server. Not verified on real devices, because none was available:

* **CCT** lights are a dimmer only: the colour temperature is not read or set.
* A component type that the mapper does not know is logged once per device at status level ("component type 'x' is not supported yet and is skipped").
* **BLU** (Bluetooth) devices and gateways are not mapped.
* **Valve control**: the valve state is shown read-only, opening and closing it is not implemented.
* Shelly Cloud, firmware updates, camera/media components and real mDNS are out of scope.

A Gen2+ command path for relay, cover and light is covered by fixtures and a mock server rather than by real devices.

## Relationship to MQTT Auto Discovery

Gen2+ Shelly devices can also reach Domoticz through MQTT Auto Discovery (an MQTT broker and MQTT enabled on every device). Both paths create their own devices, so a Shelly present in both appears twice with different DeviceIDs and two sets of history. **Use one path per device.**

To help, Domoticz checks once per device and per start whether an MQTT Auto Discovery hardware already has devices whose DeviceID contains this Shelly's device id or MAC, and logs:

```
Shelly <name>: this device is also present through MQTT Auto Discovery; use one path per device
```

This is a best-effort log message only. Nothing is disabled or changed.

## Troubleshooting

| Symptom | Check |
|---------|-------|
| "did not answer as a Shelly device" when adding | Wrong IP or port, device offline, or a firewall. `http://<ip>/shelly` should return JSON in a browser. |
| Added, but no devices appear | "Accept new hardware devices" must be enabled. Check the log for 401 (wrong credentials). |
| 401 / unauthorized in the log | Gen1: user and password; Gen2+: the `admin` password. All devices of one hardware share the credentials. A Gen1 device logs "Shelly <name>: authentication failed, check the password" once, and again after a poll has succeeded in between. |
| Device shows as unreachable, then online again | Normal for Wi-Fi hiccups and for sleepy devices (those only log at debug). Increase the poll interval if it is noisy. |
| Subnet scan finds nothing | The range must be your LAN, at most 1024 addresses (/22), and reachable from Domoticz (Docker: not the container subnet). |
| Values are delayed on Gen1 | REST polling only; enable CoIoT (Port, Address) or lower the poll interval. |
| CoIoT enabled but no push | The Shelly must reach the Domoticz host on UDP, check Address and the port mapping (Mode3), and that the hardware is started. |
| Two nodes share temperature sensors | Equal low 16 bits of the MAC, logged at start; only the lowest MAC keeps them. |
| Devices exist twice | The Shelly is also added through MQTT Auto Discovery. Remove one path. |
| An old device is no longer updated | The Shelly changed mode (for example relay to roller, input to button). The old device stays and is logged; delete it. |
| Need more detail | Set the hardware log level and enable the hardware debug flag; node, command and mapping decisions are logged. |

### Reporting a device that is not supported correctly

If a Shelly model is missing devices, shows the wrong device type or wrong values, please create an issue on GitHub (https://github.com/domoticz/domoticz/issues) and add the information below. With the three documents of a device the mapping can be replayed offline, so no one needs to own the device to fix it.

**Gen2, Gen3 and Gen4 devices.** Open these three addresses in a browser (or fetch them with `curl`) and attach the results:

* `http://<ip>/shelly`
* `http://<ip>/rpc/Shelly.GetConfig`
* `http://<ip>/rpc/Shelly.GetStatus`

**Gen1 devices.** Attach:

* `http://<ip>/shelly`
* `http://<ip>/settings`
* `http://<ip>/status`

`/shelly` never needs a password. When the device is protected, use `curl --digest -u admin:<password> <url>` for Gen2+ and `curl -u <user>:<password> <url>` for Gen1.

Also tell us:

* the exact model and firmware version, for example "Shelly Pro 2PM, firmware 1.4.4";
* what Domoticz shows and what you expected (which device is wrong, which is missing);
* the mode or wiring that matters for the model (switch or cover mode, input type, what is connected);
* the Domoticz log lines of that Shelly from the start of the hardware and the first polls, with the hardware log level set to Info, Status and Error.

Before posting, remove personal data from the documents: the Wi-Fi SSID and password, the latitude and longitude (`location`), MQTT user names and passwords, cloud and authentication fields, and IPv6 addresses. Leave everything else as it is, edited values can hide the problem. The MAC address can stay, it is only used to build the device ids.
