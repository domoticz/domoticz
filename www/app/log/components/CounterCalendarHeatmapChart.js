define(['app'], function (app) {

    app.component('counterCalendarHeatmapChart', {
        require: {
            logCtrl: '^deviceCounterLog'
        },
        bindings: {
            device: '<',
            subtype: '<'
        },
        templateUrl: 'app/log/components/chart-counter-calendar-heatmap.html',
        controller: CounterCalendarHeatmapChartController
    });

    function CounterCalendarHeatmapChartController($scope) {
        var self = this;

        // ISO weekday: 0=Mon ... 6=Sun
        function isoWeekday(date) {
            var d = date.getDay(); // 0=Sun
            return (d === 0) ? 6 : d - 1;
        }

        function dayOfYear(date) {
            var start = new Date(date.getFullYear(), 0, 0);
            return Math.floor((date - start) / 86400000);
        }

        // Return 0-based week index within the year (week containing Jan 1 = week 0)
        function weekOfYear(date) {
            var jan1 = new Date(date.getFullYear(), 0, 1);
            var jan1IsoDay = isoWeekday(jan1);
            var doy = dayOfYear(date) - 1; // 0-based day of year
            return Math.floor((doy + jan1IsoDay) / 7);
        }

        function unitForSubtype(subtype) {
            if (subtype === 'gas' || subtype === 'water') { return 'm³'; }
            return 'kWh';
        }

        function buildChart(data, deviceType, isP1) {
            var isGenerated = (deviceType === 4);
            var label = isGenerated ? $.t('Generated') : $.t('Usage');
            var titleText = isGenerated ? $.t('Generated Last Year') : $.t('Usage Last Year');
            var unit = unitForSubtype(self.subtype);

            if (!data || data.length === 0) {
                self.chartDefinition = { title: { text: titleText }, series: [] };
                return;
            }

            var heatData = [];
            var maxVal = 0;

            data.forEach(function (item) {
                if (!item.d || item.d.length < 10) { return; }
                var parts = item.d.split('-');
                var date = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]));
                var val;
                if (isP1) {
                    val = (parseFloat(item.v1) || 0) + (parseFloat(item.v2) || 0);
                } else {
                    val = Math.max(0, parseFloat(item.v) || 0);
                }
                if (val > maxVal) { maxVal = val; }
                heatData.push({
                    x: weekOfYear(date),
                    y: isoWeekday(date),
                    value: val,
                    date: item.d
                });
            });

            // Build month-label tick positions on x-axis
            var year = parseInt(data[0].d.substring(0, 4));
            var monthNames = [
                $.t('Jan'), $.t('Feb'), $.t('Mar'), $.t('Apr'),
                $.t('May'), $.t('Jun'), $.t('Jul'), $.t('Aug'),
                $.t('Sep'), $.t('Oct'), $.t('Nov'), $.t('Dec')
            ];
            var xAxisCategories = [];
            var xAxisTickPositions = [];
            var maxWeek = 0;
            heatData.forEach(function (p) { if (p.x > maxWeek) { maxWeek = p.x; } });

            for (var w = 0; w <= maxWeek; w++) { xAxisCategories.push(''); }
            for (var m = 0; m < 12; m++) {
                var firstDay = new Date(year, m, 1);
                var wk = weekOfYear(firstDay);
                if (wk <= maxWeek) {
                    xAxisCategories[wk] = monthNames[m];
                    xAxisTickPositions.push(wk);
                }
            }

            self.chartDefinition = {
                chart: {
                    type: 'heatmap',
                    marginTop: 35,
                    marginBottom: 45
                },
                title: {
                    text: titleText
                },
                xAxis: {
                    categories: xAxisCategories,
                    tickPositions: xAxisTickPositions,
                    title: { text: null },
                    labels: { rotation: 0 }
                },
                yAxis: {
                    categories: [
                        $.t('Mon'), $.t('Tue'), $.t('Wed'), $.t('Thu'),
                        $.t('Fri'), $.t('Sat'), $.t('Sun')
                    ],
                    title: { text: null },
                    reversed: false,
                    labels: { step: 1 }
                },
                colorAxis: {
                    min: 0,
                    minColor: 'rgba(255,255,255,0.05)',
                    maxColor: '#00E676'
                },
                legend: {
                    align: 'right',
                    layout: 'vertical',
                    margin: 0,
                    verticalAlign: 'middle',
                    symbolHeight: 60
                },
                tooltip: {
                    formatter: function () {
                        return '<b>' + this.point.date + '</b><br/>' +
                            label + ': <b>' + Highcharts.numberFormat(this.point.value, 3) + ' ' + unit + '</b>';
                    }
                },
                series: [{
                    name: label,
                    borderWidth: 2,
                    borderColor: 'rgba(0,0,0,0.3)',
                    nullColor: 'rgba(255,255,255,0.05)',
                    data: heatData
                }],
                plotOptions: {
                    series: { animation: false }
                }
            };
        }

        self.$onInit = function () {
            var isP1 = (self.subtype === 'p1Energy');
            $scope.$watch(function () {
                return self.logCtrl.yearGraphData;
            }, function (data) {
                if (!data || data.length === 0) { return; }
                buildChart(data, self.device.SwitchTypeVal, isP1);
            });
        };
    }
});
