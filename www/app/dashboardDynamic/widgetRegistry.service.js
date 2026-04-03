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
        categoryOrder: ['Devices', 'Energy', 'Controls', 'Charts & Data', 'Weather', 'Information', 'Custom Content', 'System'],

        getGrouped: function() {
            var raw = registry.getAll().reduce(function(groups, w) {
                var cat = w.category || 'Other';
                if (!groups[cat]) { groups[cat] = []; }
                groups[cat].push(w);
                return groups;
            }, {});

            // Sort items within each category alphabetically by label
            Object.keys(raw).forEach(function(cat) {
                raw[cat].sort(function(a, b) {
                    return (a.label || '').localeCompare(b.label || '');
                });
            });

            // Return as ordered array for predictable rendering
            var result = [];
            registry.categoryOrder.forEach(function(cat) {
                if (raw[cat]) { result.push({ category: cat, items: raw[cat] }); }
            });
            // Append any categories not in the order list
            Object.keys(raw).forEach(function(cat) {
                if (registry.categoryOrder.indexOf(cat) === -1) {
                    result.push({ category: cat, items: raw[cat] });
                }
            });
            return result;
        }
    };

    // Register as an Angular service (injected as 'widgetRegistry' in controllers/directives)
    app.factory('widgetRegistry', function() { return registry; });

    // Also return the registry object directly from the RequireJS module
    // so widget define() factories can call registry.register() synchronously
    return registry;
});
