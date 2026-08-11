define([
    'app',
    'dashboardDynamic/dashboardDynamic.module'
], function(app) {
    'use strict';

    /**
     * ddSparkline
     *
     * Renders a minimal, borderless Highcharts areaspline ("sparkline") into a
     * container element — an always-on background trend line for any widget.
     * Shared so multiple widgets can offer a "show trend graph" option without
     * duplicating the chart configuration.
     *
     *   var chart = ddSparkline.render(elementId, data, { color: '#abc' });
     *   // data: array of [timestampMillis, value]
     *   // remember to chart.destroy() on $destroy / before re-render
     */
    app.factory('ddSparkline', [function() {

        function themeColor(varName, fallback) {
            var v = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
            return v || fallback;
        }

        // Parse a Domoticz graph date ("YYYY-MM-DD" or "YYYY-MM-DD HH:MM") as LOCAL
        // time — strings without a TZ suffix are otherwise treated as UTC.
        function parseLocal(dStr) {
            var parts = (dStr || '').split(' ');
            var ymd   = parts[0].split('-');
            var hms   = (parts[1] || '0:0:0').split(':');
            return new Date(+ymd[0], +ymd[1] - 1, +ymd[2],
                            +hms[0], +hms[1] || 0, +hms[2] || 0).getTime();
        }

        function render(elementId, data, opts) {
            opts = opts || {};
            var el = document.getElementById(elementId);
            if (!el || !window.Highcharts || !data || !data.length) { return null; }

            var color = opts.color || themeColor('--dz-widget-accent', '#43a4d3');

            // Scale the Y axis to the data's own range so small fluctuations on a
            // high baseline stay visible - e.g. barometric pressure hovering around
            // 1018 hPa. Without this, the areaspline's default threshold (0) drags
            // the axis down to zero and a real trend renders as a flat line.
            var yMin, yMax;
            for (var i = 0; i < data.length; i++) {
                var v = data[i][1];
                if (typeof v !== 'number' || isNaN(v)) { continue; }
                if (yMin === undefined || v < yMin) { yMin = v; }
                if (yMax === undefined || v > yMax) { yMax = v; }
            }
            var yExtremes = {};
            if (yMin !== undefined) {
                var pad = (yMax - yMin) * 0.1;
                if (!(pad > 0)) { pad = (Math.abs(yMax) * 0.01) || 1; } // flat data: keep the line centered
                yExtremes.min = yMin - pad;
                yExtremes.max = yMax + pad;
            }

            return window.Highcharts.chart(el, {
                credits:   { enabled: false },
                exporting: { enabled: false },
                title:     { text: null },
                legend:    { enabled: false },
                tooltip:   { enabled: false },
                chart: {
                    backgroundColor: 'transparent',
                    margin:    [2, 0, 2, 0],
                    height:    el.offsetHeight || 50,
                    animation: false,
                    style:     { fontFamily: 'inherit' }
                },
                time:  { useUTC: false },
                xAxis: { type: 'datetime', visible: false },
                yAxis: { visible: false, endOnTick: false, startOnTick: false, min: yExtremes.min, max: yExtremes.max },
                plotOptions: {
                    areaspline: {
                        lineWidth: 1.5,
                        threshold: null,   // don't anchor the fill/axis baseline to zero
                        marker:    { enabled: false },
                        states:    { hover: { enabled: false } },
                        color:     color,
                        fillColor: {
                            linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
                            stops: [
                                [0, window.Highcharts.color(color).setOpacity(0.35).get()],
                                [1, window.Highcharts.color(color).setOpacity(0.02).get()]
                            ]
                        }
                    }
                },
                series: [{ type: 'areaspline', data: data }]
            });
        }

        return { render: render, parseLocal: parseLocal };
    }]);
});
