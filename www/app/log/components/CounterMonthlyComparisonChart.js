define(['app'], function (app) {

    app.component('counterMonthlyComparisonChart', {
        require: {
            logCtrl: '^deviceCounterLog'
        },
        bindings: {
            device: '<'
        },
        templateUrl: 'app/log/components/chart-counter-monthly-comparison.html',
        controller: CounterMonthlyComparisonChartController
    });

    function CounterMonthlyComparisonChartController($scope, $http, $q) {
        var self = this;

        var monthNames = [
            $.t('Jan'), $.t('Feb'), $.t('Mar'), $.t('Apr'),
            $.t('May'), $.t('Jun'), $.t('Jul'), $.t('Aug'),
            $.t('Sep'), $.t('Oct'), $.t('Nov'), $.t('Dec')
        ];

        var yearColors = ['#7cb5ec', '#90ed7d', '#f7a35c'];

        function groupByMonth(items, isP1) {
            var monthly = {};
            for (var m = 1; m <= 12; m++) { monthly[m] = 0; }
            items.forEach(function (item) {
                if (!item.d || item.d.length < 7) { return; }
                var month = parseInt(item.d.substring(5, 7), 10);
                var val = isP1
                    ? (parseFloat(item.v1) || 0) + (parseFloat(item.v2) || 0)
                    : (parseFloat(item.v) || 0);
                monthly[month] = (monthly[month] || 0) + val;
            });
            return monthly;
        }

        function fetchYear(idx, sensor, year) {
            var url = 'json.htm?type=command&param=graph&sensor=' + sensor + '&idx=' + idx + '&range=year&actyear=' + year;
            return $http({ url: url, dataType: 'json' }).then(function (resp) {
                return (resp.data && resp.data.result) ? resp.data.result : [];
            }, function () { return []; });
        }

        function buildChart(yearDataMap, isGenerated) {
            var titleText = isGenerated ? $.t('Monthly Generated Comparison') : $.t('Monthly Energy Comparison');
            var seriesList = [];
            var years = Object.keys(yearDataMap).sort();

            years.forEach(function (yr, i) {
                var monthly = yearDataMap[yr];
                var dataArr = [];
                var hasAny = false;
                for (var m = 1; m <= 12; m++) {
                    var v = monthly[m] || 0;
                    if (v > 0) { hasAny = true; }
                    dataArr.push(v > 0 ? parseFloat(v.toFixed(3)) : null);
                }
                if (hasAny) {
                    seriesList.push({
                        name: yr,
                        type: 'column',
                        color: yearColors[i % yearColors.length],
                        data: dataArr
                    });
                }
            });

            self.chartDefinition = {
                chart: { type: 'column' },
                title: { text: titleText },
                xAxis: {
                    categories: monthNames,
                    title: { text: null }
                },
                yAxis: {
                    title: { text: 'kWh' },
                    min: 0
                },
                legend: { enabled: true },
                tooltip: {
                    shared: true,
                    valueSuffix: ' kWh',
                    valueDecimals: 3
                },
                plotOptions: {
                    column: {
                        grouping: true,
                        borderWidth: 0,
                        pointPadding: 0.1,
                        groupPadding: 0.1
                    },
                    series: { animation: false }
                },
                series: seriesList
            };
        }

        self.$onInit = function () {
            var isP1 = (self.device.Type === 'P1 Smart Meter');
            var sensor = isP1 ? 'p1energy' : 'counter';
            var isGenerated = (self.device.SwitchTypeVal === 4);
            var idx = self.device.idx;
            var now = new Date();
            var currentYear = now.getFullYear();

            $scope.$watch(function () {
                return self.logCtrl.yearGraphData;
            }, function (currentYearData) {
                if (!currentYearData || currentYearData.length === 0) { return; }

                $q.all([
                    fetchYear(idx, sensor, currentYear - 1),
                    fetchYear(idx, sensor, currentYear - 2)
                ]).then(function (prevResults) {
                    var yearDataMap = {};

                    // Current year from shared data
                    var curItems = currentYearData.filter(function (item) {
                        return item.d && item.d.substring(0, 4) === String(currentYear);
                    });
                    if (curItems.length > 0) {
                        yearDataMap[String(currentYear)] = groupByMonth(curItems, isP1);
                    }

                    // Previous years from API
                    [0, 1].forEach(function (i) {
                        var yr = String(currentYear - 1 - i);
                        var items = prevResults[i].filter(function (item) {
                            return item.d && item.d.substring(0, 4) === yr;
                        });
                        if (items.length > 0) {
                            yearDataMap[yr] = groupByMonth(items, isP1);
                        }
                    });

                    buildChart(yearDataMap, isGenerated);
                });
            });
        };
    }
});
