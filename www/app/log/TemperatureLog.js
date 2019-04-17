define(['app', 'log/factories'], function (app) {

    app.component('deviceTemperatureLog', {
        bindings: {
            device: '<',
        },
        templateUrl: 'views/log/device_temperature_log.html',
        controller: function() {
            var vm = this;

            vm.$onInit = function() {
                vm.deviceIdx = vm.device.idx;
                vm.deviceType = vm.device.Type;
                vm.degreeType = $.myglobals.tempsign;
            }
        }
    });

    app.directive('temperatureLogChart', function () {
        return {
            scope: {
                deviceIdx: '<',
                deviceType: '<',
                degreeType: '<',
                range: '@'
            },
            replace: true,
            template: '<div style="height: 300px;"></div>',
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($scope, $element, $route, domoticzApi, domoticzDataPointApi, domoticzGlobals) {
                var vm = this;
                var chart;

                vm.$onInit = init;
                vm.$onChanges = onChanges;

                function init() {
                    chart = $element
                        .highcharts({
                            chart: {
                                type: getChartType(),
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
                            yAxis: [{ //temp label
                                labels: {
                                    formatter: function () {
                                        return this.value + '\u00B0 ' + vm.degreeType;
                                    }
                                },
                                title: {
                                    text: $.t('Degrees') + ' \u00B0' + vm.degreeType
                                }
                            }, { //humidity label
                                showEmpty: false,
                                allowDecimals: false,	//no need to show scale with decimals
                                ceiling: 100,			//max limit for auto zoom, bug in highcharts makes this sometimes not considered.
                                floor: 0,				//min limit for auto zoom
                                minRange: 10,			//min range for auto zoom
                                labels: {
                                    formatter: function () {
                                        return this.value + '%';
                                    }
                                },
                                title: {
                                    text: $.t('Humidity') + ' %'
                                },
                                opposite: true
                            }],
                            tooltip: {
                                crosshairs: true,
                                shared: true
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
                                                    .deletePoint(vm.deviceIdx, event.point, vm.range === 'day')
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
                                },
								line: {
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
								},
                                areasplinerange: {
                                    marker: {
                                        enabled: false
                                    }
                                }
                            },
                            title: {
                                text: getChartTitle()
                            },
                            legend: {
                                enabled: true
                            }
                        })
                        .highcharts();

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
                            sensor: 'temp',
                            range: vm.range,
                            idx: vm.deviceIdx
                        })
                        .then(function (data) {
                            if (typeof data.result != 'undefined') {
                                AddDataToTempChart(data, chart, vm.range === 'day' ? 1 : 0, (vm.deviceType === 'Thermostat'));
                                chart.redraw();
                            }
                            chart.yAxis[1].visibility = vm.range !== 'day';
                        });
                }

                function getChartType() {
					if (vm.deviceType === 'Thermostat') return 'line';
					return 'spline';
                }

                function getChartTitle() {
                    var titlePrefix = vm.deviceType === 'Humidity'
                        ? $.t('Humidity')
                        : $.t('Temperature');

                    if (vm.range === 'day') {
                        return titlePrefix + ' ' + domoticzGlobals.Get5MinuteHistoryDaysGraphTitle();
                    } else if (vm.range === 'month') {
                        return titlePrefix + ' ' + $.t('Last Month');
                    } else if (vm.range === 'year') {
                        return titlePrefix + ' ' + $.t('Last Year');
                    }
                }
            }
        }
    });
});
