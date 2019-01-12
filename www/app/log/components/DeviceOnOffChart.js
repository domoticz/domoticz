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

    function DeviceOnOffChartController($element) {
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
                return DateTime.local().minus({days: 6}).startOf('day').valueOf();
            } else {
                return undefined;
            }
        }

        function renderChart(data) {
            var chartData = [];
            var min = getMinValue();

            data
                .filter(function(point) {
                    return min === undefined || Date.parse(point.Date) >= min
                })
                .forEach(function (point, index, points) {
                    if (point.Status === 'On' || (point.Status.includes('Set Level') && point.Level > 0)) {
                        chartData.push({
                            x: Date.parse(point.Date),
                            x2: points[index + 1] ? Date.parse(points[index + 1].Date) : Date.now(),
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
                    min: min,
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
