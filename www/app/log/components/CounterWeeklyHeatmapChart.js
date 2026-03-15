define(['app'], function (app) {

    app.component('counterWeeklyHeatmapChart', {
        bindings: {
            device: '<'
        },
        templateUrl: 'app/log/components/chart-counter-weekly-heatmap.html',
        controller: CounterWeeklyHeatmapChartController
    });

    function CounterWeeklyHeatmapChartController($scope, $http) {
        var self = this;

        var dayDisplayNames = [
            $.t('Mon'), $.t('Tue'), $.t('Wed'), $.t('Thu'),
            $.t('Fri'), $.t('Sat'), $.t('Sun')
        ];

        // weekday_hour_kwh uses JS getDay() index: 0=Sun,1=Mon,...,6=Sat
        // Display order: 0=Mon(idx 1), 1=Tue(idx 2), ..., 5=Sat(idx 6), 6=Sun(idx 0)
        var displayToApiIndex = [1, 2, 3, 4, 5, 6, 0];

        function percentile95(values) {
            if (values.length === 0) { return 0; }
            var sorted = values.slice().sort(function (a, b) { return a - b; });
            var idx = Math.floor(sorted.length * 0.95);
            return sorted[Math.min(idx, sorted.length - 1)];
        }

        function buildChart(weekday_hour_kwh) {
            var heatData = [];
            var nonZeroValues = [];
            for (var displayDay = 0; displayDay < 7; displayDay++) {
                var apiIdx = displayToApiIndex[displayDay];
                var dayRow = weekday_hour_kwh[apiIdx];
                if (!dayRow) { continue; }
                for (var hour = 0; hour < 24; hour++) {
                    var val = dayRow[hour] || 0;
                    heatData.push({
                        x: hour,
                        y: displayDay,
                        value: val
                    });
                    if (val > 0) { nonZeroValues.push(val); }
                }
            }

            var colorMax = percentile95(nonZeroValues) || 1;

            self.chartDefinition = {
                chart: {
                    type: 'heatmap',
                    marginTop: 40,
                    marginBottom: 60
                },
                title: {
                    text: $.t('Weekly Usage Pattern')
                },
                xAxis: {
                    categories: ['00:00','01:00','02:00','03:00','04:00','05:00','06:00',
                        '07:00','08:00','09:00','10:00','11:00','12:00','13:00',
                        '14:00','15:00','16:00','17:00','18:00','19:00','20:00',
                        '21:00','22:00','23:00'],
                    title: { text: null }
                },
                yAxis: {
                    categories: dayDisplayNames,
                    title: { text: null },
                    reversed: false
                },
                colorAxis: {
                    min: 0,
                    max: colorMax,
                    minColor: 'rgba(0,0,0,0.3)',
                    maxColor: '#03BFFC'
                },
                legend: {
                    align: 'right',
                    layout: 'vertical',
                    margin: 0,
                    verticalAlign: 'top',
                    y: 25,
                    symbolHeight: 200
                },
                tooltip: {
                    formatter: function () {
                        var dayName = dayDisplayNames[this.point.y];
                        var hour = this.point.x;
                        return '<b>' + dayName + '</b><br/>' +
                            $.t('Hour') + ': ' + ('0' + hour).slice(-2) + ':00<br/>' +
                            $.t('Usage') + ': <b>' + Highcharts.numberFormat(this.point.value, 1) + ' Wh</b>';
                    }
                },
                series: [{
                    name: $.t('Usage'),
                    borderWidth: 1,
                    borderColor: 'rgba(255,255,255,0.05)',
                    nullColor: 'rgba(0,0,0,0.3)',
                    data: heatData
                }],
                plotOptions: {
                    series: { animation: false }
                }
            };
        }

        self.$onInit = function () {
            $http({
                url: 'json.htm?type=command&param=getkwhstats&idx=' + self.device.idx,
                dataType: 'json'
            }).then(function (response) {
                var data = response.data;
                if (data && data.status === 'OK' && data.result && data.result.weekday_hour_kwh) {
                    buildChart(data.result.weekday_hour_kwh);
                }
            });
        };
    }
});
