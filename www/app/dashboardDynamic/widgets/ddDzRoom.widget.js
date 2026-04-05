define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'dz-room',
        label:       'Room / Plan',
        description: 'All devices in a specific room or plan',
        category:    'Devices',
        icon:        'fa-solid fa-house',
        defaultW:    6,
        defaultH:    6,
        minW:        3,
        minH:        4,
        maxW:        12,
        maxH:        20,
        configSchema: [
            { key: 'planIdx', type: 'plan-picker', label: 'Room / Plan', required: true },
            { key: 'title',   type: 'text',   label: 'Custom Title',   required: false }
        ]
    });

    app.directive('ddDzRoomWidget', [function() {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboardDynamic/widgets/dz-room.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', 'ddDeviceClassifier',
                function($scope, $http, $interval, ddDeviceClassifier) {
                var ctrl = this;
                ctrl.devices = [];
                ctrl.loading = false;
                ctrl.error   = null;

                ctrl.isLight = function(d) {
                    return ddDeviceClassifier.getDirective(d) === 'dz-light-widget';
                };

                ctrl.isScene = function(d) {
                    return ddDeviceClassifier.getDirective(d) === 'dz-scene-widget';
                };

                function load() {
                    var cfg    = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var planId = cfg.planIdx;

                    if (!planId) {
                        ctrl.devices = [];
                        ctrl.loading = false;
                        ctrl.error   = null;
                        return;
                    }

                    ctrl.loading = true;
                    ctrl.error   = null;

                    var url = 'json.htm?type=command&param=getdevices&filter=all&used=true&plan=' + planId + '&order=Name';

                    $http.get(url)
                        .then(function(resp) {
                            ctrl.loading = false;
                            ctrl.devices = (resp.data && resp.data.result) || [];
                        })
                        .catch(function() {
                            ctrl.loading = false;
                            ctrl.error   = 'Failed to load room devices';
                        });
                }

                $scope.$on('device_update', function(e, updated) {
                    var idx = String(updated.idx);
                    for (var i = 0; i < ctrl.devices.length; i++) {
                        if (String(ctrl.devices[i].idx) === idx) {
                            ctrl.devices[i] = updated;
                            return;
                        }
                    }
                    load();
                });
                $scope.$on('scene_update', function() { /* scenes not shown in room widget */ });
                $scope.$on('dd:widget:refresh', load);

                $scope.$watch(
                    function() { return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.planIdx; },
                    function(val, old) { if (val !== old) { load(); } }
                );

                var timer = $interval(load, 60000);
                $scope.$on('$destroy', function() { $interval.cancel(timer); });

                ctrl.$onInit = load;
            }]
        };
    }]);
});
