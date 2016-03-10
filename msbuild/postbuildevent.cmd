@echo off
setlocal EnableDelayedExpansion

echo ^>^>^> Post build event...

echo Compressing .js and .json files...
for /f "delims=*" %%f in ('dir ..\www /s /b /a:-d ^| findstr /vi "\.js\." ^| findstr /vi "\.json\." ^| findstr /vi "\.gz" ^| findstr "\.js"') do call :compress_static %%f

echo Compressing .css files...
for /f "delims=*" %%f in ('dir ..\www /s /b /a:-d ^| findstr /vi "\.css\." ^| findstr "\.css"') do call :compress_static %%f

echo ^>^>^> Post build event OK

endlocal
goto :eof


:compress_static
set file_path=%1
for /f "tokens=*" %%i in ('tools\touch.exe /v "%file_path%"') do set text_file_date=%%i
for /f "tokens=*" %%i in ('tools\touch.exe /v "%file_path%.gz"') do set gzip_file_date=%%i
if "%text_file_date%"=="%gzip_file_date%" (
REM	echo %file_path% not modified
	goto :eof
)
if exist "%file_path%.gz" (
	del /q /s "%file_path%.gz" >nul
)
WindowsInstaller\7z.exe a -tgzip -mx5 "%file_path%.gz" "%file_path%" >nul
if %errorlevel% neq 0 (
	echo 7zip has failed compressing the file "%file_path%"
	goto :eof
)
if not exist "%file_path%.gz" (
 	echo File not found : "%file_path%"
	goto :eof
)
tools\touch.exe /r "%file_path%" "%file_path%.gz" >nul
if %errorlevel% neq 0 (
	echo Touch has failed with file "%file_path%"
	goto :eof
)
echo %file_path% compressed
goto :eof
