define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'self-sufficiency',
        label:       'Self-Sufficiency',
        description: 'Compact energy self-sufficiency percentage with house consumption, solar yield, and battery net',
        category:    'Energy',
        icon:        'fa-solid fa-leaf',
        defaultW:    4,
        defaultH:    1,
        minW:        3,
        minH:        1,
        maxW:        8,
        maxH:        4,
        configSchema: [
            {
                key:     'useEnergyDashboard',
                type:    'boolean',
                label:   'Auto-read device IDs from Energy Dashboard settings',
                default: true
            },
            {
                key:      'idP1',
                type:     'device-picker',
                label:    'Manual: P1 grid meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idSolar',
                type:     'device-picker',
                label:    'Manual: solar meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatteryEnergyIn',
                type:     'device-picker',
                label:    'Manual: battery charge meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatteryEnergyOut',
                type:     'device-picker',
                label:    'Manual: battery discharge meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:     'showGauge',
                type:    'boolean',
                label:   'Show gauge bar',
                default: true
            },
            {
                key:     'refreshInterval',
                type:    'number',
                label:   'Refresh interval (seconds)',
                default: 60
            }
        ]
    });

    app.directive('db2SelfSufficiencyWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/self-sufficiency.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;

                ctrl.balance    = null;
                ctrl.showGauge  = true;
                ctrl.configured = false;

                var cancelToken = null;
                var timer       = null;

                var ids = {
                    p1:         -1,
                    solar:      -1,
                    batteryIn:  -1,
                    batteryOut: -1
                };

                function parseKwh(str) {
                    if (!str) { return 0; }
                    return parseFloat(String(str).replace(' kWh', '').replace(',', '.')) || 0;
                }

                function applyDevices(result) {
                    if (!result || !result.length) { return; }

                    var byIdx = {};
                    result.forEach(function(item) { byIdx[String(item.idx)] = item; });

                    function get(id) { return id !== -1 ? (byIdx[String(id)] || null) : null; }

                    var grid       = get(ids.p1);
                    var solarDev   = get(ids.solar);
                    var battIn     = get(ids.batteryIn);
                    var battOut    = get(ids.batteryOut);

                    var p1Import     = grid      ? parseKwh(grid.CounterToday)      : 0;
                    var p1Export     = grid      ? parseKwh(grid.CounterDelivToday)  : 0;
                    var solar        = solarDev  ? parseKwh(solarDev.CounterToday)  : 0;
                    var batCharge    = battIn    ? parseKwh(battIn.CounterToday)    : 0;
                    var batDischarge = battOut   ? parseKwh(battOut.CounterToday)   : 0;

                    var batNet           = batCharge - batDischarge;
                    var netGrid          = p1Import - p1Export;
                    var houseConsumption = p1Import + solar - p1Export - batNet;
                    var selfSufficiency  = houseConsumption > 0
                        ? (1 - Math.max(0, netGrid) / houseConsumption) * 100
                        : 0;

                    ctrl.balance = {
                        selfSufficiency: Math.min(100, Math.max(0, selfSufficiency)),
                        houseToday:      houseConsumption,
                        solarToday:      solar,
                        batNet:          batNet
                    };
                }

                function fetchDevices() {
                    var fetchIds = [];
                    if (ids.p1         !== -1) { fetchIds.push(ids.p1);         }
                    if (ids.solar      !== -1) { fetchIds.push(ids.solar);      }
                    if (ids.batteryIn  !== -1) { fetchIds.push(ids.batteryIn);  }
                    if (ids.batteryOut !== -1) { fetchIds.push(ids.batteryOut); }

                    if (!fetchIds.length) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: fetchIds.join(',') },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        applyDevices(resp.data && resp.data.result);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                    });
                }

function loadWithAuto() {
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'getenergydashboarddevices' }
                    }).then(function(resp) {
                        var d = resp.data;
                        if (d && d.result && d.result.ESettings) {
                            var s = d.result.ESettings;
                            ids.p1         = s.idP1              || -1;
                            ids.solar      = s.idSolar           || -1;
                            ids.batteryIn  = s.idBatteryEnergyIn  || -1;
                            ids.batteryOut = s.idBatteryEnergyOut || -1;
                        }
                        fetchDevices();
                    }).catch(function() {
                        fetchDevices();
                    });
                }

                function loadWithManual(cfg) {
                    ids.p1         = cfg.idP1              || -1;
                    ids.solar      = cfg.idSolar           || -1;
                    ids.batteryIn  = cfg.idBatteryEnergyIn  || -1;
                    ids.batteryOut = cfg.idBatteryEnergyOut || -1;
                    fetchDevices();
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};

                    ctrl.showGauge = cfg.showGauge !== false;

                    var useAuto = cfg.useEnergyDashboard !== false;
                    ctrl.configured = useAuto || !!(
                        cfg.idP1 || cfg.idSolar || cfg.idBatteryEnergyIn || cfg.idBatteryEnergyOut
                    );

                    if (!ctrl.configured) { return; }

                    if (useAuto) {
                        loadWithAuto();
                    } else {
                        loadWithManual(cfg);
                    }
                }

                function startTimer() {
                    if (timer) { $interval.cancel(timer); }
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = (cfg.refreshInterval || 60) * 1000;
                    timer = $interval(load, interval);
                }

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (timer)       { $interval.cancel(timer); timer = null; }
                });

                $scope.$on('db2:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg && JSON.stringify({
                            useAuto:         cfg.useEnergyDashboard,
                            p1:              cfg.idP1,
                            solar:           cfg.idSolar,
                            battIn:          cfg.idBatteryEnergyIn,
                            battOut:         cfg.idBatteryEnergyOut,
                            showGauge:       cfg.showGauge,
                            refreshInterval: cfg.refreshInterval
                        });
                    },
                    function(val, old) {
                        if (val !== old) { load(); startTimer(); }
                    }
                );

                ctrl.$onInit = function() {
                    load();
                    startTimer();
                };
            }]
        };
    }]);
});
