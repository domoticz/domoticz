# Domoticz — Free Open Source Home Automation System

Domoticz is a free, open source home automation system for Linux, Windows, macOS, and Raspberry Pi. Monitor and control lights, switches, and sensors for temperature, rain, wind, UV, energy, gas, water and much more. Supports 150+ hardware devices including Z-Wave, Zigbee, MQTT, Philips Hue, and RFXCOM. Notifications and alerts can be sent to any mobile device.

## Multi platform: Linux/Windows/macOS/Raspberry Pi

Runs on Linux, Windows, macOS, FreeBSD, and embedded devices like Raspberry Pi and other ARM boards.
The user-interface is a scalable HTML5 web frontend, automatically adapted for desktop and mobile devices.
Compatible with all modern browsers.

### Features
- 150+ supported hardware types: Z-Wave, Zigbee, MQTT, RFXCOM, P1 Smart Meter, YouLess Meter, Pulse Counters, 1-Wire, Philips Hue and more
- Event scripting with dzVents (Lua) and Python plugins
- Extended logging
- Push notifications for iPhone, Android and desktop
- Auto learning sensors/switches
- Manual creation of switch codes
- Share / Use external devices
- Designed for simplicity

## Support

On first run, Domoticz will present a setup wizard to create your admin account. For Docker deployments, you can set the `DOMOTICZ_ADMIN_PASSWORD` and optionally `DOMOTICZ_ADMIN_USERNAME` environment variables to provision the admin account automatically.

More information on securing your Domoticz setup can be found in the [SECURITY SETUP](SECURITY_SETUP.md) documentation.

Your first place for support is the [Domoticz Forum](http://www.domoticz.com/forum)

The Github issue tracker is NOT for end-user support.

## Security issues

See the [Security](SECURITY.md) file for more information.

## Donations
Donations are more than welcome and will be used to buy new hardware, devices, sensors, hosting and coffee.
If you like the product or encourage the development, please use the link:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=6S2CXM772QY84&currency_code=EUR&source=url)

# More information
* Website: https://www.domoticz.com
* Forum: https://forum.domoticz.com/
* Wiki: https://wiki.domoticz.com/
* Build from source: https://wiki.domoticz.com/Build_Domoticz_from_source

### Build Status

Status | Operating system
------------ | -------------
[![Status Linux](https://github.com/domoticz/domoticz/actions/workflows/development.yml/badge.svg)](https://github.com/domoticz/domoticz/actions/workflows/development.yml) | Linux x86_64
[![Status Windows](https://github.com/domoticz/domoticz/actions/workflows/windows-development-build.yml/badge.svg)](https://github.com/domoticz/domoticz/actions/workflows/windows-development-build.yml) | Windows x86
[![Status Windows (Appveyor)](https://ci.appveyor.com/api/projects/status/fskiwvjs1q7svwq9?svg=true)](https://ci.appveyor.com/project/gizmocuz/domoticz) | Windows (Appveyor)

