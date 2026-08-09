define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'text-sensor',
        label:       'Text Sensor',
        description: 'Displays the content of a Domoticz Text sensor device',
        category:    'Devices',
        icon:        'fa-solid fa-align-left',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        1,
        maxW:        12,
        maxH:        6,
        transparentBackground: true,
        configSchema: [
            {
                key:          'deviceIdx',
                type:         'device-picker',
                label:        'Device',
                required:     true,
                deviceFilter: 'text'
            },
            {
                key:     'showTitle',
                type:    'boolean',
                label:   'Show device name as title',
                default: true
            },
            {
                key:     'fontSize',
                type:    'number',
                step:    1,
                label:   'Font size (px)',
                default: 14
            },
            {
                key:     'refreshInterval',
                type:    'number',
                step:    1,
                label:   'Refresh interval (seconds)',
                default: 60
            },
            { key: 'showBackground', type: 'boolean', label: 'Show panel background', default: true }
        ]
    });

    app.directive('ddTextSensorWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/text-sensor.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', '$sce', function($scope, $http, $q, $sce) {
                var ctrl = this;
                ctrl.text        = null;
                ctrl.deviceName  = '';
                ctrl.lastUpdate  = '';
                ctrl.showTitle   = true;
                ctrl.fontSize    = 14;
                var cancelToken  = null;

                function applyDevice(d) {
                    var cfg     = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var scopeId = 'dz-dd-' + String(parseInt(d.idx || cfg.deviceIdx, 10) || 0);
                    var raw     = (d.Data || '').trim();
                    var html    = sanitizeHTML(raw, scopeId);
                    ctrl.text       = $sce.trustAsHtml('<div id="' + scopeId + '">' + html + '</div>');
                    ctrl.deviceName = d.Name      || '';
                    ctrl.lastUpdate = d.LastUpdate || '';
                }

                function load() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.deviceIdx) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: cfg.deviceIdx },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        var d = resp.data.result && resp.data.result[0];
                        if (!d) { return; }
                        applyDevice(d);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                function applyConfig() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.showTitle  = cfg.showTitle  !== false;
                    ctrl.fontSize   = cfg.fontSize   || 14;
                }

                $scope.$on('device_update', function(e, updated) {
                    var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                    if (cfg && String(updated.idx) === String(cfg.deviceIdx)) {
                        applyDevice(updated);
                    }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
                    },
                    function(val, old) {
                        if (val !== old) {
                            applyConfig();
                            load();
                        }
                    }
                );

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg && (String(cfg.showTitle) + '|' + cfg.fontSize);
                    },
                    function(val, old) {
                        if (val !== old) { applyConfig(); }
                    }
                );

                ctrl.$onInit = function() {
                    applyConfig();
                    load();
                };
            }]
        };
    }]);
});
