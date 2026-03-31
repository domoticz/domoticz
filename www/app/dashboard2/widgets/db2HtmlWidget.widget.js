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

    app.directive('db2HtmlWidget', ['$sce', function($sce) {
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

                ctrl.safeHtml = '';

                function sanitize(raw) {
                    if (!raw) { return ''; }
                    if (typeof DOMPurify !== 'undefined') {
                        return $sce.trustAsHtml(DOMPurify.sanitize(raw));
                    }
                    // Fallback: escape all HTML tags when DOMPurify is unavailable
                    var tmp = document.createElement('div');
                    tmp.textContent = raw;
                    return $sce.trustAsHtml(tmp.innerHTML);
                }

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config && ctrl.widgetDef.config.html;
                    },
                    function(raw) {
                        ctrl.safeHtml = sanitize(raw || '');
                    }
                );

                // Initial render
                var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                ctrl.safeHtml = sanitize(cfg.html || '');
            }]
        };
    }]);
});
