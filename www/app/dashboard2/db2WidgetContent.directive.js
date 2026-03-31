define([
    'app',
    'dashboard2/dashboard2.module',
    'dashboard2/widgetRegistry.service'
], function(app) {
    'use strict';

    /**
     * db2-widget-content  (attribute directive)
     *
     * Dynamically loads and compiles the correct widget directive for the given
     * widget type.  Each widget module registers itself in widgetRegistry with a
     * descriptor; this directive uses that descriptor to lazy-load the module via
     * $ocLazyLoad and then $compile the widget's own element directive into the DOM.
     *
     * Usage (from widget-wrapper.html):
     *   <div db2-widget-content
     *        widget-def="ctrl.widgetDef"
     *        edit-mode="editMode"></div>
     */
    app.directive('db2WidgetContent', ['$compile', 'widgetRegistry',
        function($compile, widgetRegistry) {
        return {
            restrict: 'A',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            link: function(scope, element) {

                var compiledScope = null;

                function renderWidget(type) {
                    // Destroy previous compiled scope and element
                    if (compiledScope) {
                        compiledScope.$destroy();
                        compiledScope = null;
                    }
                    element.empty();

                    if (!type) { return; }

                    var descriptor = widgetRegistry.get(type);

                    if (!descriptor) {
                        element.html(
                            '<div class="db2-widget-error">' +
                            '<i class="fa-solid fa-triangle-exclamation"></i> ' +
                            'Unknown widget type: ' + type +
                            '</div>'
                        );
                        return;
                    }

                    // All widgets are pre-loaded — compile directly
                    var tag = descriptor.directiveTag || ('db2-' + type + '-widget');
                    var html = '<' + tag +
                               ' widget-def="widgetDef"' +
                               ' edit-mode="editMode">' +
                               '</' + tag + '>';
                    compiledScope = scope.$new(false);
                    var compiled = $compile(html)(compiledScope);
                    element.empty();
                    element.append(compiled);
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
