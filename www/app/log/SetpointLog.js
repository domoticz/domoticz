define(['app', 'RefreshingChart', 'log/factories'], function (app, RefreshingChart) {
    valSuffix = '\u00B0' + $.myglobals.tempsign;

    app.component('deviceSetpointLog', {
        bindings: {
            device: '<',
        },
        templateUrl: 'app/log/SetpointLog.html',
        controller: function() {
            const $ctrl = this;
            $ctrl.autoRefresh = true;

            $ctrl.$onInit = function() {
                $ctrl.deviceIdx = $ctrl.device.idx;
                $ctrl.deviceType = $ctrl.device.Type;

				if ($ctrl.device.vunit !== undefined) {
					$ctrl.valueUnit = $ctrl.device.vunit;
					valSuffix = $ctrl.device.vunit;
				}
				else {
					$ctrl.valueUnit = ' \u00B0' + $.myglobals.tempsign;
					valSuffix = '\u00B0' + $.myglobals.tempsign;
				}
            }
        }
    });

    app.directive('setpointShortChart', function () {
        return {
            require: {
                logCtrl: '^deviceSetpointLog'
            },
            scope: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            replace: true,
            transclude: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';
                self.sensorType = 'temp';

                self.$onInit = function() {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            true,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                            },
                            [
                                setpointSeriesSupplier(self.device.Type),
                            ]
                        )
                    );
                };
            }
        }
    });

    app.directive('setpointLongChart', function () {
        return {
            require: {
                logCtrl: '^deviceSetpointLog'
            },
            scope: {
                device: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            replace: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.sensorType = 'temp';

                self.$onInit = function() {
                    self.chart = new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParams(
                            domoticzGlobals,
                            self,
                            false,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
                                setpointAverageSeriesSupplier(),
                                setpointRangeSeriesSupplier(),
                            ]
                        )
                    );
                };
            }
        }
    });

    function setpointSeriesSupplier(deviceType) {
        return {
            id: 'setpoint',
            dataItemKeys: ['te'],
            label: 'Te',
            template: {
                name: $.t('Set Point'),
                color: 'orange',
                yAxis: 0,
                step: 'left',
                tooltip: {
                    valueSuffix: ' ' + valSuffix,
                }
            }
        };
    }

    function setpointAverageSeriesSupplier() {
        return {
            id: 'setpoint_avg',
            dataItemKeys: ['ta'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined && dataItem.ta !== undefined;
            },
            label: 'Ta',
            template: {
                name: $.t('Set Point') + ' ' + $.t('Average'),
                color: 'orange',
                fillOpacity: 0.7,
                yAxis: 0,
                zIndex: 2,
                tooltip: {
                    valueSuffix: ' ' + valSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function setpointRangeSeriesSupplier() {
        return {
            id: 'setpoint_range',
            dataItemKeys: ['tm', 'te'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined;
            },
            label: 'Tr',
            template: {
				name: $.t('Set Point') + ' ' + $.t('Range'),
                color: 'rgba(3,190,252,1.0)',
                type: 'areasplinerange',
                linkedTo: ':previous',
                zIndex: 0,
                lineWidth: 0,
                fillOpacity: 0.5,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + valSuffix,
                }
            }
        };
    }

    function baseParams(jquery) {
        return {
            jquery: jquery
        };
    }
    function angularParams(location, route, scope, timeout, element) {
        return {
            location: location,
            route: route,
            scope: scope,
            timeout: timeout,
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
    function chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
        return {
            highchartTemplate: {
                chart: {
                    type: 'line'
                }
            },
            ctrl: ctrl,
            range: ctrl.range,
            device: ctrl.device,
            sensorType: ctrl.sensorType,
            chartName: $.t('Set Point'),
            autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
            dataSupplier: {
                yAxes:
                    [
                        {
                            title: {
                                text: valSuffix
                            },
                            labels: {
                                formatter: function () {
                                    return this.value;
                                }
                            }
                        }
                    ],
                timestampFromDataItem: timestampFromDataItem,
                isShortLogChart: isShortLogChart,
                seriesSuppliers: seriesSuppliers
            }
        };
    }
});
