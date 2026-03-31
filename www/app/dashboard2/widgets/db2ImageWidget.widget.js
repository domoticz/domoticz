define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'image-widget',
        label:       'Remote Image',
        description: 'Display a remote image with optional auto-refresh',
        category:    'Custom Content',
        icon:        'fa-solid fa-image',
        defaultW:    3,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        12,
        directiveTag: 'db2-image-widget',
        configSchema: [
            { key: 'url',             type: 'url',    label: 'Image URL',                          required: true },
            { key: 'title',           type: 'text',   label: 'Caption',                            required: false },
            { key: 'objectFit',       type: 'select', label: 'Fit',
              options: [
                  { value: 'contain', label: 'Contain (full image visible)' },
                  { value: 'cover',   label: 'Cover (fill area, crop)' },
                  { value: 'fill',    label: 'Stretch to fill' }
              ],
              default: 'contain'
            },
            { key: 'refreshInterval', type: 'number', label: 'Refresh every N seconds (0=off)', default: 0 }
        ]
    });

    app.directive('db2ImageWidget', ['$sce', '$interval', function($sce, $interval) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/image-widget.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$interval', '$sce', function($scope, $interval, $sce) {
                var ctrl = this;
                ctrl.validUrl    = null;   // the validated base URL string
                ctrl.displayUrl  = null;   // $sce-trusted URL (possibly cache-busted)
                ctrl.loadError   = false;
                ctrl.urlError    = false;
                var refreshTimer;

                function buildDisplayUrl() {
                    if (!ctrl.validUrl) { ctrl.displayUrl = null; return; }
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = parseInt(cfg.refreshInterval) || 0;
                    var url      = ctrl.validUrl;
                    if (interval > 0) {
                        url += (url.indexOf('?') >= 0 ? '&' : '?') + '_t=' + Date.now();
                    }
                    ctrl.displayUrl = $sce.trustAsResourceUrl(url);
                    ctrl.loadError  = false;
                }

                function validateUrl() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var url = (cfg.url || '').trim();

                    if (!url) {
                        ctrl.validUrl   = null;
                        ctrl.displayUrl = null;
                        ctrl.urlError   = false;
                        return;
                    }

                    // Only allow http/https image URLs
                    if (!/^https?:\/\//i.test(url)) {
                        ctrl.validUrl   = null;
                        ctrl.displayUrl = null;
                        ctrl.urlError   = true;
                        return;
                    }

                    ctrl.urlError = false;
                    ctrl.validUrl = url;
                    buildDisplayUrl();
                }

                function scheduleRefresh() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = parseInt(cfg.refreshInterval) || 0;
                    if (interval > 0) {
                        refreshTimer = $interval(buildDisplayUrl, interval * 1000);
                    }
                }

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.url + '|' + cfg.refreshInterval) : '';
                    },
                    function(val, old) {
                        if (val !== old) {
                            validateUrl();
                            scheduleRefresh();
                        }
                    }
                );

                $scope.$on('$destroy', function() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); }
                });

                ctrl.$onInit = function() {
                    validateUrl();
                    scheduleRefresh();
                };
            }]
        };
    }]);
});
