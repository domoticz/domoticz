define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    var _localTz = Intl.DateTimeFormat().resolvedOptions().timeZone;
    var _tzList  = (typeof Intl.supportedValuesOf === 'function')
        ? Intl.supportedValuesOf('timeZone')
        : [_localTz];

    function _tzOffset(tz) {
        try {
            var parts = new Intl.DateTimeFormat('en', {
                timeZone: tz, timeZoneName: 'longOffset'
            }).formatToParts(new Date());
            var p = parts.find(function(x) { return x.type === 'timeZoneName'; });
            return p ? (p.value.replace('GMT', '') || '+00:00') : '';
        } catch(e) { return ''; }
    }

    // Build GMT/UTC prefix entries
    function _hh(n) { return (n < 10 ? '0' : '') + n; }
    var _gmtOptions = [{ value: 'UTC', label: 'UTC (+00:00)' }];
    for (var _i = 1; _i <= 14; _i++) {
        _gmtOptions.push({ value: 'Etc/GMT-' + _i, label: 'GMT+' + _hh(_i) + ':00 (+' + _hh(_i) + ':00)' });
    }
    for (var _i = 1; _i <= 12; _i++) {
        _gmtOptions.push({ value: 'Etc/GMT+' + _i, label: 'GMT-' + _hh(_i) + ':00 (-' + _hh(_i) + ':00)' });
    }
    var _gmtValues = _gmtOptions.map(function(z) { return z.value; });

    var _tzOptions = _gmtOptions.concat(
        _tzList
            .filter(function(tz) { return _gmtValues.indexOf(tz) === -1; })
            .map(function(tz) {
                var offset = _tzOffset(tz);
                return { value: tz, label: tz + (offset ? ' (' + offset + ')' : '') };
            })
    );

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
            { key: 'title',           type: 'text',    label: 'Title (optional)',                    required: false },
            { key: 'showSeconds',     type: 'boolean', label: 'Show seconds',                        default: true },
            { key: 'format24h',       type: 'boolean', label: '24-hour format',                      default: true },
            { key: 'showDate',        type: 'boolean', label: 'Show date',                           default: true },
            { key: 'showBackground',  type: 'boolean', label: 'Show panel background',               default: true },
            { key: 'timezone',        type: 'select',  label: 'Timezone', default: _localTz, options: _tzOptions }
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
                ctrl.title   = '';

                function pad(n) { return n < 10 ? '0' + n : '' + n; }

                function tick() {
                    var cfg     = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    ctrl.title  = cfg.title || '';
                    var use24   = cfg.format24h !== false;
                    var showSec = cfg.showSeconds !== false;
                    var tz      = (cfg.timezone || '').trim();
                    var now     = new Date();
                    var d;

                    if (tz) {
                        try {
                            d = new Date(now.toLocaleString('en-US', { timeZone: tz }));
                        } catch(e) {
                            d = now;
                        }
                    } else {
                        d = now;
                    }

                    var h = d.getHours();
                    var m = d.getMinutes();
                    var s = d.getSeconds();

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
                        ctrl.dateStr = days[d.getDay()] + ', ' + d.getDate() + ' ' +
                                       months[d.getMonth()] + ' ' + d.getFullYear();
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
