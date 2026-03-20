define(['app'], function (app) {

    app.component('counterCumulativeChart', {
        require: {
            logCtrl: '^deviceCounterLog'
        },
        bindings: {
            device: '<',
            subtype: '<'
        },
        templateUrl: 'app/log/components/chart-counter-cumulative.html',
        controller: CounterCumulativeChartController
    });

    function CounterCumulativeChartController($scope, $http, $q) {
        var self = this;

        var seriesColors = ['#7cb5ec', '#90ed7d', '#f7a35c'];

        var monthDoys =     [1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335];
        var monthDoysLeap = [1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336];
        var monthNames = [
            $.t('Jan'), $.t('Feb'), $.t('Mar'), $.t('Apr'),
            $.t('May'), $.t('Jun'), $.t('Jul'), $.t('Aug'),
            $.t('Sep'), $.t('Oct'), $.t('Nov'), $.t('Dec')
        ];

        function isLeapYear(yr) {
            return (yr % 4 === 0 && yr % 100 !== 0) || (yr % 400 === 0);
        }

        function dayOfYear(date) {
            var start = new Date(date.getFullYear(), 0, 0);
            return Math.floor((date - start) / 86400000);
        }

        function fetchYear(idx, sensor, year) {
            var url = 'json.htm?type=command&param=graph&sensor=' + sensor + '&idx=' + idx + '&range=year&actyear=' + year;
            return $http({ url: url, dataType: 'json' }).then(function (resp) {
                return (resp.data && resp.data.result) ? resp.data.result : [];
            }, function () { return []; });
        }

        function buildCumulativeSeries(items, yr, isP1) {
            var filtered = items.filter(function (item) {
                return item.d && item.d.substring(0, 4) === String(yr);
            });
            if (filtered.length === 0) { return null; }

            filtered.sort(function (a, b) { return a.d < b.d ? -1 : a.d > b.d ? 1 : 0; });

            var cumulative = 0;
            var dataPoints = [];
            filtered.forEach(function (item) {
                var parts = item.d.split('-');
                var date = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]));
                var val = isP1
                    ? (parseFloat(item.v1) || 0) + (parseFloat(item.v2) || 0)
                    : (parseFloat(item.v) || 0);
                cumulative += val;
                dataPoints.push({
                    x: dayOfYear(date),
                    y: parseFloat(cumulative.toFixed(3)),
                    date: item.d
                });
            });
            return dataPoints;
        }

        function unitForSubtype(subtype) {
            if (subtype === 'gas' || subtype === 'water') { return 'm³'; }
            return 'kWh';
        }

        function buildChart(allSeries, isGenerated, currentYear) {
            var unit = unitForSubtype(self.subtype);
            var titleText = isGenerated ? $.t('Cumulative Energy Generated') :
                (self.subtype === 'gas' ? $.t('Cumulative Gas') :
                self.subtype === 'water' ? $.t('Cumulative Water') : $.t('Cumulative Energy'));
            var seriesList = [];

            allSeries.forEach(function (s, i) {
                if (s && s.data && s.data.length > 0) {
                    seriesList.push({
                        name: String(s.year),
                        type: 'spline',
                        color: seriesColors[i % seriesColors.length],
                        data: s.data,
                        tooltip: { valueDecimals: 3, valueSuffix: ' ' + unit },
                        marker: { enabled: false }
                    });
                }
            });

            var leap = isLeapYear(currentYear);
            var doys = leap ? monthDoysLeap : monthDoys;
            var tickLabels = {};
            doys.forEach(function (doy, idx) { tickLabels[doy] = monthNames[idx]; });

            self.chartDefinition = {
                chart: { type: 'spline' },
                title: { text: titleText },
                xAxis: {
                    min: 1,
                    max: leap ? 366 : 365,
                    tickPositions: doys.slice(),
                    labels: {
                        formatter: function () { return tickLabels[this.value] || ''; }
                    },
                    title: { text: null }
                },
                yAxis: {
                    title: { text: unit },
                    min: 0
                },
                legend: { enabled: true },
                tooltip: {
                    shared: false,
                    formatter: function () {
                        return '<b>' + this.point.date + '</b><br/>' +
                            this.series.name + ': <b>' +
                            Highcharts.numberFormat(this.y, 3) + ' ' + unit + '</b>';
                    }
                },
                plotOptions: {
                    spline: { lineWidth: 2 },
                    series: { animation: false }
                },
                series: seriesList
            };
        }

        self.$onInit = function () {
            var isP1 = (self.subtype === 'p1Energy');
            var sensor = 'counter';
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
                    var allSeries = [];

                    // Current year from shared data
                    var curData = buildCumulativeSeries(currentYearData, currentYear, isP1);
                    if (curData && curData.length > 0) {
                        allSeries.push({ year: currentYear, data: curData });
                    }

                    // Previous years from API
                    [0, 1].forEach(function (i) {
                        var yr = currentYear - 1 - i;
                        var data = buildCumulativeSeries(prevResults[i], yr, isP1);
                        if (data && data.length > 0) {
                            allSeries.push({ year: yr, data: data });
                        }
                    });

                    buildChart(allSeries, isGenerated, currentYear);
                });
            });
        };
    }
});
