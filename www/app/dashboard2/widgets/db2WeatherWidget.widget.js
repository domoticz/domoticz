define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'weather',
        label:       'Weather',
        description: 'Current weather conditions from weather devices',
        category:    'Charts & Data',
        icon:        'fa-solid fa-cloud',
        modulePath:  'app/dashboard2/widgets/db2WeatherWidget.widget.js',
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
            }
        ]
    });

    app.directive('db2WeatherWidget', ['$q', function($q) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/weather-widget.html',
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

                var cancelTokens = [];

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

                    loadDevice(cfg.tempIdx, function(d) {
                        ctrl.temperature = (d.Temp !== undefined) ? d.Temp : '\u2014';
                        ctrl.humidity    = d.Humidity || null;
                        ctrl.description = d.HumidityStatus || d.Forecast || '';
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

                var timer = $interval(load, 60000);

                $scope.$on('$destroy', function() {
                    cancelAll();
                    $interval.cancel(timer);
                });
                $scope.$on('db2:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.tempIdx + '|' + cfg.windIdx + '|' + cfg.baroIdx) : '';
                    },
                    function(val, old) {
                        if (val !== old) { load(); }
                    }
                );

                ctrl.$onInit = load;
            }]
        };
    }]);
});
