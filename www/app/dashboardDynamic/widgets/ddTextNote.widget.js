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
            { key: 'content',    type: 'textarea', label: 'Text content',   required: false },
            { key: 'fontSize',   type: 'number',   label: 'Font size (px)', default: 14, min: 8, max: 72 },
            { key: 'fontFamily', type: 'select',   label: 'Font',
              options: [
                  { value: 'inherit',                                    label: 'Default (theme)' },
                  { value: 'Arial, Helvetica, sans-serif',               label: 'Arial' },
                  { value: 'Verdana, Geneva, sans-serif',                label: 'Verdana' },
                  { value: 'Tahoma, Geneva, sans-serif',                 label: 'Tahoma' },
                  { value: '"Trebuchet MS", Helvetica, sans-serif',      label: 'Trebuchet MS' },
                  { value: '"Helvetica Neue", Helvetica, Arial, sans-serif', label: 'Helvetica Neue' },
                  { value: 'Georgia, serif',                             label: 'Georgia' },
                  { value: '"Times New Roman", Times, serif',            label: 'Times New Roman' },
                  { value: '"Palatino Linotype", Palatino, serif',       label: 'Palatino' },
                  { value: '"Courier New", Courier, monospace',          label: 'Courier New' },
                  { value: '"Lucida Console", Monaco, monospace',        label: 'Lucida Console' },
                  { value: 'Impact, "Arial Narrow", sans-serif',         label: 'Impact' },
                  { value: 'system-ui, -apple-system, sans-serif',       label: 'System UI' }
              ]
            },
            { key: 'textAlign',  type: 'select', label: 'Alignment',
              options: [
                  { value: 'left',   label: 'Left' },
                  { value: 'center', label: 'Center' },
                  { value: 'right',  label: 'Right' }
              ]
            },
            { key: 'textColor',   type: 'color', label: 'Text color' },
            { key: 'textOpacity', type: 'range', label: 'Text opacity (%)', default: 100, min: 0, max: 100, step: 5 },
            { key: 'bgColor',     type: 'color', label: 'Background color' },
            { key: 'bgOpacity',   type: 'range', label: 'Background opacity (%)', default: 100, min: 0, max: 100, step: 5 }
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

                function hexToRgba(hex, opacity) {
                    if (!hex) { return undefined; }
                    var h = hex.replace('#', '');
                    if (h.length === 3) { h = h[0]+h[0]+h[1]+h[1]+h[2]+h[2]; }
                    var r = parseInt(h.substr(0, 2), 16);
                    var g = parseInt(h.substr(2, 2), 16);
                    var b = parseInt(h.substr(4, 2), 16);
                    var a = (opacity === undefined || opacity === null) ? 1 : opacity / 100;
                    return 'rgba(' + r + ',' + g + ',' + b + ',' + a + ')';
                }

                ctrl.getStyle = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var bgOp   = cfg.bgOpacity   !== undefined ? Number(cfg.bgOpacity)   : 100;
                    var txtOp  = cfg.textOpacity !== undefined ? Number(cfg.textOpacity) : 100;
                    var align  = cfg.textAlign || 'left';
                    var justifyMap = { left: 'flex-start', center: 'center', right: 'flex-end' };
                    var style  = {
                        'font-size':        (cfg.fontSize  || 14) + 'px',
                        'font-family':       cfg.fontFamily || 'inherit',
                        'text-align':        align,
                        'justify-content':   justifyMap[align] || 'flex-start'
                    };
                    style['color']            = cfg.textColor ? hexToRgba(cfg.textColor, txtOp) : '';
                    style['background-color'] = cfg.bgColor   ? hexToRgba(cfg.bgColor,   bgOp)  : (bgOp < 100 ? 'transparent' : '');
                    return style;
                };
            }]
        };
    }]);
});
