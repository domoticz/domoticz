define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'text-note',
        transparentBackground: true,
        label:       'Text Note',
        description: 'Text note with font/color options and optional sanitized HTML content',
        category:    'Custom Content',
        icon:        'fa-solid fa-pencil',
        defaultW:    3,
        defaultH:    2,
        minW:        2,
        minH:        1,
        maxW:        12,
        maxH:        12,
        configSchema: [
            { key: 'content', type: 'textarea', label: 'Text content',
              help: 'Plain text or a safe subset of HTML/CSS. Scripts, iframes, and event handlers are stripped (DOMPurify).',
              required: false },
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
                    { key: 'fontSize', type: 'number', step: 1, label: 'Size (px)', default: 14, min: 8, max: 72 }
                ]
            },
            {
                type: 'group',
                fields: [
                    { key: 'textAlign', type: 'select', label: 'Horizontal align',
                      options: [
                          { value: 'left',   label: 'Left' },
                          { value: 'center', label: 'Center' },
                          { value: 'right',  label: 'Right' }
                      ],
                      default: 'center'
                    },
                    { key: 'verticalAlign', type: 'select', label: 'Vertical align',
                      options: [
                          { value: 'top',    label: 'Top' },
                          { value: 'middle', label: 'Middle' },
                          { value: 'bottom', label: 'Bottom' }
                      ],
                      default: 'middle'
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
            {
                type: 'group',
                fields: [
                    { key: 'textColor', type: 'color-alpha', label: 'Text color',       default: 'rgba(255,255,255,1)' },
                    { key: 'bgColor',   type: 'color-alpha', label: 'Background color', default: 'rgba(0,0,0,0)' }
                ]
            },
            { key: 'showDivider',  type: 'boolean',    label: 'Show divider line', default: false },
            { key: 'dividerColor', type: 'color-alpha', label: 'Divider color (leave empty for accent color)', default: '' }
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
            controller: ['$scope', '$sce', function($scope, $sce) {
                var ctrl = this;

                ctrl.scopeId = 'dd-tn-' + Math.random().toString(36).slice(2, 10);

                ctrl.renderedHtml = null;

                function rebuildHtml() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var raw = cfg.content || '';
                    if (!raw) {
                        ctrl.renderedHtml = null;
                        return;
                    }
                    // sanitizeHTML (www/js/domoticzdevices.js) runs DOMPurify with a strict
                    // allow-list and CSS-scopes any embedded <style> blocks to scopeId.
                    var clean = (typeof sanitizeHTML === 'function')
                        ? sanitizeHTML(raw, ctrl.scopeId)
                        : raw;
                    ctrl.renderedHtml = $sce.trustAsHtml(
                        '<div id="' + ctrl.scopeId + '">' + clean + '</div>'
                    );
                }

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.content || '') : '';
                    },
                    rebuildHtml
                );

                ctrl.getStyle = function() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var halign = cfg.textAlign     || 'center';
                    var valign = cfg.verticalAlign || 'middle';
                    var ts     = cfg.textStyle     || 'normal';
                    var alignItemsMap   = { left: 'flex-start', center: 'center', right: 'flex-end' };
                    var justifyContentMap = { top: 'flex-start', middle: 'center', bottom: 'flex-end' };
                    return {
                        'font-size':        (cfg.fontSize   || 14) + 'px',
                        'font-family':      cfg.fontFamily || 'inherit',
                        'text-align':       halign,
                        'align-items':      alignItemsMap[halign]    || 'center',
                        'justify-content':  justifyContentMap[valign] || 'center',
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
