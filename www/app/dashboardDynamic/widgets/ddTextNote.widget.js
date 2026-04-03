define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
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
        minH:        1,
        maxW:        12,
        maxH:        12,
        configSchema: [
            { key: 'content', type: 'textarea', label: 'Text content', required: false },
            {
                type: 'group',
                fields: [
                    { key: 'fontFamily', type: 'select', label: 'Font',
                      default: 'inherit',
                      options: [
                          { value: 'inherit',                                        label: 'Default (theme)' },
                          { value: 'Arial, Helvetica, sans-serif',                   label: 'Arial' },
                          { value: 'Verdana, Geneva, sans-serif',                    label: 'Verdana' },
                          { value: 'Tahoma, Geneva, sans-serif',                     label: 'Tahoma' },
                          { value: '"Trebuchet MS", Helvetica, sans-serif',          label: 'Trebuchet MS' },
                          { value: '"Helvetica Neue", Helvetica, Arial, sans-serif', label: 'Helvetica Neue' },
                          { value: 'Georgia, serif',                                 label: 'Georgia' },
                          { value: '"Times New Roman", Times, serif',                label: 'Times New Roman' },
                          { value: '"Palatino Linotype", Palatino, serif',           label: 'Palatino' },
                          { value: '"Courier New", Courier, monospace',              label: 'Courier New' },
                          { value: '"Lucida Console", Monaco, monospace',            label: 'Lucida Console' },
                          { value: 'Impact, "Arial Narrow", sans-serif',             label: 'Impact' },
                          { value: 'system-ui, -apple-system, sans-serif',           label: 'System UI' }
                      ]
                    },
                    { key: 'fontSize', type: 'number', label: 'Size (px)', default: 14, min: 8, max: 72 }
                ]
            },
            {
                type: 'group',
                fields: [
                    { key: 'textAlign', type: 'select', label: 'Alignment',
                      options: [
                          { value: 'left',   label: 'Left' },
                          { value: 'center', label: 'Center' },
                          { value: 'right',  label: 'Right' }
                      ],
                      default: 'center'
                    },
                    { key: 'textStyle', type: 'select', label: 'Style',
                      options: [
                          { value: 'normal',      label: 'Normal' },
                          { value: 'bold',        label: 'Bold' },
                          { value: 'italic',      label: 'Italic' },
                          { value: 'bold-italic', label: 'Bold + Italic' },
                          { value: 'underline',   label: 'Underline' }
                      ],
                      default: 'normal'
                    }
                ]
            },
            { key: 'textColor', type: 'color-alpha', label: 'Text color', default: 'rgba(255,255,255,1)' },
            { key: 'bgColor',   type: 'color-alpha', label: 'Background color', default: 'rgba(0,0,0,0)' }
        ]
    });

    app.directive('ddTextNoteWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/text-note.html',
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
                    var align = cfg.textAlign || 'left';
                    var ts    = cfg.textStyle || 'normal';
                    var justifyMap = { left: 'flex-start', center: 'center', right: 'flex-end' };
                    return {
                        'font-size':       (cfg.fontSize   || 14) + 'px',
                        'font-family':      cfg.fontFamily || 'inherit',
                        'text-align':       align,
                        'justify-content':  justifyMap[align] || 'flex-start',
                        'font-weight':      (ts === 'bold' || ts === 'bold-italic') ? 'bold' : 'normal',
                        'font-style':       (ts === 'italic' || ts === 'bold-italic') ? 'italic' : 'normal',
                        'text-decoration':  ts === 'underline' ? 'underline' : 'none',
                        'color':            cfg.textColor || '',
                        'background-color': cfg.bgColor   || ''
                    };
                };
            }]
        };
    }]);
});
