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

        self.hasReturn = false;
        self.showUsage = true;
        self.showReturn = true;

        function buildChart(data, deviceType, isP1) {
            var isGenerated = (deviceType === 4);
            var label = isGenerated ? $.t('Generated') : $.t('Usage');
            var titleText = isGenerated ? $.t('Generated Last Year') : $.t('Usage Last Year');
            var unit = unitForSubtype(self.subtype);

            if (!data || data.length === 0) {
                self.chartDefinition = { title: { text: titleText }, series: [] };
                return;
            }

            var usageData = [];
            var returnData = [];
            var maxUsage = 0;
            var maxReturn = 0;
            var hasReturn = false;

            data.forEach(function (item) {
                if (!item.d || item.d.length < 10) { return; }
                var parts = item.d.split('-');
                var date = new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]));
                var usageVal, returnVal;
                if (isP1) {
                    usageVal = (parseFloat(item.v1) || 0) + (parseFloat(item.v2) || 0);
                    returnVal = Math.abs(parseFloat(item.r1) || 0) + Math.abs(parseFloat(item.r2) || 0);
                } else {
                    usageVal = Math.max(0, parseFloat(item.v) || 0);
                    returnVal = 0;
                }
                var net = usageVal - returnVal;
                if (usageVal > maxUsage) { maxUsage = usageVal; }
                if (returnVal > maxReturn) { maxReturn = returnVal; }
                if (returnVal > 0) { hasReturn = true; }
                var point = {
                    x: weekOfYear(date),
                    y: isoWeekday(date),
                    usageVal: usageVal,
                    returnVal: returnVal,
                    price: parseFloat(item.p) || 0,
                    date: item.d
                };
                if (net >= 0) {
                    usageData.push(Object.assign({ value: net }, point));
                } else {
                    returnData.push(Object.assign({ value: -net }, point));
                }
            });

            // Build month-label tick positions on x-axis
            var year = parseInt(data[0].d.substring(0, 4));
            var monthNames = [
                $.t('Jan'), $.t('Feb'), $.t('Mar'), $.t('Apr'),
                $.t('May'), $.t('Jun'), $.t('Jul'), $.t('Aug'),
                $.t('Sep'), $.t('Oct'), $.t('Nov'), $.t('Dec')
            ];
            self.hasReturn = hasReturn;
            self.showUsage = true;
            self.showReturn = true;
            self.usageLabel = label;
            self.returnLabel = $.t('Return');

            var xAxisCategories = [];
            var xAxisTickPositions = [];
            var maxWeek = 0;
            usageData.concat(returnData).forEach(function (p) { if (p.x > maxWeek) { maxWeek = p.x; } });

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
                colorAxis: hasReturn ? [
                    { min: 0, minColor: 'rgba(255,255,255,0.05)', maxColor: '#03BEFC' },
                    { min: 0, minColor: 'rgba(255,255,255,0.05)', maxColor: '#00E676' }
                ] : {
                    min: 0,
                    minColor: 'rgba(255,255,255,0.05)',
                    maxColor: '#03BEFC'
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
                        var p = this.point;
                        var s = '<b>' + p.date + '</b><br/>' +
                            $.t('Usage') + ': <b>' + Highcharts.numberFormat(p.usageVal, 3) + ' ' + unit + '</b>';
                        if (hasReturn) {
                            s += '<br/>' + $.t('Return') + ': <b>' + Highcharts.numberFormat(p.returnVal, 3) + ' ' + unit + '</b>' +
                                '<br/>' + $.t('Net') + ': <b>' + Highcharts.numberFormat(p.usageVal - p.returnVal, 3) + ' ' + unit + '</b>';
                        }
                        if (p.price) {
                            s += '<br/><span style="color:#f1c40f">' + $.t('Price') + ': <b>' + Highcharts.numberFormat(p.price, 2) + ' ' + $.myglobals.currencysign + '</b></span>';
                        }
                        return s;
                    }
                },
                series: hasReturn ? [
                    {
                        name: label,
                        colorAxis: 0,
                        borderWidth: 2,
                        borderColor: 'rgba(0,0,0,0.3)',
                        nullColor: 'rgba(255,255,255,0.05)',
                        data: usageData
                    },
                    {
                        name: $.t('Return'),
                        colorAxis: 1,
                        borderWidth: 2,
                        borderColor: 'rgba(0,0,0,0.3)',
                        nullColor: 'rgba(255,255,255,0.05)',
                        data: returnData
                    }
                ] : [{
                    name: label,
                    borderWidth: 2,
                    borderColor: 'rgba(0,0,0,0.3)',
                    nullColor: 'rgba(255,255,255,0.05)',
                    data: usageData
                }],
                plotOptions: {
                    series: { animation: false }
                }
            };
        }

        self.toggleSeries = function (idx) {
            if (idx === 0) {
                self.showUsage = !self.showUsage;
            } else {
                self.showReturn = !self.showReturn;
            }
            var id = 'chart-' + self.device.idx + '-calendar-heatmap';
            var chart = Highcharts.charts.find(function (c) {
                return c && c.renderTo && c.renderTo.id === id;
            });
            if (chart && chart.series[idx]) {
                chart.series[idx].setVisible(idx === 0 ? self.showUsage : self.showReturn, true);
            }
        };

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
