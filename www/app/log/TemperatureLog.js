define(['app', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart', 'log/factories'], function (app, RefreshingChart, DataLoader, ChartLoader) {

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
                                humiditySeriesSupplier(),
                                chillSeriesSupplier(),
                                setpointSeriesSupplier(),
                                temperatureSeriesSupplier(self.device.Type)
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
                                humiditySeriesSupplier(),
                                chillSeriesSupplier(),
                                chillMinimumSeriesSupplier(),
                                setpointAverageSeriesSupplier(),
                                setpointRangeSeriesSupplier(),
                                setpointPreviousSeriesSupplier(),
                                temperatureAverageSeriesSupplier(self.device.Type),
                                temperatureRangeSeriesSupplier(self.device.Type),
                                temperaturePreviousSeriesSupplier(),
                                temperatureTrendlineSeriesSupplier(self.device.Type)
                            ]
                        )
                    );
                };
            }
        }
    });
	
	changeCompType = function() {
			alert("Change!!");
	}

    app.directive('temperatureCompareChart', function () {
        return {
            require: {
                logCtrl: '^deviceTemperatureLog'
            },
            scope: {
                device: '<',
                degreeType: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '-temp.html'; },
            replace: true,
            bindToController: true,
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
                const self = this;
				self.groupingBy = 'month';
				//self.deviceType = this.device.Type;
				//console.log(self.deviceType);
                self.sensorType = 'temp';
				self.var_name = 'Temp_Avg';
				self.valueSuffix = degreeSuffix;

                self.$onInit = function() {
					let bIsHumidity = (this.device.Type === 'Humidity');
					if (bIsHumidity) {
						self.sensorType = 'hum';
						self.var_name = 'Humidity';
						self.valueSuffix = '%';
					}
					
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
						chart.chartParamsCompare(
							domoticzGlobals,
							self,
							chart.chartParamsCompareTemplate(self, 'Temperature', self.valueSuffix),
                            {
                                isShortLogChart: false,
                                yAxes: [{
											title: {
												text: (!bIsHumidity) ? $.t('Degrees') : $.t('Humidity') + ' ' + self.valueSuffix
											}
										}],
                                extendDataRequest: function (dataRequest) {
                                    dataRequest['groupby'] = self.groupingBy;
									dataRequest['var_name'] = self.var_name;
                                    return dataRequest;
                                },
                                preprocessData: function (data) {
                                    this.firstYear = data.firstYear;
                                    this.categories = categoriesFromGroupingBy.call(this, self.groupingBy);
                                    if (self.chart.chart.xAxis[0].categories === true) {
                                        self.chart.chart.xAxis[0].categories = [];
                                    } else {
                                        self.chart.chart.xAxis[0].categories.length = 0;
                                    }
                                    this.categories.forEach(function (c) {
                                        self.chart.chart.xAxis[0].categories.push(c); });

                                    function categoriesFromGroupingBy(groupingBy) {
                                        if (groupingBy === 'year') {
                                            if (this.firstYear === undefined) {
                                                return [];
                                            }
                                            return _.range(this.firstYear, new Date().getFullYear() + 1).map(year => year.toString());
                                        } else if (groupingBy === 'quarter') {
                                            return ['Q1', 'Q2', 'Q3', 'Q4'];
                                        } else if (groupingBy === 'month') {
                                            return _.range(1, 13).map(month => pad2(month));
                                        }

                                        function pad2(i) {
                                            return (i < 10 ? '0' : '') + i.toString();
                                        }
                                    }
                                },
                            },
                            chart.compareSeriesSuppliers(self)
                        ),
                        new DataLoader(),
                        new ChartLoader($location),
                        null
                    );
                };
            }
        }
    });

    function humiditySeriesSupplier() {
        return {
            id: 'humidity',
            dataItemKeys: ['hu'],
            showWithoutDatapoints: false,
            label: 'Hu',
            template: {
                name: $.t('Humidity'),
                color: 'limegreen',
				type: 'spline',
                yAxis: 1,
                tooltip: {
                    valueSuffix: ' %',
                    valueDecimals: 0
                }
            }
        };
    }

    function chillSeriesSupplier() {
        return {
            id: 'chill',
            dataItemKeys: ['ch'],
            showWithoutDatapoints: false,
            label: 'Ch',
            template: {
                name: $.t('Chill'),
                color: 'red',
                yAxis: 0,
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function setpointSeriesSupplier() {
        return {
            id: 'setpoint',
            dataItemKeys: ['se'],
            showWithoutDatapoints: false,
            label: 'Se',
            template: {
                name: $.t('Set Point'),
                color: 'blue',
                yAxis: 0,
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function temperatureSeriesSupplier(deviceType) {
        return {
            id: 'temperature',
            dataItemKeys: ['te'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            label: 'Te',
            template: {
                name: $.t('Temperature'),
                color: 'yellow',
                yAxis: 0,
                step: deviceType === 'Setpoint' ? 'left' : undefined,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function chillMinimumSeriesSupplier() {
        return {
            id: 'chillmin',
            dataItemKeys: ['cm'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.ch !== undefined;
            },
            showWithoutDatapoints: false,
            label: 'Cm',
            template: {
                name: $.t('Chill') + ' ' + $.t('Minimum'),
                color: 'rgba(255,127,39,0.8)',
                linkedTo: ':previous',
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                },
                yAxis: 0
            }
        };
    }

    function setpointAverageSeriesSupplier() {
        return {
            id: 'setpointavg',
            dataItemKeys: ['se'],
            showWithoutDatapoints: false,
            label: 'Sa',
            template: {
                name: $.t('Set Point') + ' ' + $.t('Average'),
                color: 'blue',
                fillOpacity: 0.7,
                zIndex: 2,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                },
                yAxis: 0
            }
        };
    }

    function setpointRangeSeriesSupplier() {
        return {
            id: 'setpointrange',
            dataItemKeys: ['sm', 'sx'],
            dataItemIsComplete: function (dataItem) {
                return dataItem.se !== undefined;
            },
            showWithoutDatapoints: false,
            label: 'Sr',
            template: {
                name: $.t('Set Point') + ' ' + $.t('Range'),
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
        };
    }

    function setpointPreviousSeriesSupplier() {
        return {
            id: 'prev_setpoint',
            dataItemKeys: ['se'],
            useDataItemsFromPrevious: true,
            showWithoutDatapoints: false,
            label: 'Sp',
            template: {
                name: $.t('Past') + ' ' + $.t('Set Point'),
                color: 'rgba(223,212,246,0.8)',
                zIndex: 3,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                },
                visible: false
            }
        };
    }

    function temperatureAverageSeriesSupplier(deviceType) {
        return {
            id: 'temperature_avg',
            dataItemKeys: ['ta'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined && dataItem.ta !== undefined;
            },
            label: 'Ta',
            template: {
                name: $.t('Temperature') + ' ' + $.t('Average'),
                color: 'yellow',
                fillOpacity: 0.7,
                yAxis: 0,
                zIndex: 2,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                    valueDecimals: 1
                }
            }
        };
    }

    function temperatureRangeSeriesSupplier(deviceType) {
        return {
            id: 'temperature',
            dataItemKeys: ['tm', 'te'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined;
            },
            label: 'Tr',
            template: {
                name: $.t('Temperature') + ' ' + $.t('Range'),
                color: 'rgba(3,190,252,1.0)',
                type: 'areasplinerange',
                linkedTo: ':previous',
                zIndex: 0,
                lineWidth: 0,
                fillOpacity: 0.5,
                yAxis: 0,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                }
            }
        };
    }

    function temperaturePreviousSeriesSupplier() {
        return {
            id: 'prev_temperature',
            dataItemKeys: ['ta'],
            useDataItemsFromPrevious: true,
            showWithoutDatapoints: false,
            label: 'Tp',
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
        };
    }

    function temperatureTrendlineSeriesSupplier(deviceType) {
        return {
            id: 'temp_trendline',
            dataItemKeys: ['ta'],
			showWithoutDatapoints: (deviceType !== 'Humidity'),
            dataItemIsComplete: function (dataItem) {
                return dataItem.te !== undefined && dataItem.ta !== undefined;
            },
            postprocessDatapoints: function (datapoints) {
                const trendline = CalculateTrendLine(datapoints);
                datapoints.length = 0;
                if (trendline !== undefined) {
                    datapoints.push([trendline.x0, trendline.y0]);
                    datapoints.push([trendline.x1, trendline.y1]);
                }
            },
            label: 'Tt',
            template: {
                name: $.t('Trendline') + ' ' + $.t('Temperature'),
                zIndex: 1,
                tooltip: {
                    valueSuffix: ' ' + degreeSuffix,
                },
                color: 'rgba(255,3,3,0.8)',
                dashStyle: 'LongDash',
                yAxis: 0,
                visible: false
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
                    type: ctrl.device.Type === 'Setpoint' ? 'line' : undefined
                }
            },
            ctrl: ctrl,
            range: ctrl.range,
            device: ctrl.device,
            sensorType: ctrl.sensorType,
            chartName:  (ctrl.device.Type === 'Humidity') ? $.t('Humidity') : $.t('Temperature'),
            autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
            dataSupplier: {
                yAxes:
                    [
                        {
							visible: (ctrl.device.Type !== 'Humidity'),
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
                            opposite: (ctrl.device.Type !== 'Humidity')
                        }
                    ],
                timestampFromDataItem: timestampFromDataItem,
                isShortLogChart: isShortLogChart,
                seriesSuppliers: seriesSuppliers
            }
        };
    }
});
