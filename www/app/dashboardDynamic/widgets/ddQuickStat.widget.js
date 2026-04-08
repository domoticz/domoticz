define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'quick-stat',
        label:       'Quick Stat',
        description: 'Compact multi-device status list — shows current values for temperature, switches, kWh, and more',
        category:    'Custom Content',
        icon:        'fa-solid fa-list',
        defaultW:    3,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        8,
        configSchema: [
            {
                type:   'group',
                spread: true,
                fields: [
                    { key: 'layout',         type: 'boolean', label: 'List layout',           default: false },
                    { key: 'showBackground', type: 'boolean', label: 'Show panel background',  default: true }
                ]
            },
            { key: 'fontSize', type: 'select', label: 'Font size',
              options: [
                  { value: '',       label: 'Default' },
                  { value: 'larger', label: 'Larger' },
                  { value: 'large',  label: 'Large' },
                  { value: 'xl',     label: 'Extra large' }
              ],
              default: ''
            },
            {
                key:   'devices',
                type:  'device-list',
                label: 'Devices'
            }
        ]
    });

    app.directive('ddQuickStatWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/quick-stat.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', 'ddDeviceClassifier', function($scope, $http, $interval, $q, ddDeviceClassifier) {
                var ctrl = this;
                ctrl.items   = [];
                ctrl.loading = false;
                ctrl.error   = null;

                var cancelToken = null;
                var timer       = null;

                function applyDeviceToItem(item, d) {
                    var extracted  = ddDeviceClassifier.extractDeviceValue(d);
                    item.value       = extracted.value;
                    item.secondValue = extracted.secondValue || null;
                    item.isOn        = extracted.isOn;
                    item.unit        = extracted.unit;
                    item.unit2       = extracted.unit2 || null;
                    item.typeClass   = extracted.typeClass;
                    item.icon        = item.configIcon || ddDeviceClassifier.autoDeviceIcon(d);
                    item.label       = item.configLabel || d.Name || String(d.idx);
                }

                function loadAll() {
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var devsCfg  = cfg.devices;

                    if (!Array.isArray(devsCfg)) {
                        try { devsCfg = JSON.parse(devsCfg); } catch (e) { devsCfg = []; }
                    }
                    if (!devsCfg || devsCfg.length === 0) {
                        ctrl.items = [];
                        return;
                    }

                    var idxList = devsCfg.map(function(e) { return e.idx; }).join(',');

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    ctrl.loading = true;
                    ctrl.error   = null;

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: idxList },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        ctrl.loading = false;
                        var results  = (resp.data && resp.data.result) || [];

                        // Build a response lookup keyed by idx
                        var resultByIdx = {};
                        results.forEach(function(d) {
                            resultByIdx[String(d.idx)] = d;
                        });

                        // Rebuild items in config order
                        ctrl.items = devsCfg.map(function(entry) {
                            var item = {
                                idx:         String(entry.idx),
                                configLabel: (entry.label || '').trim() || null,
                                configIcon:  (entry.icon  || '').trim() || null,
                                value:       '\u2014',
                                secondValue: null,
                                isOn:        false,
                                unit:        '',
                                unit2:       null,
                                typeClass:   'generic',
                                label:       (entry.label || '').trim() || String(entry.idx),
                                icon:        (entry.icon  || '').trim() || 'fa-solid fa-circle-dot'
                            };
                            var d = resultByIdx[item.idx];
                            if (d) {
                                applyDeviceToItem(item, d);
                            }
                            return item;
                        });
                    }).catch(function(err) {
                        ctrl.loading = false;
                        if (err && err.status === -1) { return; }
                        ctrl.error = 'Failed to load devices';
                    });
                }

                $scope.$on('device_update', function(e, updated) {
                    var updIdx = String(updated.idx);
                    (ctrl.items || []).forEach(function(item) {
                        if (item.idx !== updIdx) { return; }
                        applyDeviceToItem(item, updated);
                    });
                });

                $scope.$on('dd:widget:refresh', loadAll);

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.devices;
                    },
                    function(val, old) {
                        if (val !== old) { loadAll(); }
                    }
                );

                timer = $interval(loadAll, 30000);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                    if (timer) { $interval.cancel(timer); timer = null; }
                });

                ctrl.$onInit = loadAll;
            }]
        };
    }]);
});
