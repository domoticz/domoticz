define([
    'app',
    'dashboardDynamic/dashboardDynamic.module'
], function(app) {
    'use strict';

    app.factory('ddVisibility', ['$rootScope', function($rootScope) {
        var hidden = null;
        var visibilityChange = null;

        if (typeof document.hidden !== 'undefined') {
            hidden = 'hidden';
            visibilityChange = 'visibilitychange';
        } else if (typeof document.msHidden !== 'undefined') {
            hidden = 'msHidden';
            visibilityChange = 'msvisibilitychange';
        } else if (typeof document.webkitHidden !== 'undefined') {
            hidden = 'webkitHidden';
            visibilityChange = 'webkitvisibilitychange';
        }

        if (hidden !== null) {
            var handler = function() {
                $rootScope.$broadcast(document[hidden] ? 'dd:page:hidden' : 'dd:page:visible');
            };
            document.addEventListener(visibilityChange, handler);
            $rootScope.$on('$destroy', function() {
                document.removeEventListener(visibilityChange, handler);
            });
        }

        return {
            isHidden: function() {
                if (hidden === null) {
                    return false;
                }
                return !!document[hidden];
            }
        };
    }]);
});
