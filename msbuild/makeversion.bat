@echo OFF
for /f %%i in ('git rev-list HEAD --count') do set VAR=%%i
echo #define APPVERSION %VAR%> "..\appversion.h"
for /f %%i in ('git rev-parse --short HEAD') do set VAR=%%i
echo #define APPHASH "%VAR%">> "..\appversion.h"
git show -s --format=%%ct > %TEMP%\domtmpfile
set /p VAR=<%TEMP%\domtmpfile
del %TEMP%\domtmpfile
echo #define APPDATE %VAR%>> "..\appversion.h"
