define([
    'app',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddVisibility.service'
], function(app, widgetRegistry, ddVisibility) {
    'use strict';

    widgetRegistry.register({
        type:        'iframe-embed',
        label:       'Website Embed',
        description: 'Embed an external website via sandboxed iframe',
        category:    'Custom Content',
        icon:        'fa-solid fa-globe',
        defaultW:    4,
        defaultH:    4,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        16,
        configSchema: [
            { key: 'url',             type: 'url',     label: 'URL (https:// recommended)', required: true },
            { key: 'title',           type: 'text',    label: 'Title',                      required: false },
            { key: 'allowScripts',    type: 'boolean', label: 'Allow scripts (\u26a0 trusted sources only)',
              default: false },
            { key: 'refreshInterval', type: 'number',  step: 1, label: 'Auto-reload (seconds, 0=off)', default: 0 }
        ]
    });

    app.directive('ddIframeEmbedWidget', ['$sce', '$interval', '$timeout', function($sce, $interval, $timeout) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/iframe-embed.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$interval', '$timeout', '$sce', 'ddVisibility', function($scope, $interval, $timeout, $sce, ddVisibility) {
                var ctrl  = this;
                ctrl.trustedUrl   = null;
                ctrl.urlError     = false;
                ctrl.sandboxAttr  = '';
                var refreshTimer;

                function validateAndTrust() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var url = (cfg.url || '').trim();

                    if (!url) {
                        ctrl.trustedUrl = null;
                        ctrl.urlError   = false;
                        return;
                    }

                    // Only http and https are accepted — reject javascript:, data:, file:, etc.
                    if (!/^https?:\/\//i.test(url)) {
                        ctrl.trustedUrl = null;
                        ctrl.urlError   = true;
                        return;
                    }

                    ctrl.urlError = false;
                    var parts = ['allow-same-origin', 'allow-forms', 'allow-popups-to-escape-sandbox'];
                    if (cfg.allowScripts) {
                        parts.push('allow-scripts');
                    }
                    ctrl.sandboxAttr = parts.join(' ');
                    ctrl.trustedUrl  = $sce.trustAsResourceUrl(url);
                }

                function refresh() {
                    // Briefly clear then restore to force iframe reload
                    var saved       = ctrl.trustedUrl;
                    ctrl.trustedUrl = null;
                    $timeout(function() { ctrl.trustedUrl = saved; }, 100);
                }

                function stopTimer() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); refreshTimer = null; }
                }

                function startTimer() {
                    stopTimer();
                    var cfg      = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var interval = parseInt(cfg.refreshInterval) || 0;
                    // Minimum 5 seconds to avoid hammering the target site
                    if (interval > 0 && interval < 5) { interval = 5; }
                    if (interval > 0) {
                        refreshTimer = $interval(refresh, interval * 1000);
                    }
                }

                $scope.$on('dd:page:hidden',  function() { stopTimer(); });
                $scope.$on('dd:page:visible', function() { refresh(); startTimer(); });

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.url + '|' + cfg.allowScripts + '|' + cfg.refreshInterval) : '';
                    },
                    function(val, old) {
                        if (val !== old) {
                            validateAndTrust();
                            startTimer();
                        }
                    }
                );

                $scope.$on('$destroy', function() {
                    if (refreshTimer) { $interval.cancel(refreshTimer); }
                });

                ctrl.$onInit = function() {
                    validateAndTrust();
                    if (!ddVisibility.isHidden()) { startTimer(); }
                };
            }]
        };
    }]);
});
