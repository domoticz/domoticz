define(['app', 'RefreshingChart', 'log/factories'], function (app, RefreshingChart) {

    app.component('deviceTemperatureLog', {
        bindings: {
            device: '<',
        },
        templateUrl: 'app/log/TemperatureLog.html',
        controller: function() {
            const $ctrl = this;
            $ctrl.autoRefresh = true;

            $ctrl.$onInit = function() {
                $ctrl.deviceIdx = $ctrl.device.idx;
                $ctrl.deviceType = $ctrl.device.Type;
                $ctrl.degreeType = $.myglobals.tempsign;
            }
        }
    });

    const degreeSuffix = '\u00B0' + $.myglobals.tempsign;

    app.directive('temperatureShortChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<'
            },
            replace: true,
            template: '<div style="height: 300px;"></div>',
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';
                self.sensorType = 'temp';

                self.$onInit = function() {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            domoticzGlobals.Get5MinuteHistoryDaysGraphTitle(),
                            true,
                            function (dataItem, yearOffset=0) {
                                return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'temperature',
                                    name: 'Temperature',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.te !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.te;
                                        }
                                    ],
                                    template: {
                                        color: 'yellow',
                                        yAxis: 0,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'humidity',
                                    name: 'Humidity',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.hu !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.hu;
                                        }
                                    ],
                                    template: {
                                        color: 'lime',
                                        yAxis: 1,
                                        tooltip: {
                                            valueSuffix: ' %',
                                            valueDecimals: 0
                                        }
                                    }
                                }
                            ]
                        )
                    );
                };
            }
        }
    });

    function baseParams(jquery) {
        return {
            jquery: jquery
        };
    }
    function angularParams(location, route, scope, element) {
        return {
            location: location,
            route: route,
            scope: scope,
            element: element
        };
    }
    function domoticzParams(globals, api, datapointApi) {
        return {
            globals: globals,
            api: api,
            datapointApi: datapointApi
        };
    }
    function chartParams(domoticzGlobals, ctrl, chartTitle, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
        return {
            range: ctrl.range,
            device: ctrl.device,
            sensorType: ctrl.sensorType,
            chartTitle: $.t('Temperature') + ' ' + chartTitle,
            autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
            dataSupplier: {
                yAxes:
                    [
                        {
                            title: {
                                text: $.t('Degrees') + ' \u00B0' + ctrl.degreeType
                            },
                            labels: {
                                formatter: function () {
                                    return this.value + '\u00B0 ' + ctrl.degreeType;
                                }
                            }
                        },
                        {
                            title: {
                                text: $.t('Humidity') + ' %'
                            },
                            labels: {
                                formatter: function () {
                                    return this.value + '%';
                                }
                            },
                            showEmpty: false,
                            allowDecimals: false,	//no need to show scale with decimals
                            ceiling: 100,			//max limit for auto zoom, bug in highcharts makes this sometimes not considered.
                            floor: 0,				//min limit for auto zoom
                            minRange: 10,			//min range for auto zoom
                            opposite: true
                        }
                    ],
                timestampFromDataItem: timestampFromDataItem,
                isShortLogChart: isShortLogChart,
                seriesSuppliers: seriesSuppliers
            }
        };
    }

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
            controller: function ($scope, $element, $route, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
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
