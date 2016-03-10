@echo off
setlocal EnableDelayedExpansion

set mode=%1

set script_dir=%~dp0
set www_dir=%script_dir%\..\www
set touch_home=%script_dir%\tools
set gzip_home=%script_dir%\WindowsInstaller
set path=%path%;%touch_home%;%gzip_home%

if not exist "%www_dir%" (
	echo %www_dir% directory not found !
	goto :eof
)
if not exist "%touch_home%" (
	echo %touch_home% directory not found !
	goto :eof
)
if not exist "%gzip_home%" (
	echo %gzip_home% directory not found !
	goto :eof
)

if "%mode%" == "prepare" goto :prepare
if "%mode%" == "clean" goto :clean

if "%mode%" == "" (
	echo Missing script argument !
) else (
	echo Invalid script argument : %mode% !
)
echo Usage: msbuild\packagingtasks.cmd prepare^|clean
goto :eof



:prepare
echo Compressing .js and .json files...
for /f "delims=*" %%f in ('dir "%www_dir%" /s /b /a:-d ^| findstr /vi "\.js\." ^| findstr /vi "\.json\." ^| findstr /vi "\.gz" ^| findstr "\.js"') do call :compress_static "%%f"
echo Compressing .css files...
for /f "delims=*" %%f in ('dir "%www_dir%" /s /b /a:-d ^| findstr /vi "\.css\." ^| findstr "\.css"') do call :compress_static "%%f"
goto :eof

:compress_static
set in_file_path=%~1
set out_file_path=%in_file_path%.gz
for /f "tokens=*" %%i in ('touch.exe /v "%in_file_path%"') do set in_file_date=%%i
for /f "tokens=*" %%i in ('touch.exe /v "%out_file_path%"') do set out_file_date=%%i
if "%in_file_date%"=="%out_file_date%" (
REM	echo %in_file_path% not modified
	goto :eof
)
if exist "%out_file_path%" (
	del /q /s "%out_file_path%" >nul
)
7z.exe a -tgzip -mx5 "%out_file_path%" "%in_file_path%" >nul
if %errorlevel% neq 0 (
	echo 7zip has failed compressing the file "%in_file_path%"
	goto :eof
)
if not exist "%out_file_path%" (
 	echo File not found : "%out_file_path%"
	goto :eof
)
touch.exe /r "%in_file_path%" "%out_file_path%" >nul
if %errorlevel% neq 0 (
	echo Touch has failed on file "%out_file_path%"
	goto :eof
)
echo %out_file_path% created
goto :eof


:clean
echo Removing .js.gz and .json.gz files...
for /f "delims=*" %%f in ('dir "%www_dir%" /s /b /a:-d ^| findstr /vi "\.js\." ^| findstr /vi "\.json\." ^| findstr /vi "\.gz" ^| findstr "\.js"') do call :remove_static "%%f"
echo Removing .css.gz files...
for /f "delims=*" %%f in ('dir "%www_dir%" /s /b /a:-d ^| findstr /vi "\.css\." ^| findstr "\.css"') do call :remove_static "%%f"
goto :eof

:remove_static
set in_file_path=%~1
set out_file_path=%in_file_path%.gz
if exist "%out_file_path%" (
	del /q /s "%out_file_path%" >nul
	echo "%out_file_path%" removed
)
goto :eof


