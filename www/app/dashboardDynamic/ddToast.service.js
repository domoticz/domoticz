define([
    'app',
    'dashboardDynamic/dashboardDynamic.module'
], function(app) {
    'use strict';

    app.factory('ddToast', ['$timeout', function($timeout) {
        var toasts = [];

        function show(message, type, duration) {
            var toast = {
                id:      Date.now(),
                message: message,
                type:    type || 'info',
                visible: false
            };
            toasts.push(toast);

            // Trigger visible on next tick so the CSS transition fires
            $timeout(function() { toast.visible = true; }, 16);

            $timeout(function() {
                toast.visible = false;
                $timeout(function() {
                    var idx = toasts.indexOf(toast);
                    if (idx !== -1) { toasts.splice(idx, 1); }
                }, 400);
            }, duration || 3000);

            return toast;
        }

        var service = {
            toasts:  toasts,
            show:    show,
            success: function(msg) { return show(msg, 'success'); },
            error:   function(msg) { return show(msg, 'danger',  5000); },
            info:    function(msg) { return show(msg, 'info'); },
            warn:    function(msg) { return show(msg, 'warning', 4000); }
        };

        return service;
    }]);
});
