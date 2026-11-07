define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'energyWeather.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'energy-weather',
        transparentBackground: true,
        label:       'Energy vs Weather',
        description: 'Usage per degree day over the last 7 days, compared with the 7 days before. Uses the Energy Dashboard outside sensor, gas and airco devices.',
        category:    'Energy',
        icon:        'fa-solid fa-temperature-half',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        3,
        configSchema: [
            {
                key:      'mode',
                type:     'select',
                label:    'Show',
                options:  [{ value: 'heat', label: 'Gas per heating degree day' }, { value: 'hp', label: 'Heat pump / airco kWh per heating degree day' }, { value: 'cool', label: 'Heat pump / airco kWh per cooling degree day' }],
                default:  'heat'
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

    app.directive('ddEnergyWeatherWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/energy-weather.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$q', 'dzEnergyWeather', function($scope, $q, dzEnergyWeather) {
                var ctrl = this;
                ctrl.title     = '';
                ctrl.unit      = '';
                ctrl.perDD     = null;
                ctrl.change    = null;
                ctrl.dd        = null;
                ctrl.usage     = null;
                ctrl.noData    = false;
                ctrl.loadError = false;
                ctrl.needSetup = false;

                var loadSeq  = 0;
                var lastDate = null;

                function pad(n) { return (n < 10 ? '0' : '') + n; }

                function fmt(dt) {
                    return dt.getFullYear() + '-' + pad(dt.getMonth() + 1) + '-' + pad(dt.getDate());
                }

                // 'YYYY-MM-DD' strings of the 14 complete days before today, oldest first
                function lastDays() {
                    var out = [];
                    var now = new Date();
                    for (var i = 14; i >= 1; i--) {
                        out.push(fmt(new Date(now.getFullYear(), now.getMonth(), now.getDate() - i)));
                    }
                    return out;
                }

                function sumWindow(days, keys, mode) {
                    var usage = 0, dd = 0, n = 0;
                    keys.forEach(function(k) {
                        var day = days[k];
                        if (!day) { return; }
                        var u = (mode === 'heat') ? day.gas : day.aircoTotal;
                        if (u === null || typeof u == 'undefined') { return; }
                        usage += u;
                        dd += (mode === 'cool') ? day.cdd : day.hdd;
                        n++;
                    });
                    return { usage: usage, dd: dd, n: n };
                }

                function load() {
                    var cfg  = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var mode = cfg.mode || 'heat';
                    var seq  = ++loadSeq;
                    lastDate = fmt(new Date());
                    ctrl.loadError = false;

                    dzEnergyWeather.loadSettings().then(function(s) {
                        if (s.idOutside < 0 || (mode === 'heat' ? s.idGas < 0 : !s.idAirco.length)) {
                            ctrl.needSetup = true;
                            return null;
                        }
                        ctrl.needSetup = false;
                        var keys = lastDays();
                        var years = {};
                        keys.forEach(function(k) { years[k.substring(0, 4)] = true; });
                        return $q.all(Object.keys(years).map(function(y) {
                            return dzEnergyWeather.loadYear(s, parseInt(y, 10));
                        })).then(function(results) {
                            var days = {};
                            results.forEach(function(r) { Object.assign(days, r.days); });
                            return { keys: keys, days: days };
                        });
                    }).then(function(res) {
                        if (seq !== loadSeq || !res) { return; }
                        var prev = sumWindow(res.days, res.keys.slice(0, 7), mode);
                        var cur  = sumWindow(res.days, res.keys.slice(7), mode);
                        ctrl.title = cfg.title || (mode === 'heat' ? $.t('Gas') : $.t('Heat Pump / Airco'));
                        ctrl.unit  = (mode === 'heat') ? 'm3' : 'kWh';
                        ctrl.noData = !(cur.n > 0 && cur.dd >= 1);
                        ctrl.dd    = cur.dd;
                        ctrl.usage = cur.usage;
                        ctrl.perDD = ctrl.noData ? null : cur.usage / cur.dd;
                        ctrl.change = (!ctrl.noData && prev.n > 0 && prev.dd >= 1 && prev.usage > 0)
                            ? ((ctrl.perDD / (prev.usage / prev.dd)) - 1) * 100
                            : null;
                    }).catch(function() {
                        if (seq === loadSeq) { ctrl.loadError = true; }
                    });
                }

                $scope.$on('time_update', function() {
                    if (lastDate && lastDate !== fmt(new Date())) { load(); }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.mode || 'heat') + '|' + (cfg.title || '') : '';
                    },
                    function(val, old) { if (val !== old) { load(); } }
                );

                $scope.$on('$destroy', function() { loadSeq++; });

                ctrl.$onInit = load;
            }]
        };
    }]);
});
