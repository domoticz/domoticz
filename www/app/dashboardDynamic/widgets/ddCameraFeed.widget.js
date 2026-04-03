define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'camera-feed',
        label:       'Camera Feed',
        description: 'Live snapshot from a configured IP camera',
        category:    'Controls',
        icon:        'fa-solid fa-video',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        6,
        maxH:        6,
        configSchema: [
            {
                key:      'cameraIdx',
                type:     'camera-picker',
                label:    'Camera',
                required: true
            },
            {
                key:     'refreshInterval',
                type:    'number',
                label:   'Refresh interval (seconds)',
                default: 5
            },
            {
                key:     'showName',
                type:    'boolean',
                label:   'Show camera name',
                default: true
            }
        ]
    });

    app.directive('ddCameraFeedWidget', ['$interval', '$sce', function($interval, $sce) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/camera-feed.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$sce',
                function($scope, $http, $interval, $sce) {
                var ctrl = this;
                ctrl.imageUrl   = null;
                ctrl.cameraName = '';
                ctrl.error      = null;
                var timer       = null;

                function getInterval() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var secs = parseInt(cfg.refreshInterval, 10);
                    return (isNaN(secs) || secs < 1) ? 5 : secs;
                }

                function buildUrl(idx) {
                    // Cache-bust with timestamp so browser doesn't serve stale image
                    return $sce.trustAsResourceUrl('camsnapshot.jpg?idx=' + encodeURIComponent(idx) + '&_t=' + Date.now());
                }

                function refresh() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.cameraIdx) {
                        ctrl.imageUrl = null;
                        return;
                    }
                    ctrl.imageUrl = buildUrl(cfg.cameraIdx);
                }

                function loadCameraName() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.cameraIdx || cfg.showName === false) {
                        ctrl.cameraName = '';
                        return;
                    }
                    $http.get('json.htm', { params: { type: 'command', param: 'getcameras', order: 'Name' } })
                        .then(function(resp) {
                            var cameras = (resp.data && resp.data.result) || [];
                            var cam = cameras.find(function(c) {
                                return String(c.idx) === String(cfg.cameraIdx);
                            });
                            ctrl.cameraName = cam ? cam.Name : '';
                        })
                        .catch(function() { ctrl.cameraName = ''; });
                }

                function startTimer() {
                    stopTimer();
                    timer = $interval(refresh, getInterval() * 1000);
                }

                function stopTimer() {
                    if (timer) { $interval.cancel(timer); timer = null; }
                }

                ctrl.$onInit = function() {
                    refresh();
                    loadCameraName();
                    startTimer();
                };

                $scope.$on('$destroy', stopTimer);
                $scope.$on('dd:widget:refresh', function() {
                    refresh();
                    loadCameraName();
                });

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.cameraIdx + '|' + cfg.refreshInterval + '|' + cfg.showName) : '';
                    },
                    function(val, old) {
                        if (val !== old) {
                            stopTimer();
                            refresh();
                            loadCameraName();
                            startTimer();
                        }
                    }
                );
            }]
        };
    }]);
});
