define(['app', 'luxon'], function (app, luxon) {
    var DateTime = luxon.DateTime;

    app.component('deviceOnOffChart', {
        bindings: {
            log: '<',
            view: '@'
        },
        template: '<div></div>',
        controller: DeviceOnOffChartController
    });

    function DeviceOnOffChartController($element, dzSettings) {
        var vm = this;

        vm.$onChanges = function (changes) {
            if (changes.log && changes.log.currentValue) {
                renderChart(changes.log.currentValue)
            }
        };

        function getMinValue() {
            if (vm.view === 'daily') {
                return DateTime.local().startOf('day').valueOf();
            } else if (vm.view === 'weekly') {
                return DateTime.local().minus({ days: 6 }).startOf('day').valueOf();
            } else {
                return undefined;
            }
        }

        function getFilteredData(data) {
            var min = getMinValue();

            var firstIndex = min === undefined
                ? data.findIndex(function (point) {
                    return DateTime.fromFormat(point.Date, dzSettings.serverDateFormat).valueOf() >= min
                })
                : 0;

            // Add one point out of the range to properly render initial value
            if (firstIndex > 0) {
                firstIndex -= 1;
            }

            return firstIndex === -1
                ? []
                : data.slice(firstIndex);
        }

        function renderChart(data) {
            var chartData = [];

            getFilteredData(data).forEach(function (point, index, points) {
                if (point.Status === 'On'
                    || (point.Status.includes('Set Level') && point.Level > 0)
                    || (point.Status.includes('Set Color'))
                ) {
                    chartData.push({
                        x: DateTime.fromFormat(point.Date, dzSettings.serverDateFormat).valueOf(),
                        x2: points[index + 1] ? DateTime.fromFormat(points[index + 1].Date,  dzSettings.serverDateFormat).valueOf() : Date.now(),
                        y: 0
                    });
                }
            });

            $element.highcharts({
                chart: {
                    zoomType: 'x',
                },
                title: {
                    text: null
                },
                legend: {
                    enabled: false
                },
                xAxis: {
                    type: 'datetime',
                    min: getMinValue(),
                    max: DateTime.local().endOf('day').valueOf(),
                    startOnTick: true,
                    endOnTick: true
                },
                yAxis: {
                    title: {
                        text: ''
                    },
                    labels: false,
                    categories: ['On'],
                    reversed: true
                },
                time: {
                    useUTC: false,
                },
                series: [{
                    type: 'xrange',
                    name: 'Device Status',
                    data: chartData,
                    borderRadius: 0,
                    borderWidth: 0,
                    dataLabels: {
                        enabled: false
                    },
                    minPointLength: 2,
                    turboThreshold: 0
                }]
            });
        }
    }
});
