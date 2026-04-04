define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var WMO = {
        0:  { icon: 'fa-sun',                 label: 'Clear Sky',               token: '--dz-widget-sunpv'   },
        1:  { icon: 'fa-cloud-sun',           label: 'Mainly Clear',            token: '--dz-widget-weather-cloud' },
        2:  { icon: 'fa-cloud-sun',           label: 'Partly Cloudy',           token: '--dz-widget-weather-cloud' },
        3:  { icon: 'fa-cloud',               label: 'Overcast',                token: '--dz-widget-weather-cloud' },
        45: { icon: 'fa-smog',                label: 'Fog',                     token: '--dz-widget-weather-cloud' },
        48: { icon: 'fa-smog',                label: 'Depositing Rime Fog',     token: '--dz-widget-weather-cloud' },
        51: { icon: 'fa-cloud-rain',          label: 'Light Drizzle',           token: '--dz-widget-accent'  },
        53: { icon: 'fa-cloud-rain',          label: 'Moderate Drizzle',        token: '--dz-widget-accent'  },
        55: { icon: 'fa-cloud-rain',          label: 'Dense Drizzle',           token: '--dz-widget-accent'  },
        56: { icon: 'fa-cloud-rain',          label: 'Light Frz. Drizzle',      token: '--dz-widget-weather-snow'  },
        57: { icon: 'fa-cloud-rain',          label: 'Dense Frz. Drizzle',      token: '--dz-widget-weather-snow'  },
        61: { icon: 'fa-cloud-rain',          label: 'Slight Rain',             token: '--dz-widget-accent'  },
        63: { icon: 'fa-cloud-rain',          label: 'Moderate Rain',           token: '--dz-widget-accent'  },
        65: { icon: 'fa-cloud-rain',          label: 'Heavy Rain',              token: '--dz-widget-accent'  },
        66: { icon: 'fa-cloud-rain',          label: 'Light Frz. Rain',         token: '--dz-widget-weather-snow'  },
        67: { icon: 'fa-cloud-rain',          label: 'Heavy Frz. Rain',         token: '--dz-widget-weather-snow'  },
        71: { icon: 'fa-snowflake',           label: 'Slight Snowfall',         token: '--dz-widget-weather-snow'  },
        73: { icon: 'fa-snowflake',           label: 'Moderate Snowfall',       token: '--dz-widget-weather-snow'  },
        75: { icon: 'fa-snowflake',           label: 'Heavy Snowfall',          token: '--dz-widget-weather-snow'  },
        77: { icon: 'fa-snowflake',           label: 'Snow Grains',             token: '--dz-widget-weather-snow'  },
        80: { icon: 'fa-cloud-showers-heavy', label: 'Slight Rain Showers',     token: '--dz-widget-accent'  },
        81: { icon: 'fa-cloud-showers-heavy', label: 'Moderate Rain Showers',   token: '--dz-widget-accent'  },
        82: { icon: 'fa-cloud-showers-heavy', label: 'Violent Rain Showers',    token: '--dz-widget-accent'  },
        85: { icon: 'fa-snowflake',           label: 'Slight Snow Showers',     token: '--dz-widget-weather-snow'  },
        86: { icon: 'fa-snowflake',           label: 'Heavy Snow Showers',      token: '--dz-widget-weather-snow'  },
        95: { icon: 'fa-bolt',                label: 'Thunderstorm',            token: '--dz-widget-weather-storm' },
        96: { icon: 'fa-bolt',                label: 'Thunderstorm w/ Hail',    token: '--dz-widget-weather-storm' },
        99: { icon: 'fa-bolt',                label: 'Thunderstorm+Heavy Hail', token: '--dz-widget-weather-storm' }
    };

    var CACHE_TTL_MS = 30 * 60 * 1000; // 30 minutes

    widgetRegistry.register({
        type:        'weather-forecast',
        label:       'Weather Forecast',
        description: 'Multi-day weather forecast using the Open-Meteo API and your configured location',
        category:    'Weather',
        icon:        'fa-solid fa-cloud-sun',
        defaultW:    6,
        defaultH:    2,
        minW:        3,
        minH:        2,
        maxW:        12,
        maxH:        6,
        configSchema: [
            {
                key:      'days',
                type:     'number',
                label:    'Days to show (1–14)',
                default:  7,
                min:      1,
                max:      14,
                required: false
            },
            {
                key:      'showWind',
                type:     'boolean',
                label:    'Show wind speed',
                default:  true,
                required: false
            },
            {
                key:      'showPrecip',
                type:     'boolean',
                label:    'Show precipitation',
                default:  true,
                required: false
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddWeatherForecastWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/weather-forecast.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl       = this;
                ctrl.days      = [];
                ctrl.title     = '';
                ctrl.showPrecip = true;
                ctrl.showWind   = true;
                ctrl.error     = null;

                // Simple in-memory cache shared across directive instances via closure
                ctrl._cache = { data: null, time: 0 };

                var cancelLocation = null;
                var cancelForecast = null;

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function wmoInfo(code) {
                    return WMO[code] || { icon: 'fa-cloud', label: 'Unknown', token: '--dz-widget-weather-cloud' };
                }

                function buildDays(daily) {
                    var dates    = daily.time                  || [];
                    var maxTemps = daily.temperature_2m_max    || [];
                    var minTemps = daily.temperature_2m_min    || [];
                    var precips  = daily.precipitation_sum     || [];
                    var codes    = daily.weathercode           || [];
                    var winds    = daily.windspeed_10m_max     || [];

                    return dates.map(function(dateStr, i) {
                        var info    = wmoInfo(codes[i]);
                        var dayName = (i === 0)
                            ? 'Today'
                            : new Date(dateStr).toLocaleDateString('en-US', { weekday: 'short' });

                        return {
                            dateStr:    dateStr,
                            dayName:    dayName,
                            icon:       'fa-solid ' + info.icon,
                            label:      info.label,
                            tokenColor: info.token,
                            tempMax:    maxTemps[i] !== undefined ? Math.round(maxTemps[i]) : '—',
                            tempMin:    minTemps[i] !== undefined ? Math.round(minTemps[i]) : '—',
                            precip:     precips[i]  !== undefined ? Math.round(precips[i] * 10) / 10 : '—',
                            wind:       winds[i]    !== undefined ? Math.round(winds[i]) : '—'
                        };
                    });
                }

                function applyForecast(daily) {
                    var c        = cfg();
                    ctrl.title     = c.title || 'Weather Forecast';
                    ctrl.showPrecip = c.showPrecip !== false;
                    ctrl.showWind   = c.showWind   !== false;
                    ctrl.days      = buildDays(daily);
                    ctrl.error     = null;
                }

                function fetchForecast(lat, lon) {
                    var c         = cfg();
                    var days      = Math.min(14, Math.max(1, parseInt(c.days, 10) || 7));
                    var now       = Date.now();
                    var cache     = ctrl._cache;

                    if (cache.data && (now - cache.time) < CACHE_TTL_MS) {
                        applyForecast(cache.data);
                        return;
                    }

                    if (cancelForecast) { cancelForecast.resolve(); }
                    cancelForecast = $q.defer();

                    var url = 'https://api.open-meteo.com/v1/forecast'
                        + '?latitude='  + lat
                        + '&longitude=' + lon
                        + '&timezone=auto'
                        + '&daily=temperature_2m_max,temperature_2m_min,precipitation_sum,weathercode,windspeed_10m_max'
                        + '&forecast_days=' + days;

                    $http.get(url, { timeout: cancelForecast.promise })
                        .then(function(resp) {
                            var daily = resp.data && resp.data.daily;
                            if (!daily) {
                                ctrl.error = 'Unexpected response from Open-Meteo';
                                return;
                            }
                            ctrl._cache = { data: daily, time: Date.now() };
                            applyForecast(daily);
                        })
                        .catch(function(err) {
                            if (err.status === -1) { return; }
                            ctrl.error = 'Failed to load forecast data';
                        });
                }

                function load() {
                    ctrl.error = null;

                    if (cancelLocation) { cancelLocation.resolve(); }
                    cancelLocation = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getforecastconfig' },
                        timeout: cancelLocation.promise
                    }).then(function(resp) {
                        var lat = resp.data && resp.data.Latitude;
                        var lon = resp.data && resp.data.Longitude;
                        var latF = parseFloat(lat);
                        var lonF = parseFloat(lon);
                        if (!lat || !lon || isNaN(latF) || isNaN(lonF) || (latF === 1 && lonF === 1)) {
                            ctrl.error = 'Location not configured in Domoticz';
                            return;
                        }
                        fetchForecast(latF, lonF);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.error = 'Failed to retrieve location from Domoticz';
                    });
                }

                var timer = $interval(load, CACHE_TTL_MS);

                $scope.$on('$destroy', function() {
                    if (cancelLocation) { cancelLocation.resolve(); cancelLocation = null; }
                    if (cancelForecast) { cancelForecast.resolve(); cancelForecast = null; }
                    $interval.cancel(timer);
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var c = cfg();
                        return c.days + '|' + c.showWind + '|' + c.showPrecip + '|' + c.title;
                    },
                    function(val, old) {
                        if (val !== old) {
                            // Bust cache so a days-count change triggers a fresh fetch
                            ctrl._cache = { data: null, time: 0 };
                            load();
                        }
                    }
                );

                ctrl.$onInit = load;
            }]
        };
    }]);
});
