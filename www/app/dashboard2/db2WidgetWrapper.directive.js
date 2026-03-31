define([
    'app',
    'dashboard2/dashboard2.module',
    'dashboard2/widgetRegistry.service',
    'dashboard2/db2WidgetContent.directive'
], function(app) {
    'use strict';

    /**
     * db2-widget-wrapper  (attribute directive)
     *
     * Usage (from db2Grid):
     *   <div db2-widget-wrapper
     *        widget-def="widgetObject"
     *        edit-mode="editMode"
     *        on-remove="removeWidget(id)"
     *        on-configure="configureWidget(id)">
     *   </div>
     *
     * Wraps every grid widget in the standard chrome:
     * header bar (title + icon + action buttons), error state, and content area.
     * The actual widget content is compiled by db2WidgetContent.
     */
    app.directive('db2WidgetWrapper', ['widgetRegistry', function(widgetRegistry) {
        return {
            restrict:         'A',
            templateUrl:      'views/dashboard2/widget-wrapper.html',
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
