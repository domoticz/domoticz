define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'clock',
        label:       'Clock',
        description: 'Digital clock with date display',
        category:    'Custom Content',
        icon:        'fa-solid fa-clock',
        defaultW:    2,
        defaultH:    2,
        minW:        2,
        minH:        1,
        maxW:        6,
        maxH:        4,
        configSchema: [
            { key: 'showSeconds',     type: 'boolean', label: 'Show seconds',          default: true },
            { key: 'format24h',       type: 'boolean', label: '24-hour format',         default: true },
            { key: 'showDate',        type: 'boolean', label: 'Show date',              default: true },
            { key: 'showBackground',  type: 'boolean', label: 'Show panel background',  default: true }
        ]
    });

    app.directive('ddClockWidget', ['$interval', function($interval) {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/clock.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$interval', function($scope, $interval) {
                var ctrl = this;
                ctrl.timeStr = '';
                ctrl.dateStr = '';

                function pad(n) { return n < 10 ? '0' + n : '' + n; }

                function tick() {
                    var cfg     = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var now     = new Date();
                    var h       = now.getHours();
                    var m       = now.getMinutes();
                    var s       = now.getSeconds();
                    var use24   = cfg.format24h !== false;
                    var showSec = cfg.showSeconds !== false;

                    if (!use24) {
                        var ampm = h >= 12 ? 'PM' : 'AM';
                        h = h % 12 || 12;
                        ctrl.timeStr = pad(h) + ':' + pad(m) +
                                       (showSec ? ':' + pad(s) : '') + ' ' + ampm;
                    } else {
                        ctrl.timeStr = pad(h) + ':' + pad(m) +
                                       (showSec ? ':' + pad(s) : '');
                    }

                    if (cfg.showDate !== false) {
                        var days   = ['Sunday','Monday','Tuesday','Wednesday',
                                      'Thursday','Friday','Saturday'];
                        var months = ['Jan','Feb','Mar','Apr','May','Jun',
                                      'Jul','Aug','Sep','Oct','Nov','Dec'];
                        ctrl.dateStr = days[now.getDay()] + ', ' + now.getDate() + ' ' +
                                       months[now.getMonth()] + ' ' + now.getFullYear();
                    } else {
                        ctrl.dateStr = '';
                    }
                }

                var timer = $interval(tick, 1000);

                $scope.$on('$destroy', function() { $interval.cancel(timer); });

                ctrl.$onInit = tick;
            }]
        };
    }]);
});
