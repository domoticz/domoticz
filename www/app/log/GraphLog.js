define(['app', 'log/factories'], function (app) {
    app.component('deviceGraphLog', {
        bindings: {
            device: '<'
        },
        templateUrl: 'views/log/device_graph_log.html',
    });

    app.component('deviceLogChart', {
        bindings: {
            device: '<',
            deviceType: '<',
            degreeType: '<',
            range: '@'
        },
        template: '<div></div>',
        controllerAs: 'vm',
        controller: function ($scope, $element, $route, domoticzApi, domoticzDataPointApi, domoticzGlobals) {
            var vm = this;
            var chart;

            vm.$onInit = init;
            vm.$onChanges = onChanges;

            function init() {
                var sensor = getSensorName();
                var unit = getChartUnit();
                var axisTitle = vm.device.SubType === 'Custom Sensor'
                    ? unit
                    : sensor + ' (' + unit + ')';

                chart = $element.highcharts({
                    chart: {
                        type: 'spline',
                        zoomType: 'x',
                        resetZoomButton: {
                            position: {
                                x: -30,
                                y: -36
                            }
                        }
                    },
                    xAxis: {
                        type: 'datetime'
                    },
                    yAxis: {
                        title: {
                            text: axisTitle
                        },
                        labels: {
                            formatter: function () {
                                var value = unit === 'vM'
                                    ? Highcharts.numberFormat(this.value, 0)
                                    : this.value;

                                return value + ' ' + unit;
                            }
                        }
                    },
                    tooltip: {
                        crosshairs: true,
                        shared: true,
                        valueSuffix: ' ' + unit
                    },
                    plotOptions: {
                        series: {
                            point: {
                                events: {
                                    click: function (event) {
                                        if (event.shiftKey != true) {
                                            return;
                                        }

                                        domoticzDataPointApi
                                            .deletePoint(vm.device.idx, event.point, vm.range === 'day')
                                            .then(function () {
                                                $route.reload();
                                            });
                                    }
                                }
                            }
                        },
                        spline: {
                            lineWidth: 3,
                            states: {
                                hover: {
                                    lineWidth: 3
                                }
                            },
                            marker: {
                                enabled: false,
                                states: {
                                    hover: {
                                        enabled: true,
                                        symbol: 'circle',
                                        radius: 5,
                                        lineWidth: 1
                                    }
                                }
                            }
                        }
                    },
                    title: {
                        text: getChartTitle()
                    },
                    series: [],
                    legend: {
                        enabled: true
                    }
                }).highcharts();

                refreshChartData();
            }

            function onChanges(changesObj) {
                if (!chart) {
                    return;
                }

                if (changesObj.deviceIdx || changesObj.range) {
                    refreshChartData();
                }

                if (changesObj.deviceType) {
                    chart.setTitle({text: getChartTitle()});
                }
            }

            function refreshChartData() {
                domoticzApi
                    .sendRequest({
                        type: 'graph',
                        sensor: getChartType(),
                        range: vm.range,
                        idx: vm.device.idx
                    })
                    .then(function (data) {
                        if (typeof data.result === 'undefined') {
                            return;
                        }

                        var valueKey = getPointValueKey();

                        if (vm.range === 'day') {
                            var sensor = getSensorName();

                            chart.addSeries({
                                showInLegend: false,
                                name: sensor,
                                data: data.result.map(function (item) {
                                    return [GetUTCFromString(item.d), parseFloat(item[valueKey])]
                                })
                            }, false);
                        }

                        if (vm.range === 'month' || vm.range === 'year') {
                            var series = [
                                {name: 'min', data: []},
                                {name: 'max', data: []},
                                {name: 'avg', data: []}
                            ];

                            data.result.forEach(function (item) {
                                series[0].data.push([GetDateFromString(item.d), parseFloat(item[valueKey + '_min'])]);
                                series[1].data.push([GetDateFromString(item.d), parseFloat(item[valueKey + '_max'])]);

                                if (item[valueKey + '_avg'] !== undefined) {
                                    series[2].data.push([GetDateFromString(item.d), parseFloat(item[valueKey + '_avg'])]);
                                }
                            });

                            series
                                .filter(function (item) {
                                    return item.data.length > 0;
                                })
                                .forEach(function (item) {
                                    chart.addSeries(item, false);
                                });
                        }

                        chart.redraw();
                    });
            }

            function getPointValueKey() {
                if (vm.device.SubType === 'Lux') {
                    return 'lux'
                } else if (vm.device.Type === 'Usage') {
                    return 'u'
                } else {
                    return 'v';
                }
            }

            function getChartTitle() {
                if (vm.range === 'day') {
                    return domoticzGlobals.Get5MinuteHistoryDaysGraphTitle();
                } else if (vm.range === 'month') {
                    return $.t('Last Month');
                } else if (vm.range === 'year') {
                    return $.t('Last Year');
                }
            }

            function getChartUnit() {
                return vm.device.getUnit();
            }

            function getChartType() {
                if (['Custom Sensor', 'Waterflow', 'Percentage'].includes(vm.device.SubType)) {
                    return 'Percentage';
                } else {
                    return 'counter';
                }
            }

            function getSensorName() {
                if (vm.device.Type === 'Usage') {
                    return $.t('Usage');
                } else {
                    return $.t(vm.device.SubType)
                }
            }
        }
    });
});
