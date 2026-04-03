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

        self.sensorarea = 'usage';
        self.hasReturn = false;
        self.usageLabel = $.t('Usage');
        self.returnLabel = $.t('Return');

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

        function hasDeliveryData(items) {
            return items.some(function (item) {
                return (parseFloat(item.r1) || 0) + (parseFloat(item.r2) || 0) > 0;
            });
        }

        function buildCumulativeSeries(items, yr, isP1, sensorarea) {
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
                    ? (sensorarea === 'delivery'
                        ? Math.abs(parseFloat(item.r1) || 0) + Math.abs(parseFloat(item.r2) || 0)
                        : (parseFloat(item.v1) || 0) + (parseFloat(item.v2) || 0))
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

        function buildChartTitle(isGenerated) {
            if (self.sensorarea === 'delivery') {
                return $.t('Cumulative Energy') + ' ' + $.t('Return');
            }
            if (isGenerated) return $.t('Cumulative Energy Generated');
            if (self.subtype === 'gas') return $.t('Cumulative Gas');
            if (self.subtype === 'water') return $.t('Cumulative Water');
            return $.t('Cumulative Energy');
        }

        function buildChart(allSeries, isGenerated, currentYear) {
            var unit = unitForSubtype(self.subtype);
            self.chartTitle = buildChartTitle(isGenerated);
            var seriesList = [];

            allSeries.forEach(function (s, i) {
                if (s && s.data && s.data.length > 0) {
                    seriesList.push({
                        name: String(s.year),
                        type: 'spline',
                        color: seriesColors[i % seriesColors.length],
                        lineWidth: s.year === currentYear ? 3 : 2,
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
                chart: { type: 'spline', zoomType: 'x' },
                title: null,
                xAxis: {
                    min: 1,
                    max: leap ? 366 : 365,
                    tickPositions: doys.slice(),
                    labels: {
                        formatter: function () { return tickLabels[this.value] || ''; }
                    },
                    title: { text: null },
                    crosshair: true
                },
                yAxis: {
                    title: { text: unit },
                    min: 0
                },
                legend: { enabled: true },
                tooltip: {
                    shared: true,
                    formatter: function () {
                        var date = this.points[0].point.date;
                        var s = '<b>' + date + '</b><br/>';
                        this.points.forEach(function (point) {
                            s += point.series.name + ': <b>' +
                                Highcharts.numberFormat(point.y, 3) + ' ' + unit + '</b><br/>';
                        });
                        return s;
                    }
                },
                plotOptions: {
                    spline: { lineWidth: 2 },
                    series: { animation: false }
                },
                series: seriesList
            };
        }

        self.toggleSensorArea = function (area) {
            if (!self.hasReturn || self.sensorarea === area) { return; }
            self.sensorarea = area;
            rebuildFromData();
        };

        function rebuildFromData() {
            var isP1 = (self.subtype === 'p1Energy');
            var isGenerated = (self.device.SwitchTypeVal === 4) && !isP1;
            var now = new Date();
            var currentYear = now.getFullYear();
            var allSeries = [];

            var curData = buildCumulativeSeries(self._currentYearData, currentYear, isP1, self.sensorarea);
            if (curData && curData.length > 0) {
                allSeries.push({ year: currentYear, data: curData });
            }

            [0, 1].forEach(function (i) {
                var yr = currentYear - 1 - i;
                var data = buildCumulativeSeries(self._prevResults[i], yr, isP1, self.sensorarea);
                if (data && data.length > 0) {
                    allSeries.push({ year: yr, data: data });
                }
            });

            allSeries.sort(function (a, b) { return a.year - b.year; });
            buildChart(allSeries, isGenerated, currentYear);
        }

        self.$onInit = function () {
            var isP1 = (self.subtype === 'p1Energy');
            var sensor = 'counter';
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
                    self._prevResults = prevResults;
                    self._currentYearData = currentYearData;

                    if (isP1) {
                        var allItems = currentYearData.concat(prevResults[0]).concat(prevResults[1]);
                        self.hasReturn = hasDeliveryData(allItems);
                    } else {
                        self.hasReturn = false;
                    }

                    rebuildFromData();
                });
            });
        };
    }
});
