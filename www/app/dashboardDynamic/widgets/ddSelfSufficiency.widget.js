define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'self-sufficiency',
        transparentBackground: true,
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

    app.directive('ddSelfSufficiencyWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/self-sufficiency.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;

                ctrl.balance   = null;
                ctrl.showGauge = true;

                var cancelToken         = null;
                var cancelSettingsToken = null;
                var timer               = null;

                var ids = {
                    p1:         -1,
                    solar:      -1,
                    batteryIn:  -1,
                    batteryOut: -1
                };

                // All energy device IDs (for live-update trigger — superset of ids above)
                var allEnergyIds = [];

                function parseKwh(str) {
                    if (!str) { return 0; }
                    return parseFloat(String(str).replace(' kWh', '').replace(',', '.')) || 0;
                }

                function applyDevices(result) {
                    if (!result || !result.length) { return; }

                    var byIdx = {};
                    result.forEach(function(item) { byIdx[String(item.idx)] = item; });

                    function get(id) { return id !== -1 ? (byIdx[String(id)] || null) : null; }

                    var grid    = get(ids.p1);
                    var solar   = get(ids.solar);
                    var battIn  = get(ids.batteryIn);
                    var battOut = get(ids.batteryOut);

                    var p1Import     = grid    ? parseKwh(grid.CounterToday)      : 0;
                    var p1Export     = grid    ? parseKwh(grid.CounterDelivToday)  : 0;
                    var solarKwh     = solar   ? parseKwh(solar.CounterToday)      : 0;
                    var batCharge    = battIn  ? parseKwh(battIn.CounterToday)     : 0;
                    var batDischarge = battOut ? parseKwh(battOut.CounterToday)    : 0;

                    var batNet           = batCharge - batDischarge;
                    var houseConsumption = p1Import + solarKwh - p1Export - batNet;
                    var selfSufficiency  = houseConsumption > 0
                        ? Math.min(100, Math.max(0, (solarKwh + batDischarge) / houseConsumption * 100))
                        : 0;

                    ctrl.balance = {
                        selfSufficiency: selfSufficiency,
                        houseToday:      houseConsumption,
                        solarToday:      solarKwh,
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

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.showGauge = cfg.showGauge !== false;

                    if (cancelSettingsToken) { cancelSettingsToken.resolve(); }
                    cancelSettingsToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getenergydashboarddevices' },
                        timeout: cancelSettingsToken.promise
                    }).then(function(resp) {
                        var d = resp.data;
                        if (d && d.result && d.result.ESettings) {
                            var s = d.result.ESettings;
                            ids.p1         = s.idP1               || -1;
                            ids.solar      = s.idSolar            || -1;
                            ids.batteryIn  = s.idBatteryEnergyIn  || -1;
                            ids.batteryOut = s.idBatteryEnergyOut || -1;
                            // Build full set for live-update detection
                            allEnergyIds = [
                                s.idP1, s.idSolar, s.idOutsideTempSensor,
                                s.idGas, s.idBatteryEnergyIn, s.idBatteryEnergyOut,
                                s.idBatterySoc, s.idBatteryWatt, s.idBatteryVolt
                            ].filter(function(id) { return id && id !== -1; })
                             .map(String);
                        }
                        fetchDevices();
                    }).catch(function() {
                        fetchDevices();
                    });
                }

                function startTimer() {
                    if (timer) { $interval.cancel(timer); }
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = (cfg.refreshInterval || 60) * 1000;
                    timer = $interval(load, interval);
                }

                $scope.$on('device_update', function(event, device) {
                    if (!device || !device.idx) { return; }
                    if (allEnergyIds.indexOf(String(device.idx)) !== -1) {
                        fetchDevices();
                    }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken)         { cancelToken.resolve();         cancelToken         = null; }
                    if (cancelSettingsToken) { cancelSettingsToken.resolve(); cancelSettingsToken = null; }
                    if (timer)               { $interval.cancel(timer);       timer               = null; }
                });

                ctrl.$onInit = function() {
                    load();
                    startTimer();
                };
            }]
        };
    }]);
});
