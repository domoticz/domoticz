define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'battery-status',
        transparentBackground: true,
        label:       'Battery Status',
        description: 'Battery import/export kWh, net, SOC %, watts, and voltage',
        category:    'Energy',
        icon:        'fa-solid fa-battery-half',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        configSchema: [
            {
                key:     'useEnergyDashboard',
                type:    'boolean',
                label:   'Auto-read device IDs from Energy Dashboard settings',
                default: true
            },
            {
                key:      'idBatteryEnergyIn',
                type:     'device-picker',
                label:    'Manual: import kWh meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatteryEnergyOut',
                type:     'device-picker',
                label:    'Manual: export kWh meter',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatterySoc',
                type:     'device-picker',
                label:    'Manual: SOC % sensor',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatteryWatt',
                type:     'device-picker',
                label:    'Manual: current power (W)',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'idBatteryVolt',
                type:     'device-picker',
                label:    'Manual: voltage (V)',
                showWhen: { key: 'useEnergyDashboard', value: false }
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title',
                default:  'Battery'
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddBatteryStatusWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/battery-status.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', function($scope, $http, $q) {
                var ctrl = this;

                ctrl.title       = 'Battery';
                ctrl.battery     = null;   // { importedKwh, exportedKwh }
                ctrl.batteryLive = null;   // { soc, watt, voltage }
                ctrl.configured  = false;

                var cancelToken  = null;

                // Device IDs resolved from either auto or manual config
                var ids = {
                    energyIn:  -1,
                    energyOut: -1,
                    soc:       -1,
                    watt:      -1,
                    volt:      -1
                };
                ctrl.ids = ids; // exposed to template for ng-href log links

                function parseKwh(str) {
                    if (!str) { return 0; }
                    return parseFloat(String(str).replace(' kWh', '').replace(',', '.')) || 0;
                }

                function applyDevices(result) {
                    if (!result || !result.length) { return; }

                    var byIdx = {};
                    result.forEach(function(item) { byIdx[String(item.idx)] = item; });

                    function get(id) { return id !== -1 ? (byIdx[String(id)] || null) : null; }

                    var bi = get(ids.energyIn);
                    var bo = get(ids.energyOut);

                    if (bi || bo) {
                        ctrl.battery = {
                            importedKwh: bi ? parseKwh(bi.CounterToday) : 0,
                            exportedKwh: bo ? parseKwh(bo.CounterToday) : 0
                        };
                    } else {
                        ctrl.battery = null;
                    }

                    var soc = get(ids.soc);
                    var bw  = get(ids.watt);
                    var bv  = get(ids.volt);

                    if (soc || bw || bv) {
                        ctrl.batteryLive = {
                            soc:     soc ? soc.Data : null,
                            watt:    bw  ? (function(v) { var n = parseFloat(v); return isNaN(n) ? v : Math.round(n) + ' W'; })(bw.Usage || bw.Data) : null,
                            voltage: bv  ? bv.Data : null
                        };
                    } else {
                        ctrl.batteryLive = null;
                    }
                }

                function fetchDevices() {
                    var fetchIds = [];
                    if (ids.energyIn  !== -1) { fetchIds.push(ids.energyIn);  }
                    if (ids.energyOut !== -1) { fetchIds.push(ids.energyOut); }
                    if (ids.soc       !== -1) { fetchIds.push(ids.soc);       }
                    if (ids.watt      !== -1) { fetchIds.push(ids.watt);      }
                    if (ids.volt      !== -1) { fetchIds.push(ids.volt);      }

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
                            ids.energyIn  = s.idBatteryEnergyIn  || -1;
                            ids.energyOut = s.idBatteryEnergyOut || -1;
                            ids.soc       = s.idBatterySoc       || -1;
                            ids.watt      = s.idBatteryWatt      || -1;
                            ids.volt      = s.idBatteryVolt      || -1;
                        }
                        fetchDevices();
                    }).catch(function() {
                        fetchDevices();
                    });
                }

                function loadWithManual(cfg) {
                    ids.energyIn  = cfg.idBatteryEnergyIn  || -1;
                    ids.energyOut = cfg.idBatteryEnergyOut || -1;
                    ids.soc       = cfg.idBatterySoc       || -1;
                    ids.watt      = cfg.idBatteryWatt      || -1;
                    ids.volt      = cfg.idBatteryVolt      || -1;
                    fetchDevices();
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title = cfg.title || 'Battery';

                    var useAuto = cfg.useEnergyDashboard !== false;
                    ctrl.configured = useAuto || !!(
                        cfg.idBatteryEnergyIn || cfg.idBatteryEnergyOut ||
                        cfg.idBatterySoc || cfg.idBatteryWatt || cfg.idBatteryVolt
                    );

                    if (!ctrl.configured) { return; }

                    if (useAuto) {
                        loadWithAuto();
                    } else {
                        loadWithManual(cfg);
                    }
                }

                ctrl.netKwh = function() {
                    if (!ctrl.battery) { return null; }
                    return ctrl.battery.importedKwh - ctrl.battery.exportedKwh;
                };

                ctrl.netClass = function() {
                    var n = ctrl.netKwh();
                    if (n === null) { return ''; }
                    return n >= 0 ? 'dd-energy-import' : 'dd-energy-export';
                };

                $scope.$on('device_update', function(e, updated) {
                    var idx = String(updated.idx);
                    if (idx === String(ids.energyIn)  || idx === String(ids.energyOut) ||
                        idx === String(ids.soc)        || idx === String(ids.watt)      ||
                        idx === String(ids.volt)) {
                        fetchDevices();
                    }
                });

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg && JSON.stringify({
                            useAuto: cfg.useEnergyDashboard,
                            ein:     cfg.idBatteryEnergyIn,
                            eout:    cfg.idBatteryEnergyOut,
                            soc:     cfg.idBatterySoc,
                            watt:    cfg.idBatteryWatt,
                            volt:    cfg.idBatteryVolt
                        });
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
