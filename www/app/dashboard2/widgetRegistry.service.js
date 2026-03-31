define(['app'], function(app) {
    'use strict';

    var _widgets = {};

    var registry = {
        register: function(descriptor) {
            if (!descriptor || !descriptor.type) { return; }
            _widgets[descriptor.type] = descriptor;
        },
        get: function(type) {
            return _widgets[type] || null;
        },
        getAll: function() {
            return Object.keys(_widgets).map(function(k) { return _widgets[k]; });
        },
        getGrouped: function() {
            return registry.getAll().reduce(function(groups, w) {
                var cat = w.category || 'Other';
                if (!groups[cat]) { groups[cat] = []; }
                groups[cat].push(w);
                return groups;
            }, {});
        }
    };

    // Register as an Angular service (injected as 'widgetRegistry' in controllers/directives)
    app.factory('widgetRegistry', function() { return registry; });

    // Also return the registry object directly from the RequireJS module
    // so widget define() factories can call registry.register() synchronously
    return registry;
});
