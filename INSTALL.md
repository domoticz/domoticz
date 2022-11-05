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
* Edge/Chrome/Firefox/Safari...

Be aware that a Raspberry pi receives its time from an online ntp server.
If the pi is not connected to a network, the device time will not be updated, resulting in scheduling issues. 

All ports below 1024 on linux systems can only be started by root.
If you run Domoticz on port 80, make sure to run it as root, e.g. : sudo ./domoticz

