define([
    'app',
    'dashboard2/widgetRegistry.service',
    'widgets/dzSceneWidget'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'dz-scene',
        label:       'Scene / Group',
        description: 'Activate a scene or toggle a group',
        category:    'Devices',
        icon:        'fa-solid fa-circle-play',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        4,
        configSchema: [
            { key: 'sceneIdx', type: 'scene-picker', label: 'Scene or Group', required: true }
        ]
    });

    app.directive('db2DzSceneWidget', ['$http', '$compile',
        function($http, $compile) {
        return {
            restrict:         'EA',
            templateUrl:      'views/dashboard2/widgets/dz-scene.html',
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

                    $http.get('json.htm?type=command&param=getscenes&rid=' + idx)
                        .then(function(resp) {
                            ctrl.loading = false;
                            var scene = resp.data && resp.data.result && resp.data.result[0];
                            if (!scene) {
                                ctrl.error = 'Scene not found';
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
                    var container = $element[0].querySelector('.db2-dz-inner');
                    if (container) {
                        angular.element(container).empty().append($compile(html)(innerScope));
                    }
                }

                $scope.$on('scene_update', function(e, updated) {
                    if (ctrl.scene && String(updated.idx) === String(ctrl.scene.idx)) {
                        load();
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
