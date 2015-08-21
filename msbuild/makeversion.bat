@echo OFF
for /f %%i in ('git rev-list HEAD --count') do set VAR=%%i
set /A VAR=VAR+2107
echo #define APPVERSION %VAR%> "..\appversion_temp.h"
for /f %%i in ('git rev-parse --short HEAD') do set VAR=%%i
echo #define APPHASH "%VAR%">> "..\appversion_temp.h"
git show -s --format=%%ct > %TEMP%\domtmpfile
set /p VAR=<%TEMP%\domtmpfile
del %TEMP%\domtmpfile
echo #define APPDATE %VAR%>> "..\appversion_temp.h"

IF EXIST ..\appversion.h ( 
    FC /B ..\appversion.h ..\appversion_temp.h > NUL
    IF ERRORLEVEL 1 GOTO :PROCESS
    del ..\appversion_temp.h
    EXIT /B
)

:PROCESS
cd ..
copy /Y appversion_temp.h appversion.h
del appversion_temp.h

