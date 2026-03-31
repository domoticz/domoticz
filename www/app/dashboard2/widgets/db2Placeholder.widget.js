define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app) {
    'use strict';

    /**
     * db2-placeholder-widget
     *
     * A minimal stub widget used for testing the widget framework.
     * Real widget types (Features 06-09) follow this same pattern:
     *   1. define() with app + widgetRegistry.service as deps
     *   2. Register a descriptor with widgetRegistry.register(...)
     *   3. Declare an attribute directive named db2-<type>-widget
     */

    // Register the descriptor so the library panel picks it up.
    app.run(['widgetRegistry', function(widgetRegistry) {
        widgetRegistry.register({
            type:        'placeholder',
            label:       'Placeholder',
            description: 'Test widget for framework verification',
            category:    'Other',
            icon:        'fa-solid fa-border-all',
            defaultW:    3,
            defaultH:    2,
            minW:        2,
            minH:        2,
            maxW:        12,
            maxH:        12,
            configSchema: [
                {
                    key:      'title',
                    type:     'text',
                    label:    'Title',
                    required: false
                }
            ]
        });
    }]);

    app.directive('db2PlaceholderWidget', [function() {
        return {
            restrict: 'A',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            template:
                '<div style="display:flex;flex-direction:column;align-items:center;' +
                'justify-content:center;height:100%;color:var(--dz-widget-text);' +
                'opacity:0.5;text-align:center;gap:8px;padding:8px;">' +
                '<i class="fa-solid fa-border-all" style="font-size:28px"></i>' +
                '<span style="font-size:0.85em">Placeholder Widget</span>' +
                '</div>'
        };
    }]);
});
