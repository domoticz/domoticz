# Domoticz

Usage:
```
domoticz [-www <port>]  [-verbose <0|1>]
          -www <port>   Default is: -www 8080
          -verbose <0|1> (0 is none, 1 is debug)   Default is: -verbose 0
```

Examples:
```
domoticz            (this is the same as domoticz -www 8080 -verbose 0)
domoticz -www 81 -verbose 1
```

If Domoticz and the browser are running on the same system you can connect with http://localhost:8080/
To stop the application: press Ctrl-C in the application screen (not in the browser)

Compatible browsers:
* Chrome/Firefox/Safari...
* Internet Explorer version 10+

Be aware that a Raspberry pi receives its time from an online ntp server.
If the pi is not connected to a network, the device time will not be updated, resulting in scheduling issues. 

All ports below 1024 on linux systems can only be started by root.
If you run Domoticz on port 80, make sure to run it as root, e.g. : sudo ./domoticz

Compiling from source code:
---------------------------
### Ubuntu / Raspberry Pi (wheezy)
Follow this tutorial (Also for other Unix/Linux types)

https://www.domoticz.com/wiki/Installing_and_running_Domoticz_on_a_Raspberry_PI

### Windows
- You need Visual Studio 2019 (Community Edition is perfect and is free)
- The project file for Visual Studio can be found inside the "msbuild" folder
- For the Initial setup, Launch Visual Studio and from the 'Tools' menu choose 'Visual Studio Command Prompt'
- Go to the "msbuild" folder and issue the commands inside "packages.bat" one by one (or execute the batch file, but i prefer step by step!)
- You should now be able to compile Domoticz
- Make sure to check the projects configuration properties for Debugging/Working Directory, this should be set to "$(ProjectDir)/.."

### Synology

(Initial setup time is 30 minutes, DSM 5.1+/tested with 212+)]
Please see Wiki page at: http://www.domoticz.com/wiki/DomoticzSynology
