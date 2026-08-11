define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    // ── Arc geometry ───────────────────────────────────────────────────────
    // viewBox -6 -6 112 112, centre (50,50).
    // Arc starts at 135° (7-o'clock), sweeps 270° clockwise to 45° (5-o'clock).
    var CX        = 50;
    var CY        = 50;
    var ARC_R     = 37;    // amber ring radius & tip position
    var ARC_START = 135;   // degrees from +x axis (SVG clockwise)
    var ARC_SPAN  = 270;   // total degrees

    // Inner hub geometry — needle is a triangle from NEEDLE_BASE_R to ARC_R;
    // the hub circle (r=INNER_R) is drawn on top, masking the needle base so
    // the visible needle runs from INNER_R to ARC_R only.
    var NEEDLE_BASE_R = 29; // triangle base depth (inside hub)
    var INNER_R       = 30; // inner hub circle radius
    var BEZEL_R       = 49; // outer bezel ring radius (edge of the dial)

    // Arc-fill geometry (for temp / kWh / numeric / P1 modes).
    // ARC_R_FILL sits between hub (INNER_R=30) and ring (ARC_R=37); stroke-width=4
    // so inner edge is at ~32 (outside hub) and outer edge at ~36 (inside ring).
    var ARC_R_FILL    = 34;
    var CIRC_FILL     = 2 * Math.PI * ARC_R_FILL;      // ~213.63
    var ARC_LEN_FILL  = CIRC_FILL * ARC_SPAN / 360;    // ~160.22 for 270°

    var _verifiedDevices = {};

    // ── Widget registration ────────────────────────────────────────────────
    widgetRegistry.register({
        type:                  'dial',
        label:                 'Dial',
        description:           'Circular dial — drag to set setpoints/selectors, arc-fill for sensors, compass for wind, bidirectional for P1',
        category:              'Controls',
        icon:                  'fa-solid fa-gauge',
        transparentBackground: true,
        defaultW:              2,
        defaultH:              3,
        minW:                  1,
        minH:                  2,
        maxW:                  4,
        maxH:                  5,
        configSchema: [
            {
                key:      'deviceIdx',
                type:     'device-picker',
                label:    'Device',
                required: true
            },
            {
                key:      'title',
                type:     'text',
                label:    'Title (optional, falls back to device name)',
                required: false
            },
            {
                type:          'group',
                spread:        true,
                hideForSwitch: true,
                fields: [
                    {
                        key:     'minVal',
                        type:    'number',
                        label:   'Min',
                        default: '',
                        help:    'Scale min (leave empty to use device range).'
                    },
                    {
                        key:     'maxVal',
                        type:    'number',
                        label:   'Max',
                        default: '',
                        help:    'Scale max (leave empty to use device range).'
                    }
                ]
            },
            {
                key:           'ranges',
                type:          'range-list',
                label:         'Value ranges',
                help:          'Map value intervals to colors: normal (green), warning (amber), critical (red). Ranges are checked in order; the first match wins.',
                hideForSwitch: true
            },
            { key: 'showMinorTicks', type: 'boolean', label: 'Show intermediate tick marks', default: true, hideForSwitch: true },
            { key: 'unit',          type: 'text',    label: 'Unit override (blank = auto from device)', required: false, hideForSwitch: true }
        ]
    });

    // ── Scale helpers ──────────────────────────────────────────────────────

    function niceStep(range) {
        if (range <= 0) { return 1; }
        var rough = range / 6;
        var mag   = Math.pow(10, Math.floor(Math.log(rough) / Math.LN10));
        var norm  = rough / mag;
        var step;
        if      (norm < 1.5) { step = 1; }
        else if (norm < 3.5) { step = 2; }
        else if (norm < 7.5) { step = 5; }
        else                 { step = 10; }
        return step * mag;
    }

    // How many decimals a label needs follows from the tick step, not from the
    // device: a 0.02 step needs two, a 20 step needs none.
    function stepDecimals(step) {
        if (!step || step <= 0 || !isFinite(step)) { return 1; }
        return Math.max(0, Math.min(3, -Math.floor(Math.log(step) / Math.LN10)));
    }

    function fmtLabel(v, step) {
        if (Math.abs(v) >= 10000) { return Math.round(v / 1000) + 'k'; }
        if (Math.abs(v) >= 1000)  { return (v / 1000).toFixed(1).replace('.0', '') + 'k'; }
        var s = v.toFixed(stepDecimals(step));
        if (s.indexOf('.') !== -1) { s = s.replace(/0+$/, '').replace(/\.$/, ''); }
        return s === '-0' ? '0' : s;
    }

    // ── Minor tick helper ──────────────────────────────────────────────────
    var TICK_R = 43.5;
    function makeMinorTick(angleDeg) {
        var rad = angleDeg * Math.PI / 180;
        var cos = Math.cos(rad), sin = Math.sin(rad);
        return {
            x1: (CX + 42.0 * cos).toFixed(2), y1: (CY + 42.0 * sin).toFixed(2),
            x2: (CX + TICK_R * cos).toFixed(2), y2: (CY + TICK_R * sin).toFixed(2),
            label: '', textX: '0', textY: '0', textRot: '0', minor: true
        };
    }

    // Numeric scale ticks (min→max spread around the 270° arc).
    // textRot = angleDeg + 90 so the TOP of each label points outward from centre.
    function buildScale(min, max, showMinor) {
        var range = max - min;
        if (range <= 0) { return []; }
        var step = niceStep(range);

        var seen = {}, vals = [];
        function addVal(v) {
            var key = String(Math.round(v * 1e6));
            if (!seen[key]) { seen[key] = true; vals.push(v); }
        }
        addVal(min);
        var first = Math.ceil((min + 1e-9) / step) * step;
        for (var v = first; v < max - 1e-9; v += step) { addVal(v); }
        addVal(max);
        vals.sort(function(a, b) { return a - b; });

        var TICK_LEN = 2.0;
        var LBL_R    = 44.5;
        var ticks    = [];

        for (var i = 0; i < vals.length; i++) {
            var sv       = vals[i];
            var p        = (sv - min) / range;
            var angleDeg = ARC_START + p * ARC_SPAN;
            var rad      = angleDeg * Math.PI / 180;
            var cos      = Math.cos(rad);
            var sin      = Math.sin(rad);
            ticks.push({
                x1:      (CX + (TICK_R - TICK_LEN) * cos).toFixed(2),
                y1:      (CY + (TICK_R - TICK_LEN) * sin).toFixed(2),
                x2:      (CX + TICK_R * cos).toFixed(2),
                y2:      (CY + TICK_R * sin).toFixed(2),
                label:   fmtLabel(sv, step),
                textX:   (CX + LBL_R * cos).toFixed(2),
                textY:   (CY + LBL_R * sin).toFixed(2),
                textRot: (angleDeg + 90).toFixed(1)
            });
        }

        // 4 minor ticks between each pair of major ticks (5 equal sub-divisions)
        if (showMinor) {
            var N = 5;
            for (var j = 0; j < vals.length - 1; j++) {
                for (var m = 1; m < N; m++) {
                    var vm  = vals[j] + (vals[j + 1] - vals[j]) * m / N;
                    var pm  = (vm - min) / range;
                    ticks.push(makeMinorTick(ARC_START + pm * ARC_SPAN));
                }
            }
        }
        return ticks;
    }

    // Selector: one tick mark per level, no labels (level names shown inside face).
    function buildSelectorTicks(count) {
        if (count < 2) { return []; }
        var ticks = [];
        for (var i = 0; i < count; i++) {
            var p   = i / (count - 1);
            var rad = (ARC_START + p * ARC_SPAN) * Math.PI / 180;
            ticks.push({
                x1:      (CX + 38.5 * Math.cos(rad)).toFixed(2),
                y1:      (CY + 38.5 * Math.sin(rad)).toFixed(2),
                x2:      (CX + 40.5 * Math.cos(rad)).toFixed(2),
                y2:      (CY + 40.5 * Math.sin(rad)).toFixed(2),
                label: '', textX: '0', textY: '0', textRot: '0'
            });
        }
        return ticks;
    }

    // Compass ticks: degree values (0/45/90…315) mapped to SVG angles.
    function buildCompassTicks(showMinor) {
        var TICK_LEN = 2.0;
        var LBL_R    = 44.5;
        var ticks    = [];
        for (var i = 0; i < 8; i++) {
            var compassDeg = i * 45;
            var svgAngle   = (compassDeg - 90 + 360) % 360;
            var rad        = svgAngle * Math.PI / 180;
            var cos        = Math.cos(rad);
            var sin        = Math.sin(rad);
            ticks.push({
                x1:      (CX + (TICK_R - TICK_LEN) * cos).toFixed(2),
                y1:      (CY + (TICK_R - TICK_LEN) * sin).toFixed(2),
                x2:      (CX + TICK_R * cos).toFixed(2),
                y2:      (CY + TICK_R * sin).toFixed(2),
                label:   String(compassDeg),
                textX:   (CX + LBL_R * cos).toFixed(2),
                textY:   (CY + LBL_R * sin).toFixed(2),
                textRot: (svgAngle + 90).toFixed(1)
            });
            // 2 minor ticks between each 45° major (at +15° and +30°)
            if (showMinor) {
                ticks.push(makeMinorTick((svgAngle + 15 + 360) % 360));
                ticks.push(makeMinorTick((svgAngle + 30 + 360) % 360));
            }
        }
        return ticks;
    }

    // SVG arc path helper (used for P1 bidirectional fill)
    function svgArcPath(r, startAngleDeg, endAngleDeg, sweepFlag) {
        var sa  = startAngleDeg * Math.PI / 180;
        var ea  = endAngleDeg   * Math.PI / 180;
        var x1  = CX + r * Math.cos(sa);
        var y1  = CY + r * Math.sin(sa);
        var x2  = CX + r * Math.cos(ea);
        var y2  = CY + r * Math.sin(ea);
        var spanDeg = sweepFlag
            ? ((endAngleDeg - startAngleDeg) + 360) % 360
            : ((startAngleDeg - endAngleDeg) + 360) % 360;
        var largeArc = spanDeg > 180 ? 1 : 0;
        return 'M ' + x1.toFixed(2) + ',' + y1.toFixed(2) +
               ' A ' + r + ',' + r + ' 0 ' + largeArc + ',' + sweepFlag +
               ' ' + x2.toFixed(2) + ',' + y2.toFixed(2);
    }

    // ── Directive ──────────────────────────────────────────────────────────
    app.directive('ddDialWidget', ['bootbox', '$document',
        function(bootbox, $document) { // eslint-disable-line no-unused-vars

        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/dial.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$q', '$rootScope',
                function($scope, $http, $q, $rootScope) {

                var ctrl = this;

                // ── Safe defaults ────────────────────────────────────────
                ctrl.title           = '';
                ctrl.deviceName      = '';
                ctrl.deviceType      = 'numeric';
                ctrl.value           = null;
                ctrl.textValue       = '';
                ctrl.valueStr        = '--';
                ctrl.unitStr         = '';
                ctrl.effectiveMin    = 0;
                ctrl.effectiveMax    = 100;
                ctrl.deviceStep      = 0.5;
                ctrl.deviceProtected = false;
                ctrl.levelOptions    = [];
                ctrl.levelInt        = 0;
                ctrl.selectorActiveName = '';
                ctrl.selectorItems      = [];
                ctrl.sending         = false;
                ctrl.loadError       = false;
                ctrl.timedOut        = false;
                ctrl.dragging        = false;
                ctrl.scaleTicks      = [];
                // Wind extras
                ctrl.windSpeed  = null;
                ctrl.windGust   = null;
                ctrl.windTemp   = null;
                ctrl.windChill  = null;
                // Temp extras
                ctrl.humidity    = null;
                ctrl.baro        = null;
                ctrl.forecastStr = '';
                ctrl.lastUpdate = '';
                // kWh extras
                ctrl.kwhToday    = null;
                ctrl.kwhTodayStr = '--';
                ctrl.powerStr    = '--';
                // P1 extras
                ctrl.p1Power    = null;
                ctrl.importKwh  = null;
                ctrl.exportKwh  = null;
                // Switch extras
                ctrl.switchOn   = false;
                ctrl.switchType = '';

                var cancelToken = null;
                var _svgEl      = null;

                function cfg() { return (ctrl.widgetDef && ctrl.widgetDef.config) || {}; }

                // ── Needle angle ──────────────────────────────────────────
                // Wind: compass heading → SVG angle (N=270°, E=0°, S=90°, W=180°)
                // Others: arc position based on value percentage
                function needleAngleDeg() {
                    if (ctrl.deviceType === 'wind') {
                        if (ctrl.value === null) { return 270; }
                        return ((ctrl.value - 90) % 360 + 360) % 360;
                    }
                    return ARC_START + pct() * ARC_SPAN;
                }

                // ── Percent helpers ───────────────────────────────────────
                function pct() {
                    if (ctrl.deviceType === 'selector') {
                        if (!ctrl.levelOptions.length) { return 0; }
                        for (var i = 0; i < ctrl.levelOptions.length; i++) {
                            if (ctrl.levelOptions[i].level === ctrl.levelInt) {
                                return ctrl.levelOptions.length > 1
                                    ? i / (ctrl.levelOptions.length - 1) : 0;
                            }
                        }
                        return 0;
                    }
                    if (ctrl.value === null) { return 0; }
                    var range = ctrl.effectiveMax - ctrl.effectiveMin;
                    if (range === 0) { return 0; }
                    return Math.min(1, Math.max(0, (ctrl.value - ctrl.effectiveMin) / range));
                }

                // ── Tip dot ───────────────────────────────────────────────
                ctrl.tipX = function() {
                    var rad = needleAngleDeg() * Math.PI / 180;
                    return (CX + ARC_R * Math.cos(rad)).toFixed(2);
                };
                ctrl.tipY = function() {
                    var rad = needleAngleDeg() * Math.PI / 180;
                    return (CY + ARC_R * Math.sin(rad)).toFixed(2);
                };
                ctrl.showTip = function() {
                    return ctrl.deviceType === 'selector';
                };

                // ── Triangular needle polygon ─────────────────────────────
                // Triangle from NEEDLE_BASE_R toward tip at ARC_R;
                // the inner hub (r=INNER_R) is drawn on top, masking the base,
                // so only the portion from INNER_R to ARC_R is visible.
                ctrl.needlePoints = function() {
                    var angleDeg = needleAngleDeg();
                    var angleRad = angleDeg * Math.PI / 180;
                    var perpRad  = (angleDeg + 90) * Math.PI / 180;
                    var hw       = 2.0; // half-width at base

                    var tipX = CX + ARC_R      * Math.cos(angleRad);
                    var tipY = CY + ARC_R      * Math.sin(angleRad);
                    var lx   = CX + NEEDLE_BASE_R * Math.cos(angleRad) + hw * Math.cos(perpRad);
                    var ly   = CY + NEEDLE_BASE_R * Math.sin(angleRad) + hw * Math.sin(perpRad);
                    var rx   = CX + NEEDLE_BASE_R * Math.cos(angleRad) - hw * Math.cos(perpRad);
                    var ry   = CY + NEEDLE_BASE_R * Math.sin(angleRad) - hw * Math.sin(perpRad);

                    return tipX.toFixed(2) + ',' + tipY.toFixed(2) + ' ' +
                           lx.toFixed(2)   + ',' + ly.toFixed(2)   + ' ' +
                           rx.toFixed(2)   + ',' + ry.toFixed(2);
                };

                ctrl.dialContentScale = function() {
                    return (ctrl.deviceType === 'switch' || ctrl.deviceType === 'selector') ? 1.2 : 1;
                };

                ctrl.showNeedle = function() {
                    return ctrl.value !== null &&
                           ctrl.deviceType !== 'selector' &&
                           ctrl.deviceType !== 'text'   &&
                           ctrl.deviceType !== 'switch';
                };

                // ── Arc fill (for temp / kWh / numeric modes) ─────────────
                ctrl.showArcFill = function() {
                    return ctrl.deviceType === 'temp'    ||
                           ctrl.deviceType === 'kwh'     ||
                           ctrl.deviceType === 'p1'      ||
                           ctrl.deviceType === 'numeric';
                };
                ctrl.arcTrackDasharray = function() {
                    return ARC_LEN_FILL.toFixed(2) + ' ' + CIRC_FILL.toFixed(2);
                };
                ctrl.arcFillDasharray = function() {
                    return (pct() * ARC_LEN_FILL).toFixed(2) + ' ' + CIRC_FILL.toFixed(2);
                };

                // ── P1 bidirectional arc path ─────────────────────────────
                // 0W is at the top (SVG 270° = 12-o'clock = arc midpoint).
                // Export fills clockwise; import fills counterclockwise.
                ctrl.p1FillPath = function() {
                    if (ctrl.deviceType !== 'p1' || ctrl.value === null) { return ''; }
                    var p = pct();
                    if (Math.abs(p - 0.5) < 0.005) { return ''; }
                    var centerAngle  = ARC_START + ARC_SPAN * 0.5; // 270°
                    var currentAngle = ARC_START + p * ARC_SPAN;
                    return svgArcPath(ARC_R_FILL, centerAngle, currentAngle, p > 0.5 ? 1 : 0);
                };
                ctrl.p1FillColor = function() {
                    if (ctrl.p1Power === null) { return 'none'; }
                    return ctrl.p1Power >= 0
                        ? 'var(--dz-widget-amber)'
                        : 'var(--dz-widget-energy-export)';
                };

                // ── Arc fill colour (range-aware) ────────────────────────
                ctrl.arcFillColor = function() {
                    var type = ctrl.deviceType;
                    if (type === 'switch') {
                        return ctrl.switchOn
                            ? 'var(--dz-accent-color)'
                            : 'var(--dz-widget-stat-muted)';
                    }
                    if (type === 'selector' || type === 'text' || type === 'wind') {
                        return 'var(--dz-widget-amber)';
                    }
                    var v = ctrl.value;
                    if (v === null) { return 'var(--dz-widget-amber)'; }
                    var c = cfg();

                    // Range-based coloring: narrowest matching range wins
                    var ranges = c.ranges;
                    if (Array.isArray(ranges) && ranges.length > 0) {
                        var matched = null, matchedWidth = Infinity;
                        for (var i = 0; i < ranges.length; i++) {
                            var r    = ranges[i];
                            var from = parseFloat(r.from);
                            var to   = parseFloat(r.to);
                            if (!isNaN(from) && !isNaN(to) && v >= from && v <= to) {
                                var width = Math.abs(to - from);
                                if (width < matchedWidth) { matched = r; matchedWidth = width; }
                            }
                        }
                        if (matched) {
                            // Prefer explicit color (consistent with kwh-summary, gauge, etc.)
                            if (matched.color) { return matched.color; }
                            // Legacy status-based mapping (backwards compat)
                            if (matched.status === 'critical') { return 'var(--dz-accent-red)'; }
                            if (matched.status === 'warning')  { return 'var(--dz-widget-amber)'; }
                            return 'var(--dz-widget-energy-export)';
                        }
                        return 'var(--dz-widget-amber)';
                    }

                    // Legacy threshold coloring (backward compat)
                    var warn = parseFloat(c.thresholdWarn);
                    var crit = parseFloat(c.thresholdCrit);
                    if (isNaN(warn) || isNaN(crit)) { return 'var(--dz-widget-amber)'; }
                    var mode = c.thresholdMode || 'low-is-good';
                    if (mode === 'high-is-good') {
                        if (v >= crit) { return 'var(--dz-widget-energy-export)'; }
                        if (v >= warn) { return 'var(--dz-widget-amber)'; }
                        return 'var(--dz-accent-red)';
                    } else {
                        if (v < warn)  { return 'var(--dz-widget-energy-export)'; }
                        if (v < crit)  { return 'var(--dz-widget-amber)'; }
                        return 'var(--dz-accent-red)';
                    }
                };

                // ── Last update string ────────────────────────────────────
                ctrl.lastUpdateStr = function() {
                    var s = ctrl.lastUpdate;
                    if (!s) { return ''; }
                    var m = s.match(/(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2})/);
                    if (!m) { return s.length > 10 ? s.substring(0, 10) : s; }
                    return m[4] + ':' + m[5] + ' ' + m[3] + '/' + m[2];
                };

                // ── Wind sub-line helpers ─────────────────────────────────
                ctrl.windSubLine = function() {
                    if (ctrl.windSpeed === null) { return ''; }
                    var wu   = ($rootScope.config && $rootScope.config.WindSign) || 'm/s';
                    var line = ctrl.windSpeed + ' ' + wu;
                    if (ctrl.windGust !== null) { line += '  gust ' + ctrl.windGust + ' ' + wu; }
                    return line;
                };
                ctrl.windTempLine = function() {
                    var tu    = ($rootScope.config && $rootScope.config.TempSign) || 'C';
                    var parts = [];
                    if (ctrl.windTemp  !== null) { parts.push(ctrl.windTemp  + '\u00b0' + tu); }
                    if (ctrl.windChill !== null) { parts.push('chill ' + ctrl.windChill + '\u00b0' + tu); }
                    return parts.join('  ');
                };

                // ── Scale rebuild ─────────────────────────────────────────
                function rebuildScale() {
                    var showMinor = (cfg().showMinorTicks !== false);
                    if (ctrl.deviceType === 'selector') {
                        ctrl.scaleTicks = buildSelectorTicks(ctrl.levelOptions.length);
                    } else if (ctrl.deviceType === 'wind') {
                        ctrl.scaleTicks = buildCompassTicks(showMinor);
                    } else if (ctrl.deviceType !== 'text') {
                        ctrl.scaleTicks = buildScale(ctrl.effectiveMin, ctrl.effectiveMax, showMinor);
                    } else {
                        ctrl.scaleTicks = [];
                    }
                }

                // ── Device data ───────────────────────────────────────────

                function formatNum(v) {
                    if (v === null || v === undefined) { return '--'; }
                    var n = parseFloat(v);
                    if (isNaN(n)) { return '--'; }
                    if (Math.abs(n) >= 10000) { return Math.round(n / 1000) + 'k'; }
                    if (Math.abs(n) >= 1000)  { return String(Math.round(n)); }
                    return String(n);
                }

                function parseFirst(s) {
                    if (!s) { return null; }
                    var m = String(s).match(/^([-\d.]+)/);
                    return m ? parseFloat(m[1]) : null;
                }

                function decodeLevelNames(b64) {
                    try { return atob(b64).split('|'); } catch (e) { return []; }
                }

                // Apply config min/max for non-setpoint devices.
                function applyConfigRange(deviceMin, deviceMax) {
                    var c      = cfg();
                    var rawMin = c.minVal;
                    var rawMax = c.maxVal;
                    var cfgMin = (rawMin !== '' && rawMin !== null && rawMin !== undefined)
                                    ? parseFloat(rawMin) : NaN;
                    var cfgMax = (rawMax !== '' && rawMax !== null && rawMax !== undefined)
                                    ? parseFloat(rawMax) : NaN;
                    if (!isNaN(cfgMin) && !isNaN(cfgMax) && cfgMin === 0 && cfgMax === 0) {
                        cfgMin = NaN; cfgMax = NaN;
                    }
                    var fallbackMin = (deviceMin !== undefined && !isNaN(deviceMin)) ? deviceMin : 0;
                    var fallbackMax = (deviceMax !== undefined && !isNaN(deviceMax)) ? deviceMax : 100;
                    ctrl.effectiveMin = !isNaN(cfgMin) ? cfgMin : fallbackMin;
                    ctrl.effectiveMax = !isNaN(cfgMax) ? cfgMax : fallbackMax;
                }

                // Update selector display: vertical list centered on active item
                var SEL_SPACING  = 13;   // SVG units between item centres
                var SEL_FONTS    = [2.8, 3.5, 4.5, 3.5, 2.8];  // offsets -2..+2
                var SEL_OPACITY  = [0.35, 0.60, 1.0, 0.60, 0.35];

                function updateSelectorDisplay() {
                    var activeIdx = 0;
                    ctrl.selectorActiveName = '';
                    for (var i = 0; i < ctrl.levelOptions.length; i++) {
                        if (ctrl.levelOptions[i].level === ctrl.levelInt) {
                            activeIdx = i;
                            ctrl.selectorActiveName = ctrl.levelOptions[i].name;
                            break;
                        }
                    }
                    ctrl.selectorItems = [];
                    for (var off = -2; off <= 2; off++) {
                        var idx = activeIdx + off;
                        if (idx < 0 || idx >= ctrl.levelOptions.length) { continue; }
                        var ai  = off + 2; // 0..4
                        ctrl.selectorItems.push({
                            name:    ctrl.levelOptions[idx].name,
                            level:   ctrl.levelOptions[idx].level,
                            y:       (50 + off * SEL_SPACING).toFixed(1),
                            fontSize: SEL_FONTS[ai],
                            opacity:  SEL_OPACITY[ai],
                            isActive: off === 0
                        });
                    }
                }

                function applyDevice(d) {
                    var c = cfg();
                    ctrl.deviceName = d.Name || '';
                    ctrl.title      = c.title || ctrl.deviceName;
                    ctrl.timedOut   = !!d.HaveTimeout;
                    ctrl.lastUpdate = d.LastUpdate || '';

                    // ── Wind ──────────────────────────────────────────────
                    if (d.Type === 'Wind') {
                        ctrl.deviceType   = 'wind';
                        ctrl.value        = (d.Direction !== undefined) ? parseFloat(d.Direction) : null;
                        ctrl.valueStr     = ctrl.value !== null ? ctrl.value.toFixed(0) + '\u00b0' : '--';
                        ctrl.unitStr      = d.DirectionStr || '';
                        ctrl.windSpeed    = (d.Speed !== undefined) ? d.Speed : null;
                        ctrl.windGust     = (d.Gust  !== undefined) ? d.Gust  : null;
                        ctrl.windTemp     = (d.Temp  !== undefined) ? parseFloat(d.Temp)  : null;
                        ctrl.windChill    = (d.Chill !== undefined) ? parseFloat(d.Chill) : null;
                        ctrl.effectiveMin = 0;
                        ctrl.effectiveMax = 360;
                        rebuildScale();
                        return;
                    }

                    // ── Setpoint / Thermostat / Thermostat6 / Evohome ─────
                    if (d.SetPoint !== undefined) {
                        ctrl.deviceType      = 'setpoint';
                        ctrl.deviceProtected = d.Protected || false;
                        ctrl.deviceStep   = (d.step !== undefined) ? (parseFloat(d.step) || 0.5) : 0.5;
                        var spMin = (d.min !== undefined) ? parseFloat(d.min) : 0;
                        var spMax = (d.max !== undefined) ? parseFloat(d.max) : 100;
                        applyConfigRange(spMin, spMax);
                        ctrl.unitStr      = (d.vunit !== undefined && d.vunit !== '') ? d.vunit : '\u00b0C';
                        var sp = parseFloat(d.SetPoint !== undefined ? d.SetPoint : d.Data);
                        ctrl.value    = isNaN(sp) ? null : sp;
                        ctrl.valueStr = ctrl.value !== null ? String(ctrl.value) : '--';
                        rebuildScale();
                        return;
                    }

                    // ── Selector Switch ───────────────────────────────────
                    if (d.SwitchType === 'Selector') {
                        ctrl.deviceType      = 'selector';
                        ctrl.deviceProtected = d.Protected || false;
                        var names = decodeLevelNames(d.LevelNames || '');
                        ctrl.levelOptions = [];
                        for (var ni = 0; ni < names.length; ni++) {
                            if (names[ni] && !(ni === 0 && d.LevelOffHidden)) {
                                ctrl.levelOptions.push({ level: ni * 10, name: names[ni] });
                            }
                        }
                        ctrl.levelInt = parseInt(d.LevelInt, 10) || 0;
                        ctrl.value    = ctrl.levelInt;
                        ctrl.unitStr  = '';
                        updateSelectorDisplay();
                        ctrl.valueStr = ctrl.selectorActiveName;
                        rebuildScale();
                        return;
                    }

                    // ── Switch (On/Off / PushOn / PushOff) ───────────────────────────────
                    if (d.SwitchType && d.SwitchType !== 'Selector' && d.Status !== undefined) {
                        ctrl.deviceType      = 'switch';
                        ctrl.deviceProtected = d.Protected || false;
                        ctrl.switchType  = d.SwitchType || '';
                        if (ctrl.switchType === 'Push On Button') {
                            ctrl.switchOn = true;
                            ctrl.valueStr = $.t('On');
                        } else if (ctrl.switchType === 'Push Off Button') {
                            ctrl.switchOn = true;
                            ctrl.valueStr = $.t('Off');
                        } else {
                            ctrl.switchOn = (d.Status === 'On');
                            ctrl.valueStr = d.Status ? $.t(d.Status) : '--';
                        }
                        ctrl.value       = ctrl.switchOn ? 1 : 0;
                        ctrl.unitStr     = '';
                        ctrl.scaleTicks  = [];
                        return;
                    }

                    // ── Text sensor ───────────────────────────────────────
                    if (d.SubType === 'Text') {
                        ctrl.deviceType = 'text';
                        ctrl.textValue  = d.Data || '';
                        ctrl.value      = null;
                        ctrl.valueStr   = d.Data || '--';
                        ctrl.unitStr    = '';
                        ctrl.scaleTicks = [];
                        return;
                    }

                    // ── P1 Smart Meter (electricity only — Gas handled below) ─
                    if (d.Type === 'P1 Smart Meter' && d.SubType !== 'Gas') {
                        ctrl.deviceType = 'p1';
                        var importW = parseFirst(d.Usage)      || 0;
                        var exportW = parseFirst(d.UsageDeliv) || 0;
                        ctrl.p1Power = importW - exportW;
                        ctrl.value    = ctrl.p1Power;
                        ctrl.valueStr = formatNum(ctrl.p1Power);
                        ctrl.unitStr  = 'W';
                        ctrl.importKwh = parseFirst(d.CounterToday);
                        ctrl.exportKwh = parseFirst(d.CounterDelivToday);
                        ctrl.lastUpdate = d.LastUpdate || '';
                        // Symmetric range auto-selected from current power
                        var absP   = Math.max(Math.abs(ctrl.p1Power), 500);
                        var ranges = [1000, 2000, 5000, 10000, 20000];
                        var autoMax = ranges[ranges.length - 1];
                        for (var ri = 0; ri < ranges.length; ri++) {
                            if (absP <= ranges[ri]) { autoMax = ranges[ri]; break; }
                        }
                        applyConfigRange(-autoMax, autoMax);
                        rebuildScale();
                        return;
                    }

                    // ── Temperature (+ optional humidity / barometer) ─────
                    if (d.Temp !== undefined) {
                        ctrl.deviceType = 'temp';
                        ctrl.value      = parseFloat(d.Temp);
                        if (isNaN(ctrl.value)) { ctrl.value = null; }
                        var tempUnit = ($rootScope.config && $rootScope.config.TempSign) || 'C';
                        ctrl.unitStr  = '\u00b0' + tempUnit;
                        ctrl.valueStr = ctrl.value !== null ? String(ctrl.value) : '--';
                        ctrl.humidity    = (d.Humidity  !== undefined) ? parseInt(d.Humidity, 10)    : null;
                        ctrl.baro        = (d.Barometer !== undefined) ? parseFloat(d.Barometer)     : null;
                        ctrl.forecastStr = d.ForecastStr || '';
                        ctrl.lastUpdate  = d.LastUpdate || '';
                        var defMax = tempUnit === 'F' ? 104 : 40;
                        applyConfigRange(0, defMax);
                        rebuildScale();
                        return;
                    }

                    // ── kWh / Usage / P1 Gas ─────────────────────────────
                    var isP1Gas = d.Type === 'P1 Smart Meter' && d.SubType === 'Gas';
                    if (d.SubType === 'kWh' || d.Type === 'Usage' || isP1Gas) {
                        ctrl.deviceType = 'kwh';
                        var powerVal   = isP1Gas ? null : parseFirst(d.Usage || d.Data);
                        ctrl.powerStr  = powerVal !== null ? formatNum(powerVal) + ' W' : '--';
                        var kwhVal     = parseFirst(d.CounterToday);
                        ctrl.kwhToday  = kwhVal;
                        ctrl.kwhTodayStr = kwhVal !== null ? String(kwhVal) : '--';
                        ctrl.lastUpdate  = d.LastUpdate || '';
                        if (kwhVal !== null) {
                            ctrl.value    = kwhVal;
                            ctrl.valueStr = ctrl.kwhTodayStr;
                            ctrl.unitStr  = isP1Gas ? 'm\u00b3' : 'kWh';
                            applyConfigRange(0, isP1Gas ? 10 : 50);
                        } else {
                            ctrl.value    = powerVal;
                            ctrl.valueStr = powerVal !== null ? formatNum(powerVal) : '--';
                            ctrl.unitStr  = 'W';
                            applyConfigRange(0, 5000);
                        }
                        rebuildScale();
                        return;
                    }

                    // ── Humidity-only sensor (no Temp) ─────────────────────
                    // pTypeHUM devices carry the value in d.Humidity, not d.Data
                    // ("Humidity 50 %"), so the generic numeric branch below would
                    // fail to parse it. Temp+Hum devices are handled earlier.
                    if (d.Humidity !== undefined) {
                        ctrl.deviceType = 'numeric';
                        var hum = parseInt(d.Humidity, 10);
                        ctrl.value    = isNaN(hum) ? null : hum;
                        ctrl.unitStr  = '%';
                        ctrl.valueStr = ctrl.value !== null ? formatNum(ctrl.value) : '--';
                        applyConfigRange(0, 100);
                        rebuildScale();
                        return;
                    }

                    // ── Generic numeric ───────────────────────────────────
                    ctrl.deviceType = 'numeric';
                    var dataRaw = d.Data || '';
                    var nm      = dataRaw.match(/^([-\d.]+)/);
                    ctrl.value    = nm ? parseFloat(nm[1]) : null;
                    // Extract unit after the leading number (space optional: "55.1 %" and "55.1%")
                    var unitN    = dataRaw.match(/^[-\d.]+\s*([^\d\s].*)/);
                    var rawUnit  = unitN ? unitN[1].trim() : (d.Unit || '');
                    // Domoticz stores Unit=1 as a dimensionless placeholder — suppress it;
                    // fall back to SubType for known cases (e.g. Percentage → %)
                    if (/^\d+$/.test(String(rawUnit))) {
                        rawUnit = d.SubType === 'Percentage' ? '%' : '';
                    }
                    ctrl.unitStr = rawUnit;
                    ctrl.valueStr = formatNum(ctrl.value);
                    applyConfigRange(undefined, undefined);
                    rebuildScale();
                }

                // ── Config unit override — runs after every applyDevice call ──
                // Each device-type branch above sets ctrl.unitStr from the device
                // data; this function lets the user override it from config, or
                // leaves it untouched when the config field is blank.
                function applyUnitOverride() {
                    var cfgUnit = String(cfg().unit || '').trim();
                    if (cfgUnit) { ctrl.unitStr = cfgUnit; }
                }

                // ── HTTP load ─────────────────────────────────────────────
                function load() {
                    var c = cfg();
                    if (!c.deviceIdx) { return; }
                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: c.deviceIdx },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        var d = resp.data && resp.data.result && resp.data.result[0];
                        if (!d) { ctrl.loadError = true; return; }
                        ctrl.loadError = false;
                        applyDevice(d);
                        applyUnitOverride();
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                        ctrl.loadError = true;
                    });
                }

                // ── Setpoint send ─────────────────────────────────────────
                function clamp(v) {
                    return Math.max(ctrl.effectiveMin, Math.min(ctrl.effectiveMax, v));
                }

                function roundToStep(v) {
                    var s       = ctrl.deviceStep;
                    var snapped = Math.round(v / s) * s;
                    var dec     = (s.toString().split('.')[1] || '').length;
                    return parseFloat(snapped.toFixed(dec));
                }

                function sendSetpoint(newVal) {
                    ctrl.sending = true;
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'setsetpoint',
                                  idx: cfg().deviceIdx, setpoint: newVal }
                    }).then(function() {
                        ctrl.value    = newVal;
                        ctrl.valueStr = String(newVal);
                        ctrl.sending  = false;
                    }).catch(function() { ctrl.sending = false; });
                }

                // Precise numeric entry — the shared setpoint popup (handles its
                // own passcode protection and fires dz:setpoint:saved on success).
                function openSetpointPopup(evt) {
                    if (typeof ShowSetpointPopup === 'function') {
                        ShowSetpointPopup(evt, cfg().deviceIdx, ctrl.deviceProtected, ctrl.value, false,
                            ctrl.deviceStep, ctrl.effectiveMin, ctrl.effectiveMax);
                    }
                }

                // ── Switch toggle ─────────────────────────────────────────
                function toggleSwitch(passcode) {
                    if (ctrl.sending) { return; }
                    ctrl.sending = true;
                    var cmd;
                    if (ctrl.switchType === 'Push On Button')       { cmd = 'On'; }
                    else if (ctrl.switchType === 'Push Off Button') { cmd = 'Off'; }
                    else                                            { cmd = 'Toggle'; }

                    // Capture expected state now — ctrl.switchOn may change via WS before .then() runs
                    var expectedOn = (cmd === 'Toggle') ? !ctrl.switchOn : (cmd === 'On');

                    // Push buttons: dim to muted briefly then restore (uses existing transition)
                    if (ctrl.switchType === 'Push On Button' || ctrl.switchType === 'Push Off Button') {
                        ctrl.switchOn = false;
                        setTimeout(function() {
                            $scope.$apply(function() {
                                ctrl.switchOn = true;
                                ctrl.valueStr = $.t(ctrl.switchType === 'Push Off Button' ? 'Off' : 'On');
                            });
                        }, 300);
                    }

                    $http.get('json.htm', {
                        params: { type: 'command', param: 'switchlight',
                                  idx: cfg().deviceIdx, switchcmd: cmd,
                                  passcode: passcode || '' }
                    }).then(function(resp) {
                        if (resp.data && resp.data.status === 'OK') {
                            if (ctrl.switchType === 'Push On Button') {
                                ctrl.valueStr = $.t('On');
                                ctrl.value    = 1;
                            } else if (ctrl.switchType === 'Push Off Button') {
                                ctrl.valueStr = $.t('Off');
                                ctrl.value    = 1;
                            } else {
                                ctrl.switchOn = expectedOn;
                                ctrl.valueStr = expectedOn ? $.t('On') : $.t('Off');
                                ctrl.value    = expectedOn ? 1 : 0;
                                load();
                            }
                        }
                        ctrl.sending = false;
                    }).catch(function() { ctrl.sending = false; });
                }

                ctrl.handleClick = function() {
                    if (ctrl.deviceType !== 'switch' || ctrl.editMode) { return; }
                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function(passcode) {
                                _verifiedDevices[idx] = passcode;
                                $scope.$apply(function() { toggleSwitch(passcode); });
                            });
                        }
                        return;
                    }
                    toggleSwitch(_verifiedDevices[idx] || '');
                };

                // ── Drag to set ───────────────────────────────────────────

                function getSvgPoint(clientX, clientY) {
                    if (!_svgEl) { return null; }
                    var pt  = _svgEl.createSVGPoint();
                    pt.x    = clientX;
                    pt.y    = clientY;
                    var ctm = _svgEl.getScreenCTM();
                    if (!ctm) { return null; }
                    return pt.matrixTransform(ctm.inverse());
                }

                var DEAD_ZONE_GUARD = 20;
                function angleToValue(angleDeg) {
                    var norm    = ((angleDeg % 360) + 360) % 360;
                    var shifted = norm - ARC_START;
                    if (shifted < 0) { shifted += 360; }
                    if (shifted > ARC_SPAN + DEAD_ZONE_GUARD) { return null; }
                    var p   = Math.min(shifted / ARC_SPAN, 1.0);
                    var raw = ctrl.effectiveMin + p * (ctrl.effectiveMax - ctrl.effectiveMin);
                    return roundToStep(clamp(raw));
                }

                function getActiveLevelIdx() {
                    for (var i = 0; i < ctrl.levelOptions.length; i++) {
                        if (ctrl.levelOptions[i].level === ctrl.levelInt) { return i; }
                    }
                    return 0;
                }

                // Pixels of vertical drag per one selector step
                var SEL_DRAG_PX = 22;
                var _dragStartClientY  = 0;
                var _dragStartLevelIdx = 0;

                function updateFromClient(clientX, clientY) {
                    if (ctrl.deviceType === 'selector') {
                        // Vertical drag: drag up → go to lower index; down → higher index
                        var dy     = clientY - _dragStartClientY;
                        var offset = -Math.round(dy / SEL_DRAG_PX);
                        var n      = ctrl.levelOptions.length;
                        var newIdx = Math.max(0, Math.min(n - 1, _dragStartLevelIdx + offset));
                        ctrl.levelInt = ctrl.levelOptions[newIdx].level;
                        ctrl.value    = ctrl.levelInt;
                        updateSelectorDisplay();
                        ctrl.valueStr = ctrl.selectorActiveName;
                    } else {
                        var pt = getSvgPoint(clientX, clientY);
                        if (!pt) { return; }
                        var angle  = Math.atan2(pt.y - CY, pt.x - CX) * 180 / Math.PI;
                        var newVal = angleToValue(angle);
                        if (newVal === null) { return; }
                        ctrl.value    = newVal;
                        ctrl.valueStr = String(newVal);
                    }
                }

                function onDragMove(event) {
                    var e = event.originalEvent || event;
                    var clientX = e.touches ? e.touches[0].clientX : e.clientX;
                    var clientY = e.touches ? e.touches[0].clientY : e.clientY;
                    e.preventDefault();
                    $scope.$apply(function() { updateFromClient(clientX, clientY); });
                }

                function onDragEnd() {
                    $document.off('mousemove touchmove', onDragMove);
                    $document.off('mouseup touchend',   onDragEnd);
                    $scope.$apply(function() {
                        ctrl.dragging = false;
                        if (ctrl.deviceType === 'selector') {
                            ctrl.selectLevel(ctrl.levelInt, _verifiedDevices[cfg().deviceIdx] || '');
                        } else if (ctrl.value !== null) {
                            sendSetpoint(ctrl.value);
                        }
                    });
                }

                function startDrag(clientX, clientY) {
                    ctrl.dragging = true;
                    _dragStartClientY  = clientY;
                    _dragStartLevelIdx = getActiveLevelIdx();
                    // For setpoint: update from click position immediately
                    // For selector: vertical delta from drag start, so don't update on click
                    if (ctrl.deviceType !== 'selector') {
                        updateFromClient(clientX, clientY);
                    }
                    $document.on('mousemove touchmove', onDragMove);
                    $document.on('mouseup touchend',   onDragEnd);
                }

                ctrl.dragStart = function(event) {
                    var draggable = ctrl.deviceType === 'setpoint' || ctrl.deviceType === 'selector';
                    if (!draggable || ctrl.sending || ctrl.editMode) { return; }

                    var el = event.target;
                    while (el && el.tagName && el.tagName.toUpperCase() !== 'SVG') {
                        el = el.parentNode;
                    }
                    if (!el) { return; }
                    _svgEl = el;

                    var e       = event.originalEvent || event;
                    var clientX = e.touches ? e.touches[0].clientX : e.clientX;
                    var clientY = e.touches ? e.touches[0].clientY : e.clientY;

                    // Setpoint: split the dial by radius — the inner hub (where the
                    // value is shown) opens the precise-entry popup, the ring band
                    // drags to set, and clicks outside the dial are ignored.
                    if (ctrl.deviceType === 'setpoint') {
                        var pt = getSvgPoint(clientX, clientY);
                        if (pt) {
                            var dx = pt.x - CX, dy = pt.y - CY;
                            var r  = Math.sqrt(dx * dx + dy * dy);
                            if (r > BEZEL_R) { return; }
                            if (r < INNER_R) {
                                e.preventDefault();
                                openSetpointPopup({ clientX: clientX, clientY: clientY, target: e.target });
                                return;
                            }
                        }
                    }

                    e.preventDefault();

                    var idx = cfg().deviceIdx;
                    if (ctrl.deviceProtected && !_verifiedDevices[idx]) {
                        if (typeof HandleProtection === 'function') {
                            HandleProtection(ctrl.deviceProtected, function(passcode) {
                                _verifiedDevices[idx] = passcode;
                                $scope.$apply(function() { startDrag(clientX, clientY); });
                            });
                        }
                        return;
                    }
                    startDrag(clientX, clientY);
                };

                // ── Selector level switch ─────────────────────────────────
                ctrl.selectLevel = function(level, passcode) {
                    if (ctrl.sending) { return; }
                    ctrl.sending = true;
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'switchlight',
                                  idx: cfg().deviceIdx,
                                  switchcmd: level === 0 ? 'Off' : 'Set Level',
                                  level: level,
                                  passcode: passcode || '' }
                    }).then(function() {
                        ctrl.levelInt = level;
                        ctrl.value    = level;
                        updateSelectorDisplay();
                        ctrl.valueStr = ctrl.selectorActiveName;
                        ctrl.sending  = false;
                    }).catch(function() { ctrl.sending = false; });
                };

                // ── WebSocket updates ─────────────────────────────────────
                $scope.$on('device_update', function(e, updated) {
                    var c = cfg();
                    if (!ctrl.dragging && c && String(updated.idx) === String(c.deviceIdx)) {
                        applyDevice(updated);
                        applyUnitOverride();
                    }
                });

                $scope.$on('dd:widget:refresh', load);

                // Reflect saves made through the setpoint popup immediately
                function onSetpointSaved(e, data) {
                    var c = cfg();
                    if (c && String(data.idx) === String(c.deviceIdx)) {
                        $scope.$applyAsync(function() {
                            ctrl.value    = data.value;
                            ctrl.valueStr = String(data.value);
                        });
                    }
                }
                $document.on('dz:setpoint:saved', onSetpointSaved);

                $scope.$watch(
                    function() {
                        var c = ctrl.widgetDef && ctrl.widgetDef.config;
                        return c ? c.deviceIdx + '|' + c.minVal + '|' + c.maxVal : '';
                    },
                    function(val, old) { if (val !== old) { load(); } }
                );

                // Title updates instantly from config without a full reload
                $scope.$watch(
                    function() {
                        var c = ctrl.widgetDef && ctrl.widgetDef.config;
                        return c ? (c.title || '') : '';
                    },
                    function(newTitle) {
                        ctrl.title = newTitle || ctrl.deviceName || '';
                    }
                );

                $scope.$on('$destroy', function() {
                    $document.off('mousemove touchmove', onDragMove);
                    $document.off('mouseup touchend',   onDragEnd);
                    $document.off('dz:setpoint:saved', onSetpointSaved);
                    if (cancelToken) { cancelToken.resolve(); cancelToken = null; }
                });

                ctrl.$onInit = load;
            }],
            link: function(scope, element) {
                // AngularJS 1.x has no ng-touchstart directive, so attach a
                // native touchstart listener via event delegation.  The handler
                // only fires when the touch originates inside the SVG, and uses
                // { passive: false } so preventDefault() inside dragStart works.
                function onTouchStart(e) {
                    var svgEl = element[0].querySelector('svg.dd-dial-svg');
                    if (!svgEl || !svgEl.contains(e.target)) { return; }
                    scope.$apply(function() { scope.ctrl.dragStart(e); });
                }
                element[0].addEventListener('touchstart', onTouchStart, { passive: false });
                scope.$on('$destroy', function() {
                    element[0].removeEventListener('touchstart', onTouchStart);
                });
            }
        };
    }]);
});
