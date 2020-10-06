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
            templateUrl: 'app/log/chart-day.html',
            replace: true,
            transclude: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.chartTitle = domoticzGlobals.Get5MinuteHistoryDaysGraphTitle();
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
                                        color: 'limegreen',
                                        yAxis: 1,
                                        tooltip: {
                                            valueSuffix: ' %',
                                            valueDecimals: 0
                                        }
                                    }
                                },
                                {
                                    id: 'chill',
                                    name: 'Chill',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.ch !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.ch;
                                        }
                                    ],
                                    template: {
                                        color: 'red',
                                        yAxis: 0,
                                        zIndex: 1,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'setpoint',
                                    name: 'Set Point',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.se !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.se;
                                        }
                                    ],
                                    template: {
                                        color: 'blue',
                                        yAxis: 0,
                                        zIndex: 1,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
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
                                        step: self.device.Type === 'Thermostat' ? 'left' : undefined,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
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

    app.directive('temperatureLongChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<',
                chartTitle: '@',
                range: '@'
            },
            replace: true,
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.sensorType = 'temp';

                self.$onInit = function() {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            $.t(self.chartTitle),
                            false,
                            function (dataItem, yearOffset=0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
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
                                        color: 'limegreen',
                                        yAxis: 1,
                                        tooltip: {
                                            valueSuffix: ' %',
                                            valueDecimals: 0
                                        }
                                    }
                                },
                                {
                                    id: 'chill',
                                    name: 'Chill',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.ch !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.ch;
                                        }
                                    ],
                                    template: {
                                        color: 'red',
                                        yAxis: 0,
                                        zIndex: 1,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'chillmin',
                                    name: 'Chill_min',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.ch !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.cm;
                                        }
                                    ],
                                    template: {
                                        color: 'rgba(255,127,39,0.8)',
                                        linkedTo: ':previous',
                                        zIndex: 1,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        },
                                        yAxis: 0
                                    }
                                },
                                {
                                    id: 'setpointavg',
                                    name: $.t('Set Point') + '_avg',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.se !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.se;
                                        }
                                    ],
                                    template: {
                                        color: 'blue',
                                        fillOpacity: 0.7,
                                        zIndex: 2,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        },
                                        yAxis: 0
                                    }
                                },
                                {
                                    id: 'setpointrange',
                                    name: $.t('Set Point') + '_range',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.se !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.sm;
                                        },
                                        function (dataItem) {
                                            return dataItem.sx;
                                        },
                                    ],
                                    template: {
                                        color: 'rgba(164,75,148,1.0)',
                                        type: 'areasplinerange',
                                        linkedTo: ':previous',
                                        zIndex: 1,
                                        lineWidth: 0,
                                        fillOpacity: 0.5,
                                        yAxis: 0,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'prev_setpoint',
                                    name: $.t('Past') + ' ' + $.t('Set Point'),
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.sm !== undefined && dataItem.sx !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.sm;
                                        },
                                        function (dataItem) {
                                            return dataItem.sx;
                                        },
                                    ],
                                    useDataItemFromPrevious: true,
                                    template: {
                                        color: 'rgba(223,212,246,0.8)',
                                        zIndex: 3,
                                        yAxis: 0,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        },
                                        visible: false
                                    }
                                },
                                {
                                    id: 'temperature_avg',
                                    name: 'Temperature',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.te !== undefined && dataItem.ta !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.ta;
                                        }
                                    ],
                                    template: {
                                        color: 'yellow',
                                        fillOpacity: 0.7,
                                        yAxis: 0,
                                        zIndex: 2,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'temperature',
                                    name: 'Temperature range',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.te !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.tm;
                                        },
                                        function (dataItem) {
                                            return dataItem.te;
                                        }
                                    ],
                                    template: {
                                        color: 'rgba(3,190,252,1.0)',
                                        type: 'areasplinerange',
                                        linkedTo: ':previous',
                                        zIndex: 0,
                                        lineWidth: 0,
                                        fillOpacity: 0.5,
                                        yAxis: 0,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        }
                                    }
                                },
                                {
                                    id: 'prev_temperature',
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.te !== undefined && dataItem.ta !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.tm;
                                        },
                                        function (dataItem) {
                                            return dataItem.te;
                                        }
                                    ],
                                    useDataItemFromPrevious: true,
                                    template: {
                                        name: $.t('Past') + ' ' + $.t('Temperature'),
                                        color: 'rgba(224,224,230,0.8)',
                                        zIndex: 3,
                                        yAxis: 0,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        },
                                        visible: false
                                    }
                                },
                                {
                                    id: 'temp_trendline',
                                    name: $.t('Trendline') + ' ' + $.t('Temperature'),
                                    dataItemIsValid: function (dataItem) {
                                        return dataItem.te !== undefined && dataItem.ta !== undefined;
                                    },
                                    valuesFromDataItem: [
                                        function (dataItem) {
                                            return dataItem.ta;
                                        }
                                    ],
                                    aggregateDataItems: function (datapoints, dataItems) {
                                        const trendline = CalculateTrendLine(datapoints);
                                        datapoints.length = 0;
                                        if (trendline !== undefined) {
                                            datapoints.push([trendline.x0, trendline.y0]);
                                            datapoints.push([trendline.x1, trendline.y1]);
                                        }
                                    },
                                    template: {
                                        zIndex: 1,
                                        tooltip: {
                                            valueSuffix: ' ' + degreeSuffix,
                                            valueDecimals: 1
                                        },
                                        color: 'rgba(255,3,3,0.8)',
                                        dashStyle: 'LongDash',
                                        yAxis: 0,
                                        visible: false
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
            chartType: ctrl.device.Type === 'Thermostat' ? 'line' : undefined,
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
});
