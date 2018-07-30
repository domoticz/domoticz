define(['app', 'log/factories', 'log/components'], function (app) {
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
                    credits: {
                        enabled: true,
                        href: 'http://www.domoticz.com',
                        text: 'Domoticz.com'
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
                if (vm.device.SubType === 'Custom Sensor') {
                    return vm.device.SensorUnit
                } else if (vm.device.Type === 'General' && vm.device.SubType === 'Voltage') {
                    return 'V';
                } else if (vm.device.Type === 'General' && vm.device.SubType === 'Distance') {
                    return vm.device.SwitchTypeVal === 1 ? 'in' : 'cm'
                } else if (vm.device.Type === 'General' && vm.device.SubType === 'Current') {
                    return 'A';
                } else if (vm.device.Type === 'General' && vm.device.SubType === 'Pressure') {
                    return 'Bar';
                } else if (vm.device.Type === 'General' && vm.device.SubType === 'Sound Level') {
                    return 'dB';
                } else if (vm.device.SubType === 'Visibility') {
                    return vm.device.SwitchTypeVal === 1 ? 'mi' : 'km';
                } else if (vm.device.SubType === 'Solar Radiation') {
                    return 'Watt/m2';
                } else if (vm.device.SubType === 'Soil Moisture') {
                    return 'cb';
                } else if (vm.device.SubType === 'Leaf Wetness') {
                    return 'Range';
                } else if (vm.device.SubType === 'Weight') {
                    return 'kg';
                } else if (['Voltage', 'A/D'].includes(vm.device.SubType)) {
                    return 'mV';
                } else if (vm.device.SubType === 'Waterflow') {
                    return 'l/min';
                } else if (vm.device.SubType === 'Lux') {
                    return 'lx';
                } else if (vm.device.SubType === 'Percentage') {
                    return '%';
                } else if (vm.device.Type === 'Usage' && vm.device.SubType === 'Electric') {
                    return 'W';
                } else {
                    return '?';
                }
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

    app.controller('DeviceGraphLogController', function ($routeParams, domoticzApi, deviceApi, permissions) {
        var vm = this;

        init();

        function init() {
            vm.deviceIdx = $routeParams.id;

            deviceApi.getDeviceInfo(vm.deviceIdx).then(function (device) {
                vm.pageName = device.Name;
                vm.device = device;
            });
        }
    });
});