define([
    'app',
    'dashboard2/widgetRegistry.service',
    'dashboard2/dashboard2.module',
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

    app.directive('db2DzRoomWidget', [function() {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboard2/widgets/dz-room.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', 'db2DeviceClassifier',
                function($scope, $http, $interval, db2DeviceClassifier) {
                var ctrl = this;
                ctrl.devices = [];
                ctrl.loading = false;
                ctrl.error   = null;

                ctrl.isLight = function(d) {
                    return db2DeviceClassifier.getDirective(d) === 'dz-light-widget';
                };

                ctrl.isScene = function(d) {
                    return db2DeviceClassifier.getDirective(d) === 'dz-scene-widget';
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

                $scope.$on('device_update', load);
                $scope.$on('scene_update',  load);
                $scope.$on('db2:widget:refresh', load);

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
