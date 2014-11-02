@echo off
tx.exe pull -l en
tx.exe pull -a --skip
FOR %%i IN (..\www\i18n\*.json) DO 7z.exe -tgzip a "..\www\i18n\%%~ni.json.gz" "%%i"
del ..\www\i18n\*.json