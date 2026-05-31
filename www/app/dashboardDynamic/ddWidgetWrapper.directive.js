define([
    'app',
    'dashboardDynamic/dashboardDynamic.module',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/ddWidgetContent.directive'
], function(app) {
    'use strict';

    /**
     * dd-widget-wrapper  (attribute directive)
     *
     * Usage (from ddGrid):
     *   <div dd-widget-wrapper
     *        widget-def="widgetObject"
     *        edit-mode="editMode"
     *        on-remove="removeWidget(id)"
     *        on-configure="configureWidget(id)">
     *   </div>
     *
     * Wraps every grid widget in the standard chrome:
     * header bar (title + icon + action buttons), error state, and content area.
     * The actual widget content is compiled by ddWidgetContent.
     */
    app.directive('ddWidgetWrapper', ['widgetRegistry', function(widgetRegistry) {
        return {
            restrict:         'A',
            templateUrl:      'views/dashboardDynamic/widget-wrapper.html',
            scope: {
                widgetDef:   '=',
                editMode:    '<',
                onRemove:    '&',
                onConfigure: '&',
                onClone:     '&'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', function($scope) {
                var ctrl = this;

                ctrl.error      = null;
                ctrl.descriptor = null;

                ctrl.$onInit = function() {
                    if (ctrl.widgetDef && ctrl.widgetDef.type) {
                        ctrl.descriptor = widgetRegistry.get(ctrl.widgetDef.type);
                        if (!ctrl.descriptor) {
                            ctrl.error = 'Unknown widget type: ' + ctrl.widgetDef.type;
                        }
                    }
                    ctrl.isTransparent = function() {
                        // transparentBackground in the descriptor always wins — the widget
                        // controls its own card background; the outer shell is always transparent.
                        if (ctrl.descriptor && ctrl.descriptor.transparentBackground) {
                            return true;
                        }
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        if (cfg && typeof cfg.showBackground === 'boolean') {
                            return !cfg.showBackground;
                        }
                        return false;
                    };
                };

                // Template-facing helpers — call the isolate-scope bindings
                $scope.onRemove    = function() { ctrl.onRemove(); };
                $scope.onConfigure = function() { ctrl.onConfigure(); };
                $scope.onClone     = function() { ctrl.onClone(); };

                // Keep template-accessible editMode in sync with bound value
                $scope.$watch(function() { return ctrl.editMode; }, function(v) {
                    $scope.editMode = v;
                });
            }]
        };
    }]);
});
