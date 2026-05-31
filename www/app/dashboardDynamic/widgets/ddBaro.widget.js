define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'baro',
        label:       'Barometer',
        description: 'Barometric pressure and weather forecast',
        category:    'Weather',
        icon:        'fa-solid fa-gauge',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        transparentBackground: true,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Barometer device',
                required:     true,
                deviceFilter: 'baro'
            },
            {
                key:      'title',
                type:     'text',
                label:    'Custom Title',
                required: false
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddBaroWidget', ['$q', function($q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/baro.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$rootScope', '$q',
                function($scope, $http, $rootScope, $q) {
                var ctrl = this;
                ctrl.title        = '';
                ctrl.pressure     = null;
                ctrl.forecastStr  = '';
                ctrl.weatherScene = 'fcw-cloudy';
                ctrl.isNight      = false;
                ctrl.loading      = false;
                ctrl.loadError    = false;
                ctrl.sunrise      = '';
                ctrl.sunset       = '';

                var cancelTokens = [];

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

                function cfg() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function applyDevice(d) {
                    ctrl.title        = cfg().title || d.Name;
                    ctrl.pressure     = d.Barometer;
                    ctrl.forecastStr  = d.ForecastStr || d.Forecast || '';
                    ctrl.weatherScene = ctrl.getWeatherScene(ctrl.forecastStr);
                }

                function cancelAll() {
                    cancelTokens.forEach(function(t) { t.resolve(); });
                    cancelTokens = [];
                }

                function load() {
                    cancelAll();
                    var deviceIdx = cfg().deviceIdx;
                    if (!deviceIdx) { return; }

                    ctrl.loading   = true;
                    ctrl.loadError = false;

                    var token = $q.defer();
                    cancelTokens.push(token);

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: deviceIdx },
                        timeout: token.promise
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var result = resp.data.result;
                        if (result && result[0]) {
                            applyDevice(result[0]);
                        }
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loading   = false;
                        ctrl.loadError = true;
                    });

                    ctrl.logLink = '#/Devices/' + deviceIdx + '/Log';
                }

                $scope.$on('device_update', function(e, updated) {
                    var deviceIdx = cfg().deviceIdx;
                    if (deviceIdx && String(updated.idx) === String(deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                $scope.$on('time_update', function(e, data) {
                    if (data && data.sunrise && data.sunset) {
                        ctrl.sunrise = data.sunrise;
                        ctrl.sunset  = data.sunset;
                        ctrl.isNight = computeIsNight(data.sunrise, data.sunset);
                    }
                });

                $scope.$on('$destroy', cancelAll);

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var c = ctrl.widgetDef && ctrl.widgetDef.config;
                        return c ? (c.deviceIdx + '|' + (c.title || '')) : '';
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
