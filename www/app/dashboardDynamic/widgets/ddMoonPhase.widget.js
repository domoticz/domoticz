define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:        'moon-phase',
        label:       'Moon Phase',
        description: 'Displays current moon phase with phase name, illumination %, and next full/new moon date',
        category:    'Information',
        icon:        'fa-solid fa-moon',
        defaultW:    2,
        defaultH:    3,
        minW:        2,
        minH:        2,
        maxW:        4,
        maxH:        4,
        configSchema: [
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional)',
                required: false,
                default:  'Moon'
            },
            {
                key:     'showPhaseName',
                type:    'boolean',
                label:   'Show phase name',
                default: true
            },
            {
                key:     'showIllumination',
                type:    'boolean',
                label:   'Show illumination %',
                default: true
            },
            {
                key:     'showNextEvent',
                type:    'boolean',
                label:   'Show next full/new moon',
                default: true
            }
        ]
    });

    // ── Moon phase calculation helpers ────────────────────────

    var KNOWN_NEW_MOON  = new Date(2000, 0, 6).getTime();  // 2000-01-06 known new moon
    var LUNAR_CYCLE     = 29.530588853;                    // days

    function getMoonPhase(date) {
        var elapsed = (date.getTime() - KNOWN_NEW_MOON) / 86400000;
        return ((elapsed % LUNAR_CYCLE) + LUNAR_CYCLE) % LUNAR_CYCLE;
    }

    function phaseInfo(phase) {
        if (phase < 1.85)  { return { name: 'New Moon',        emoji: '\uD83C\uDF11' }; }
        if (phase < 7.38)  { return { name: 'Waxing Crescent', emoji: '\uD83C\uDF12' }; }
        if (phase < 9.22)  { return { name: 'First Quarter',   emoji: '\uD83C\uDF13' }; }
        if (phase < 14.76) { return { name: 'Waxing Gibbous',  emoji: '\uD83C\uDF14' }; }
        if (phase < 16.61) { return { name: 'Full Moon',       emoji: '\uD83C\uDF15' }; }
        if (phase < 22.15) { return { name: 'Waning Gibbous',  emoji: '\uD83C\uDF16' }; }
        if (phase < 24.0)  { return { name: 'Last Quarter',    emoji: '\uD83C\uDF17' }; }
        return                { name: 'Waning Crescent',       emoji: '\uD83C\uDF18' };
    }

    function illumination(phase) {
        return Math.round((1 - Math.abs(phase - 14.765) / 14.765) * 100);
    }

    // Advance from today in 0.5-day steps until phase crosses the target
    function daysUntilPhase(today, targetPhase, tolerance) {
        var step = 0.5;
        for (var days = 1; days <= Math.ceil(LUNAR_CYCLE) + 2; days++) {
            var future = new Date(today.getTime() + days * 86400000 * step);
            var p = getMoonPhase(future);
            if (Math.abs(p - targetPhase) <= tolerance) {
                return days * step;
            }
        }
        // Fallback: return full cycle
        return LUNAR_CYCLE;
    }

    function formatDate(date) {
        var months = ['Jan','Feb','Mar','Apr','May','Jun',
                      'Jul','Aug','Sep','Oct','Nov','Dec'];
        return months[date.getMonth()] + ' ' + date.getDate();
    }

    function nextEventDate(today, targetPhase, tolerance) {
        var step = 0.5;
        for (var days = 1; days <= (LUNAR_CYCLE / step) + 4; days++) {
            var future = new Date(today.getTime() + days * 86400000 * step);
            var p = getMoonPhase(future);
            if (Math.abs(p - targetPhase) <= tolerance) {
                return future;
            }
        }
        // Fallback: one full cycle from now
        return new Date(today.getTime() + LUNAR_CYCLE * 86400000);
    }

    // ── Directive ─────────────────────────────────────────────

    app.directive('ddMoonPhaseWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/moon-phase.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$interval', function($scope, $interval) {
                var ctrl  = this;
                var timer = null;

                ctrl.title          = '';
                ctrl.emoji          = '';
                ctrl.phaseName      = '';
                ctrl.illumination   = 0;
                ctrl.showPhaseName  = true;
                ctrl.showIllumination = true;
                ctrl.showNextEvent  = true;
                ctrl.fullMoonDate   = '';
                ctrl.newMoonDate    = '';
                ctrl.daysToFull     = 0;
                ctrl.daysToNew      = 0;

                ctrl.update = function() {
                    var cfg  = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var now  = new Date();

                    ctrl.title            = (cfg.title !== undefined ? cfg.title : 'Moon');
                    ctrl.showPhaseName    = cfg.showPhaseName    !== false;
                    ctrl.showIllumination = cfg.showIllumination !== false;
                    ctrl.showNextEvent    = cfg.showNextEvent    !== false;

                    var phase  = getMoonPhase(now);
                    var info   = phaseInfo(phase);
                    ctrl.emoji     = info.emoji;
                    ctrl.phaseName = info.name;
                    ctrl.illumination = illumination(phase);

                    // Next full moon (~14.765 days into cycle, tolerance ±0.75 days)
                    var fullDate      = nextEventDate(now, 14.765, 0.75);
                    ctrl.fullMoonDate = formatDate(fullDate);
                    ctrl.daysToFull   = (fullDate.getTime() - now.getTime()) / 86400000;

                    // Next new moon (~0 or ~29.53 days into cycle, tolerance ±0.75 days)
                    // Search for phase < 0.75 or phase > 28.78
                    var step = 0.5;
                    var newDate = null;
                    for (var d = 1; d <= (LUNAR_CYCLE / step) + 4; d++) {
                        var future = new Date(now.getTime() + d * 86400000 * step);
                        var p = getMoonPhase(future);
                        if (p <= 0.75 || p >= (LUNAR_CYCLE - 0.75)) {
                            newDate = future;
                            break;
                        }
                    }
                    if (!newDate) {
                        newDate = new Date(now.getTime() + LUNAR_CYCLE * 86400000);
                    }
                    ctrl.newMoonDate = formatDate(newDate);
                    ctrl.daysToNew   = (newDate.getTime() - now.getTime()) / 86400000;
                };

                // Refresh daily (86400000 ms = 24 h)
                timer = $interval(ctrl.update, 86400000);

                $scope.$on('$destroy', function() {
                    $interval.cancel(timer);
                });

                $scope.$watch(
                    function() {
                        return ctrl.widgetDef && ctrl.widgetDef.config;
                    },
                    function(val, old) {
                        if (val !== old) { ctrl.update(); }
                    },
                    true
                );

                ctrl.$onInit = ctrl.update;
            }]
        };
    }]);
});
