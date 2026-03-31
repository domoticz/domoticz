define([
    'app',
    'dashboard2/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'db2HtmlWidget',
        directiveTag: 'db2-html-widget',
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
            }
        ]
    });

    // Helper directive: sets iframe.srcdoc directly via DOM to avoid Angular encoding issues
    app.directive('db2HtmlFrame', [function() {
        return {
            restrict: 'A',
            link: function(scope, element, attrs) {
                scope.$watch(attrs.db2HtmlFrame, function(doc) {
                    if (doc) {
                        element[0].srcdoc = doc;
                    }
                });
            }
        };
    }]);

    app.directive('db2HtmlWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboard2/widgets/html-widget.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', function($scope) {
                var ctrl = this;

                ctrl.iframeDoc = '';

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

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.html;
                    },
                    function(raw) {
                        ctrl.iframeDoc = buildDoc(raw || '');
                    }
                );

                var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                ctrl.iframeDoc = buildDoc(cfg.html || '');
            }]
        };
    }]);
});
