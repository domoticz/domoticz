define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/dashboardDynamic.module',
    'widgets/dzSceneWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'dz-scene',
        transparentBackground: true,
        label:       'Scene',
        description: 'A scene or group',
        category:    'Devices',
        icon:        'fa-solid fa-film',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        6,
        configSchema: [
            { key: 'sceneIdx', type: 'scene-picker', label: 'Scene', required: true }
        ]
    });

    app.directive('ddDzSceneWidget', ['$http', '$compile',
        function($http, $compile) {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboardDynamic/widgets/dz-scene.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$element', function($scope, $element) {
                var ctrl = this;
                ctrl.scene   = null;
                ctrl.loading = false;
                ctrl.error   = null;

                var innerScope = null;

                function load() {
                    var idx = ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.sceneIdx;
                    if (!idx) {
                        ctrl.scene   = null;
                        ctrl.loading = false;
                        ctrl.error   = null;
                        return;
                    }

                    ctrl.loading = true;
                    ctrl.error   = null;

                    $http.get('json.htm?type=command&param=getscenes')
                        .then(function(resp) {
                            ctrl.loading = false;
                            var results = resp.data && resp.data.result;
                            var scene = null;
                            if (results) {
                                for (var i = 0; i < results.length; i++) {
                                    if (String(results[i].idx) === String(idx)) {
                                        scene = results[i];
                                        break;
                                    }
                                }
                            }
                            if (!scene) {
                                ctrl.error = 'Scene ' + idx + ' not found';
                                return;
                            }
                            renderScene(scene);
                        })
                        .catch(function() {
                            ctrl.loading = false;
                            ctrl.error   = 'Failed to load scene';
                        });
                }

                function renderScene(scene) {
                    ctrl.scene = scene;

                    if (innerScope) {
                        innerScope.$destroy();
                        innerScope = null;
                    }

                    innerScope = $scope.$new(false);
                    innerScope.scene         = scene;
                    innerScope.dashboardType = 0;

                    var html = '<dz-scene-widget scene="scene" dashboard-type="dashboardType"></dz-scene-widget>';

                    var container = $element[0].querySelector('.dd-dz-inner');
                    if (container) {
                        angular.element(container).empty().append($compile(html)(innerScope));
                    }
                }

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.sceneIdx;
                    },
                    function(val, old) {
                        if (val !== old) { load(); }
                    }
                );

                $scope.$on('dd:widget:refresh', load);

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
