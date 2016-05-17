setlocal

cd ..

if not exist www\i18n mkdir www\i18n

for %%f in (i18n\*.json) do tools\gzip -c %%f > www\%%f.json.gz
