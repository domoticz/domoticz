define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry, ddVisibility) {
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
        transparentBackground: true,
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
                step:    1,
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
            controller: ['$scope', '$http', '$interval', '$sce', '$q', '$timeout', 'ddVisibility',
                function($scope, $http, $interval, $sce, $q, $timeout, ddVisibility) {
                var ctrl = this;
                ctrl.imageUrl    = null;
                ctrl.cameraName  = '';
                ctrl.cameraAspect = 1;
                ctrl.error       = null;
                var timer              = null;
                var currentBlobUrl     = null;
                var fetchCanceller     = null;
                var requestTimeoutHnd  = null;

                // Blob URL lifecycle is supported by all browsers we target; this
                // guard provides a graceful fallback for very old environments.
                var blobUrlSupported = (typeof URL !== 'undefined' && typeof URL.createObjectURL === 'function');

                function getInterval() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var secs = parseInt(cfg.refreshInterval, 10);
                    return (isNaN(secs) || secs < 1) ? 5 : secs;
                }

                function revokeCurrent() {
                    if (currentBlobUrl && blobUrlSupported) {
                        URL.revokeObjectURL(currentBlobUrl);
                        currentBlobUrl = null;
                    }
                }

                function clearRequestTimeout() {
                    if (requestTimeoutHnd) {
                        $timeout.cancel(requestTimeoutHnd);
                        requestTimeoutHnd = null;
                    }
                }

                function abortInflight() {
                    clearRequestTimeout();
                    if (fetchCanceller) {
                        fetchCanceller.resolve('cancelled');
                        fetchCanceller = null;
                    }
                }

                function refresh() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.cameraIdx) {
                        ctrl.imageUrl = null;
                        return;
                    }

                    // Abort any previous in-flight request before starting a new one.
                    abortInflight();

                    if (!blobUrlSupported) {
                        // Fallback: old string-URL approach (no blob lifecycle management).
                        ctrl.imageUrl = $sce.trustAsResourceUrl(
                            'camsnapshot.jpg?idx=' + encodeURIComponent(cfg.cameraIdx) + '&_t=' + Date.now()
                        );
                        return;
                    }

                    var thisCanceller = $q.defer();
                    fetchCanceller = thisCanceller;

                    // Auto-cancel if the server takes too long to respond. Without this, a slow or
                    // unavailable camera causes camsnapshot.jpg to hang, accumulating connections that
                    // exhaust the browser's per-host connection pool and freeze all other widgets.
                    var timeoutMs = Math.max(3000, Math.min(10000, (getInterval() - 1) * 1000));
                    requestTimeoutHnd = $timeout(function() {
                        requestTimeoutHnd = null;
                        if (fetchCanceller === thisCanceller) { abortInflight(); }
                    }, timeoutMs);

                    $http.get('camsnapshot.jpg', {
                        params:   { idx: cfg.cameraIdx, _t: Date.now() },
                        responseType: 'blob',
                        timeout:  thisCanceller.promise
                    }).then(function(resp) {
                        clearRequestTimeout();

                        // Guard: widget may have been destroyed while the request was in flight.
                        if ($scope.$$destroyed) { return; }

                        // Reject non-image responses (e.g. camera returning an HTML error page with HTTP 200).
                        if (!resp.data || !resp.data.type || resp.data.type.indexOf('image/') !== 0) { return; }

                        var newBlobUrl = URL.createObjectURL(resp.data);

                        // Release the previous frame's pixel buffer before adopting the new one.
                        revokeCurrent();

                        currentBlobUrl = newBlobUrl;
                        ctrl.imageUrl  = $sce.trustAsResourceUrl(newBlobUrl);
                    }).catch(function() {
                        clearRequestTimeout();
                        // Only clear the canceller if it still refers to this request — a concurrent
                        // refresh() may have already replaced it with a newer deferred.
                        if (fetchCanceller === thisCanceller) { fetchCanceller = null; }
                        // leave currentBlobUrl and ctrl.imageUrl intact — keep last good frame
                    });
                }

                function loadCameraName() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.cameraIdx) {
                        ctrl.cameraName = '';
                        ctrl.cameraAspect = 1;
                        return;
                    }
                    $http.get('json.htm', { params: { type: 'command', param: 'getcameras', order: 'Name' } })
                        .then(function(resp) {
                            var cameras = (resp.data && resp.data.result) || [];
                            var cam = cameras.find(function(c) {
                                return String(c.idx) === String(cfg.cameraIdx);
                            });
                            ctrl.cameraName   = (cam && cfg.showName !== false) ? cam.Name : '';
                            ctrl.cameraAspect = (cam && cam.AspectRatio != null) ? cam.AspectRatio : 1;
                        })
                        .catch(function() { ctrl.cameraName = ''; ctrl.cameraAspect = 1; });
                }

                ctrl.openLiveStream = function() {
                    if (ctrl.editMode) { return; }
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    if (!cfg.cameraIdx) { return; }
                    if (typeof ShowCameraLiveStream === 'function') {
                        ShowCameraLiveStream(escape(ctrl.cameraName || ''), cfg.cameraIdx, ctrl.cameraAspect);
                    }
                };

                function startTimer() {
                    stopTimer();
                    timer = $interval(refresh, getInterval() * 1000);
                }

                function stopTimer() {
                    if (timer) { $interval.cancel(timer); timer = null; }
                }

                function teardown() {
                    stopTimer();
                    abortInflight();
                    revokeCurrent();
                }

                ctrl.$onInit = function() {
                    refresh();
                    loadCameraName();
                    if (!ddVisibility.isHidden()) { startTimer(); }
                };

                $scope.$on('$destroy', teardown);
                $scope.$on('dd:widget:refresh', function() {
                    refresh();
                    loadCameraName();
                });
                $scope.$on('dd:page:hidden', function() { stopTimer(); });
                $scope.$on('dd:page:visible', function() { refresh(); startTimer(); });

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
