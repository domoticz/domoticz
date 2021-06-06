# Updating Highcharts

Highcharts needs to be patched every time it is updated. Follow these steps:

1. Download Highcharts.zip and extract highcharts.src.js to here
2. Copy highcharts.src.js to highcharts-<version>.src.js and highcharts-<version>-patch.9612.src.js
3. Compare highcharts-<previous>.src.js and highcharts-<previous>-patch.9612.src.js
4. Intelligently, apply each difference to highcharts-<version>-patch.9612.src.js
5. Update index.html and html5.appcache: 'js/highcharts-<previous>…' to 'js/highcharts-<version>…'
6. Extract all other highcharts files used here.
   (Don't forget the .js.map-files)
