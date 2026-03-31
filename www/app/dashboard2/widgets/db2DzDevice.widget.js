define([
    'app',
    'dashboard2/widgetRegistry.service',
    'dashboard2/dashboard2.module',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'dz-device',
        label:       'Device',
        description: 'Any switch, sensor, meter, or actuator (type auto-detected)',
        category:    'Devices',
        icon:        'fa-solid fa-power-off',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        6,
        configSchema: [
            { key: 'deviceIdx', type: 'device-picker', label: 'Device', required: true }
        ]
    });

    app.directive('db2DzDeviceWidget', ['$http', '$compile', 'db2DeviceClassifier',
        function($http, $compile, db2DeviceClassifier) {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboard2/widgets/dz-device.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$element', function($scope, $element) {
                var ctrl = this;
                ctrl.device  = null;
                ctrl.loading = false;
                ctrl.error   = null;

                var innerScope = null;

                function load() {
                    var idx = ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
                    if (!idx) {
                        ctrl.device  = null;
                        ctrl.loading = false;
                        ctrl.error   = null;
                        return;
                    }

                    ctrl.loading = true;
                    ctrl.error   = null;

                    $http.get('json.htm?type=command&param=getdevices&rid=' + idx)
                        .then(function(resp) {
                            ctrl.loading = false;
                            var device = resp.data && resp.data.result && resp.data.result[0];
                            if (!device) {
                                ctrl.error = 'Device ' + idx + ' not found';
                                return;
                            }
                            renderDevice(device);
                        })
                        .catch(function() {
                            ctrl.loading = false;
                            ctrl.error   = 'Failed to load device';
                        });
                }

                function renderDevice(device) {
                    ctrl.device = device;

                    // Destroy the previous inner scope to avoid leaks
                    if (innerScope) {
                        innerScope.$destroy();
                        innerScope = null;
                    }

                    var directiveName = db2DeviceClassifier.getDirective(device);
                    if (!directiveName) {
                        ctrl.error = 'Unknown device type: ' + device.Type;
                        return;
                    }

                    innerScope = $scope.$new(false);
                    innerScope.device        = device;
                    innerScope.dashboardType = 0;

                    var html;
                    if (directiveName === 'dz-scene-widget') {
                        innerScope.scene = device;
                        html = '<dz-scene-widget scene="scene" dashboard-type="dashboardType"></dz-scene-widget>';
                    } else {
                        html = '<' + directiveName +
                               ' device="device"' +
                               ' dashboard-type="dashboardType">' +
                               '</' + directiveName + '>';
                    }

                    // Wait for ng-show to expose the container before appending
                    var container = $element[0].querySelector('.db2-dz-inner');
                    if (container) {
                        angular.element(container).empty().append($compile(html)(innerScope));
                    }
                }

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.deviceIdx;
                    },
                    function(val, old) {
                        if (val !== old) { load(); }
                    }
                );

                $scope.$on('device_update', function(e, updatedDevice) {
                    if (ctrl.device && String(updatedDevice.idx) === String(ctrl.device.idx)) {
                        renderDevice(updatedDevice);
                    }
                });

                $scope.$on('db2:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (innerScope) {
                        innerScope.$destroy();
                        innerScope = null;
                    }
                });

                ctrl.$onInit = load;
            }]
        };
    }]);
});
