define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'text-note',
        label:       'Text Note',
        description: 'Plain text note with font and color options',
        category:    'Custom Content',
        icon:        'fa-solid fa-pencil',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        12,
        configSchema: [
            { key: 'content',    type: 'textarea', label: 'Text content',      required: false },
            { key: 'fontSize',   type: 'number',   label: 'Font size (px)',    default: 14, min: 8, max: 72 },
            { key: 'fontFamily', type: 'select',   label: 'Font',
              options: [
                  { value: 'inherit',            label: 'Default' },
                  { value: 'serif',              label: 'Serif' },
                  { value: 'monospace',          label: 'Monospace' },
                  { value: 'Arial, sans-serif',  label: 'Arial' },
                  { value: 'Georgia, serif',     label: 'Georgia' }
              ]
            },
            { key: 'textColor',  type: 'color',  label: 'Text color' },
            { key: 'bgColor',    type: 'color',  label: 'Background color' },
            { key: 'textAlign',  type: 'select', label: 'Alignment',
              options: [
                  { value: 'left',   label: 'Left' },
                  { value: 'center', label: 'Center' },
                  { value: 'right',  label: 'Right' }
              ]
            }
        ]
    });

    app.directive('db2TextNoteWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/text-note.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', function($scope) {
                var ctrl = this;

                ctrl.getStyle = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    return {
                        'font-size':        (cfg.fontSize   || 14) + 'px',
                        'font-family':       cfg.fontFamily  || 'inherit',
                        'color':             cfg.textColor   || '',
                        'background-color':  cfg.bgColor     || '',
                        'text-align':        cfg.textAlign   || 'left'
                    };
                };
            }]
        };
    }]);
});
