/*
 * Example custom widget for the Domoticz dynamic dashboard.
 *
 * A widget module is an AMD module that exports either an Angular directive
 * definition object, or -- as here -- a factory that receives the package
 * context and returns one. Use the factory form when the widget needs to build
 * urls inside its own package; ctx.baseUrl is where the package was installed.
 *
 * Domoticz registers the directive itself, so the module must not touch the
 * 'app' module or pick its own element name.
 *
 * Every widget is handed two bindings on its scope:
 *   widgetDef  the placed widget, whose .config holds the user's answers to
 *              the configSchema in widget.json. Watch it; the user can change
 *              a setting while the widget is on screen.
 *   editMode   true while the dashboard is being edited.
 *
 * See docs/custom-widgets.md for the full contract.
 */
define(function() {
    'use strict';

    return function(ctx) {
        return {
            templateUrl: ctx.templateUrl,

            controller: ['$scope', '$http', function($scope, $http) {
                var ctrl = this;

                ctrl.loading = true;
                ctrl.error   = null;
                ctrl.device  = null;

                function config() {
                    return (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                }

                function applyConfig() {
                    var cfg = config();
                    ctrl.title       = cfg.title || '';
                    ctrl.accent      = cfg.accent || '#4e9af1';
                    ctrl.decimals    = angular.isNumber(cfg.decimals) ? cfg.decimals : 1;
                    ctrl.showUpdated = cfg.showUpdated !== false;
                }

                function load() {
                    var idx = config().deviceIdx;
                    if (!idx) {
                        ctrl.loading = false;
                        ctrl.error   = 'Pick a device in this widget\'s settings';
                        return;
                    }
                    ctrl.error = null;
                    $http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: idx } })
                        .then(function(resp) {
                            var result = resp.data && resp.data.result;
                            ctrl.loading = false;
                            if (!result || !result.length) {
                                ctrl.error = 'Device ' + idx + ' not found';
                                return;
                            }
                            ctrl.device = result[0];
                        }, function() {
                            ctrl.loading = false;
                            ctrl.error   = 'Could not read device ' + idx;
                        });
                }

                // Domoticz pushes a 'device_update' for every device that changes,
                // so filter to the one this widget is showing.
                $scope.$on('device_update', function(event, updated) {
                    if (ctrl.device && String(updated.idx) === String(ctrl.device.idx)) {
                        ctrl.device = angular.extend({}, ctrl.device, updated);
                    }
                });

                // Broadcast when the user hits refresh on the dashboard.
                $scope.$on('dd:widget:refresh', load);

                // Re-read whenever the user changes this widget's settings.
                $scope.$watch(function() { return config(); }, function() {
                    var previousIdx = ctrl.$idx;
                    applyConfig();
                    ctrl.$idx = config().deviceIdx;
                    if (ctrl.$idx !== previousIdx) {
                        ctrl.loading = true;
                        ctrl.device  = null;
                        load();
                    }
                }, true);

                ctrl.$onInit = function() {
                    applyConfig();
                    ctrl.$idx = config().deviceIdx;
                    load();
                };
            }]
        };
    };
});
