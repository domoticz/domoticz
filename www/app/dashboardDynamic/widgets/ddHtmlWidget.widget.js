define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'html-widget',
        directiveTag: 'dd-html-widget',
        label:       'HTML',
        description: 'Display custom HTML content (sanitized)',
        icon:        'fa-solid fa-code',
        category:    'Custom Content',
        defaultW:    4,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        12,
        maxH:        12,
        configSchema: [
            {
                key:      'title',
                label:    'Widget Title',
                type:     'text',
                required: false,
                placeholder: 'HTML'
            },
            {
                key:      'html',
                label:    'HTML Content',
                type:     'textarea',
                required: false,
                help:     'Paste your HTML here. Content is sanitized for security.'
            },
            {
                key:     'allowSameOrigin',
                type:    'boolean',
                label:   'Allow backend API access (\u26a0 trusted content only)',
                help:    'Grants scripts access to the Domoticz API (json.htm). Only enable for HTML you wrote yourself.',
                default: false
            }
        ]
    });

    // Helper directive: sets iframe.srcdoc directly via DOM to avoid Angular encoding issues.
    // Also uses ResizeObserver to re-stamp srcdoc when the iframe dimensions change so that
    // content inside the iframe reflowss when the widget is resized in the grid editor.
    app.directive('ddHtmlFrame', [function() {
        return {
            restrict: 'A',
            link: function(scope, element, attrs) {
                var el          = element[0];
                var lastDoc     = '';
                var lastSandbox = 'allow-scripts';

                function apply() {
                    el.setAttribute('sandbox', lastSandbox);
                    if (lastDoc) { el.srcdoc = lastDoc; }
                }

                scope.$watch(attrs.ddHtmlFrame, function(doc) {
                    lastDoc = doc || '';
                    apply();
                });

                if (attrs.ddHtmlSandbox) {
                    scope.$watch(attrs.ddHtmlSandbox, function(sandbox) {
                        lastSandbox = sandbox || 'allow-scripts';
                        apply();
                    });
                }

                if (typeof ResizeObserver !== 'undefined') {
                    var ro = new ResizeObserver(function() {
                        apply();
                    });
                    ro.observe(el);
                    scope.$on('$destroy', function() { ro.disconnect(); });
                }
            }
        };
    }]);

    app.directive('ddHtmlWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/html-widget.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', function($scope) {
                var ctrl = this;

                ctrl.iframeDoc = '';
                ctrl.sandboxValue = 'allow-scripts';

                // Injected style: override 100vh to fit the iframe, hide scrollbars
                var FIT_STYLE = '<style>html,body{height:100%!important;max-height:100%!important;overflow:hidden!important;box-sizing:border-box}</style>';

                function buildDoc(raw) {
                    if (!raw) { return ''; }
                    var isFullDoc = /<!DOCTYPE|<html/i.test(raw);
                    if (isFullDoc) {
                        // Inject fit style into existing <head>
                        return raw.replace(/(<head[^>]*>)/i, '$1' + FIT_STYLE);
                    }
                    return '<!DOCTYPE html><html><head>' +
                        '<meta charset="utf-8">' + FIT_STYLE +
                        '<style>body{margin:0;padding:4px;font-family:inherit;background:transparent;color:inherit}</style>' +
                        '</head><body>' + raw + '</body></html>';
                }

                function applyConfig() {
                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.sandboxValue = cfg.allowSameOrigin
                        ? 'allow-scripts allow-same-origin'
                        : 'allow-scripts';
                    ctrl.iframeDoc = buildDoc(cfg.html || '');
                }

                $scope.$watch(
                    function() {
                        var cfg = ctrl.widgetDef && ctrl.widgetDef.config;
                        return cfg ? (cfg.html || '') + '|' + !!cfg.allowSameOrigin : '';
                    },
                    applyConfig
                );

                applyConfig();
            }]
        };
    }]);
});
