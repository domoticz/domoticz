define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'weather',
        label:       'Weather',
        description: 'Current weather conditions from weather devices',
        category:    'Charts & Data',
        icon:        'fa-solid fa-cloud',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        configSchema: [
            {
                key:          'tempIdx',
                type:         'device-picker',
                label:        'Temperature device',
                required:     false,
                deviceFilter: 'temp'
            },
            {
                key:          'windIdx',
                type:         'device-picker',
                label:        'Wind device',
                required:     false,
                deviceFilter: 'wind'
            },
            {
                key:          'baroIdx',
                type:         'device-picker',
                label:        'Barometer device',
                required:     false,
                deviceFilter: 'baro'
            },
            {
                key:     'displayStyle',
                type:    'select',
                label:   'Display style',
                options: [
                    { value: 'compact',  label: 'Style 1' },
                    { value: 'forecast', label: 'Style 2' }
                ],
                default: 'compact'
            }
        ]
    });

    app.directive('ddWeatherWidget', ['$q', function($q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/weather-widget.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$rootScope', '$q',
                function($scope, $http, $interval, $rootScope, $q) {
                var ctrl = this;
                ctrl.temperature   = '\u2014';
                ctrl.humidity      = null;
                ctrl.windSpeed     = null;
                ctrl.windDirection = null;
                ctrl.barometer     = null;
                ctrl.description   = '';
                ctrl.forecastStr   = '';
                ctrl.weatherScene  = 'fcw-cloudy';
                ctrl.tempSign      = ($rootScope.config && $rootScope.config.TempSign) || 'C';
                ctrl.windUnit      = ($rootScope.config && $rootScope.config.WindSign)  || 'm/s';
                ctrl.isNight       = false;
                ctrl.sunrise       = '';
                ctrl.sunset        = '';
                ctrl.dewPoint      = null;
                ctrl.displayStyle  = 'compact';

                var cancelTokens = [];
                var nightTimer   = null;

                function computeIsNight(sunriseStr, sunsetStr) {
                    var toMinutes = function(s) {
                        var p = s.split(':');
                        return parseInt(p[0], 10) * 60 + parseInt(p[1], 10);
                    };
                    var now    = new Date();
                    var nowMin = now.getHours() * 60 + now.getMinutes();
                    return nowMin < toMinutes(sunriseStr) || nowMin >= toMinutes(sunsetStr);
                }

                function loadSunRiseSet() {
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'getSunRiseSet' }
                    }).then(function(resp) {
                        var d = resp.data;
                        if (d && d.Sunrise && d.Sunset) {
                            ctrl.sunrise = d.Sunrise;
                            ctrl.sunset  = d.Sunset;
                            ctrl.isNight = computeIsNight(d.Sunrise, d.Sunset);
                        }
                    });
                }

                ctrl.getWeatherScene = function(forecastStr) {
                    if (!forecastStr) { return 'fcw-cloudy'; }
                    var s = forecastStr.toLowerCase();
                    if (s.indexOf('heavy rain') >= 0 || s.indexOf('thunderstorm') >= 0) { return 'fcw-heavyrain'; }
                    if (s.indexOf('rain') >= 0 || s.indexOf('shower') >= 0) { return 'fcw-rain'; }
                    if (s.indexOf('heavy snow') >= 0 || s.indexOf('blizzard') >= 0) { return 'fcw-heavysnow'; }
                    if (s.indexOf('snow') >= 0 || s.indexOf('sleet') >= 0) { return 'fcw-snow'; }
                    if (s.indexOf('sunny') >= 0 || (s.indexOf('clear') >= 0 && s.indexOf('night') < 0)) { return 'fcw-sunny'; }
                    if (s.indexOf('partly') >= 0 || s.indexOf('scattered') >= 0) { return 'fcw-partlycloudy'; }
                    if (s.indexOf('night') >= 0) { return 'fcw-night'; }
                    if (s.indexOf('cloud') >= 0 || s.indexOf('overcast') >= 0) { return 'fcw-cloudy'; }
                    return 'fcw-cloudy';
                };

                function cancelAll() {
                    cancelTokens.forEach(function(t) { t.resolve(); });
                    cancelTokens = [];
                }

                function loadDevice(idx, callback) {
                    if (!idx) { return; }
                    var token = $q.defer();
                    cancelTokens.push(token);
                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: idx },
                        timeout: token.promise
                    }).then(function(resp) {
                        var d = resp.data.result && resp.data.result[0];
                        if (d) { callback(d); }
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.error = 'Failed to load data';
                        ctrl.loading = false;
                    });
                }

                function load() {
                    cancelAll();
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};

                    ctrl.displayStyle = cfg.displayStyle || 'compact';

                    loadDevice(cfg.tempIdx, function(d) {
                        ctrl.temperature = (d.Temp !== undefined) ? d.Temp : '\u2014';
                        ctrl.humidity    = d.Humidity || null;
                        ctrl.description = d.HumidityStatus || d.Forecast || '';

                        // Extract dew point: prefer device field, fall back to Magnus formula
                        if (d.DewPoint !== undefined && d.DewPoint !== null) {
                            ctrl.dewPoint = parseFloat(d.DewPoint);
                        } else if (d.Temp !== undefined && d.Humidity) {
                            var temp     = parseFloat(d.Temp);
                            var humidity = parseFloat(d.Humidity);
                            if (!isNaN(temp) && !isNaN(humidity) && humidity > 0) {
                                var a     = 17.27, b = 237.7;
                                var alpha = (a * temp / (b + temp)) + Math.log(humidity / 100);
                                ctrl.dewPoint = (b * alpha) / (a - alpha);
                            }
                        }
                    });

                    loadDevice(cfg.windIdx, function(d) {
                        ctrl.windSpeed     = d.Speed;
                        ctrl.windDirection = d.DirectionStr;
                    });

                    loadDevice(cfg.baroIdx, function(d) {
                        ctrl.barometer   = d.Barometer;
                        ctrl.forecastStr = d.ForecastStr || d.Forecast || '';
                        ctrl.weatherScene = ctrl.getWeatherScene(ctrl.forecastStr);
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var id = String(updated.idx);
                    if (cfg.tempIdx && id === String(cfg.tempIdx)) {
                        ctrl.temperature = (updated.Temp !== undefined) ? updated.Temp : '\u2014';
                        ctrl.humidity    = updated.Humidity || null;
                        ctrl.description = updated.HumidityStatus || updated.Forecast || '';

                        if (updated.DewPoint !== undefined && updated.DewPoint !== null) {
                            ctrl.dewPoint = parseFloat(updated.DewPoint);
                        } else if (updated.Temp !== undefined && updated.Humidity) {
                            var temp     = parseFloat(updated.Temp);
                            var humidity = parseFloat(updated.Humidity);
                            if (!isNaN(temp) && !isNaN(humidity) && humidity > 0) {
                                var a     = 17.27, b = 237.7;
                                var alpha = (a * temp / (b + temp)) + Math.log(humidity / 100);
                                ctrl.dewPoint = (b * alpha) / (a - alpha);
                            }
                        }
                    }
                    if (cfg.windIdx && id === String(cfg.windIdx)) {
                        ctrl.windSpeed     = updated.Speed;
                        ctrl.windDirection = updated.DirectionStr;
                    }
                    if (cfg.baroIdx && id === String(cfg.baroIdx)) {
                        ctrl.barometer   = updated.Barometer;
                        ctrl.forecastStr = updated.ForecastStr || updated.Forecast || '';
                        ctrl.weatherScene = ctrl.getWeatherScene(ctrl.forecastStr);
                    }
                });

                var timer = $interval(load, 60000);

                // Re-evaluate isNight every minute (sunrise/sunset data loaded once on init)
                nightTimer = $interval(function() {
                    if (ctrl.sunrise && ctrl.sunset) {
                        ctrl.isNight = computeIsNight(ctrl.sunrise, ctrl.sunset);
                    }
                }, 60000);

                $scope.$on('$destroy', function() {
                    cancelAll();
                    $interval.cancel(timer);
                    if (nightTimer) { $interval.cancel(nightTimer); }
                });
                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.tempIdx + '|' + cfg.windIdx + '|' + cfg.baroIdx + '|' + (cfg.displayStyle || 'compact')) : '';
                    },
                    function(val, old) {
                        if (val !== old) { load(); }
                    }
                );

                ctrl.$onInit = function() {
                    load();
                    loadSunRiseSet();
                };
            }]
        };
    }]);
});
