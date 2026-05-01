define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module',
    'widgets/dzLightWidget',
    'widgets/dzUtilityWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'dz-device',
        transparentBackground: true,
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

    app.directive('ddDzDeviceWidget', ['$http', '$compile', 'ddDeviceClassifier',
        function($http, $compile, ddDeviceClassifier) {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboardDynamic/widgets/dz-device.html',
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

                    var directiveName = ddDeviceClassifier.getDirective(device);
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
                    var container = $element[0].querySelector('.dd-dz-inner');
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
                        var newDirective = ddDeviceClassifier.getDirective(updatedDevice);
                        var curDirective = ddDeviceClassifier.getDirective(ctrl.device);
                        if (newDirective !== curDirective || !innerScope || innerScope.$$destroyed) {
                            // Directive type changed or inner scope gone — full re-render needed
                            renderDevice(updatedDevice);
                        } else {
                            // Same directive — update device data in place to avoid DOM teardown flicker
                            ctrl.device = updatedDevice;
                            innerScope.device = updatedDevice;
                        }
                    }
                });

                $scope.$on('dd:widget:refresh', load);
                $scope.$on('dd:page:visible',  load);

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
