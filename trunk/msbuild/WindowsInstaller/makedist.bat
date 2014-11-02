@REM makes the sourceforge dist zip file

set HIGHNAME=
set VERSIONNO=
for %%1 in (DomoticzSetup*.exe) do call :versionhighest %%1
del domoticz-win32-*.zip
set VERSIONNO=%HIGHNAME:~14,6%
7z.exe a -tzip -mx5 domoticz-win32-%VERSIONNO%-setup.zip %HIGHNAME% Readme.txt ..\domoticz\History.txt

goto :eof

:versionhighest

set HIGHNAME=%1

goto :eof