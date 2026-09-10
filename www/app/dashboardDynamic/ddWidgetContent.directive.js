define([
    'app',
    'dashboardDynamic/dashboardDynamic.module',
    'dashboardDynamic/widgetRegistry.service',
    'dashboardDynamic/customWidgets.service'
], function(app) {
    'use strict';

    /**
     * dd-widget-content  (attribute directive)
     *
     * Dynamically loads and compiles the correct widget directive for the given
     * widget type.  Each widget module registers itself in widgetRegistry with a
     * descriptor; this directive uses that descriptor to compile the widget's own
     * element directive into the DOM.
     *
     * Built-in widgets are all pre-loaded, so they compile straight away. Custom
     * (third-party) widgets are registered from their package manifest without
     * their code being run, so the first time one is rendered its script is
     * fetched and its directive registered before compiling -- see
     * customWidgets.service.js.
     *
     * Usage (from widget-wrapper.html):
     *   <div dd-widget-content
     *        widget-def="ctrl.widgetDef"
     *        edit-mode="editMode"></div>
     */
    app.directive('ddWidgetContent', ['$compile', 'widgetRegistry', 'customWidgets',
        function($compile, widgetRegistry, customWidgets) {
        return {
            restrict: 'A',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            link: function(scope, element) {

                var compiledScope = null;

                function teardown() {
                    if (compiledScope) {
                        compiledScope.$destroy();
                        compiledScope = null;
                    }
                    element.empty();
                }

                function showMessage(cssClass, iconClass, text) {
                    element.html(
                        '<div class="' + cssClass + '">' +
                        '<i class="' + iconClass + '"></i> ' +
                        angular.element('<div>').text(text).html() +
                        '</div>'
                    );
                }

                function compileWidget(descriptor, type) {
                    var tag = descriptor.directiveTag || ('dd-' + type + '-widget');
                    var html = '<' + tag +
                               ' widget-def="widgetDef"' +
                               ' edit-mode="editMode">' +
                               '</' + tag + '>';
                    compiledScope = scope.$new(false);
                    var compiled = $compile(html)(compiledScope);
                    element.empty();
                    element.append(compiled);
                }

                function renderWidget(type) {
                    teardown();

                    if (!type) { return; }

                    var descriptor = widgetRegistry.get(type);

                    if (!descriptor) {
                        showMessage('dd-widget-error', 'fa-solid fa-triangle-exclamation',
                                    'Unknown widget type: ' + type);
                        return;
                    }

                    if (descriptor.custom && !descriptor.$loaded) {
                        showMessage('dd-widget-loading', 'fa-solid fa-circle-notch fa-spin',
                                    'Loading ' + (descriptor.label || type) + '…');

                        customWidgets.load(descriptor).then(function() {
                            // The widget may have been swapped out or the card
                            // removed while its script was in flight.
                            if (scope.$$destroyed || scope.widgetDef.type !== type) { return; }
                            teardown();
                            compileWidget(descriptor, type);
                        }, function(err) {
                            if (scope.$$destroyed || scope.widgetDef.type !== type) { return; }
                            showMessage('dd-widget-error', 'fa-solid fa-triangle-exclamation',
                                        (descriptor.label || type) + ' failed to load: ' + err);
                        });
                        return;
                    }

                    compileWidget(descriptor, type);
                }

                // Re-render whenever the widget type changes (e.g. after a clone/replace)
                scope.$watch('widgetDef.type', function(type) {
                    renderWidget(type);
                });

                // Clean up on destroy
                scope.$on('$destroy', function() {
                    if (compiledScope) {
                        compiledScope.$destroy();
                        compiledScope = null;
                    }
                });


            }
        };
    }]);
});
