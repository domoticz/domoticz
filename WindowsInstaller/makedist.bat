@REM makes the sourceforge dist zip file
del domoticz-win32-setup.zip
7z.exe a -tzip -mx5 domoticz-win32-setup.zip DomoticzSetup.exe Readme.txt ..\domoticz\History.txt
