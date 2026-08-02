define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader'],
    function (app, _, RefreshingChart, DataLoader, ChartLoader) {

        app.component('deviceGraphLog', {
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/GraphLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
            }
        });

        app.component('graphCurrentConditions', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<'
            },
            template:
                '<div class="chart noselect current-conditions" style="padding: 10px 0;">' +
                    '<div style="display:flex; justify-content:center; flex-wrap:wrap; gap:20px;">' +
                        '<div ng-repeat="card in vm.cards" style="text-align:center; min-width:140px; padding:12px 20px; ' +
                            'background:rgba(255,255,255,0.05); border-radius:8px;">' +
                            '<div style="font-size:0.85em; opacity:0.7;">{{card.label}}</div>' +
                            '<div style="font-size:1.8em; font-weight:bold;">{{card.value}}</div>' +
                            '<div style="font-size:0.85em;" ng-style="{color: card.deltaColor}">{{card.delta}}</div>' +
                        '</div>' +
                    '</div>' +
                '</div>',
            controllerAs: 'vm',
            controller: function ($element, $scope, domoticzGlobals) {
                const self = this;
                self.cards = [];

                self.$onInit = function () {
                    var device = self.device;
                    var valueKey = domoticzGlobals.valueKeyForDevice(device);
                    var deviceUnit = device.getUnit();

                    function rebuildCards() {
                        self.cards.length = 0;

                        // Parse the device's current value so Min/Max use the same precision and unit
                        var dataStr = String(device.Data);
                        var dataMatch = dataStr.match(/(-?\d+(?:\.(\d+))?)\s*(.*)/);
                        var decimals = (dataMatch && dataMatch[2]) ? dataMatch[2].length : 0;
                        var unit = (dataMatch && dataMatch[3]) ? dataMatch[3] : deviceUnit;

                        // Current value from device.Data
                        if (device.Data !== undefined && device.Data !== null && device.Data !== '') {
                            self.cards.push({
                                label: $.t('Current'),
                                value: dataStr,
                                delta: '',
                                deltaColor: ''
                            });
                        }

                        var min = Infinity, max = -Infinity;

                        // Prefer the historical range from the long-log (month) data, which carries
                        // per-period min/max. This reflects the full visible history rather than just
                        // today's short-log, which for infrequently updated sensors holds a single value.
                        var longData = self.logCtrl.longGraphData;
                        if (longData && longData.length > 0) {
                            longData.forEach(function (item) {
                                var vMin = parseFloat(item[valueKey + '_min']);
                                var vMax = parseFloat(item[valueKey + '_max']);
                                if (!isNaN(vMin) && vMin < min) min = vMin;
                                if (!isNaN(vMax) && vMax > max) max = vMax;
                            });
                        }

                        // Also fold in today's short-log raw values
                        var dayData = self.logCtrl.dayGraphData;
                        if (dayData && dayData.length > 0) {
                            dayData.forEach(function (item) {
                                var v = parseFloat(item[valueKey]);
                                if (!isNaN(v)) {
                                    if (v < min) min = v;
                                    if (v > max) max = v;
                                }
                            });
                        }

                        // And the live current value
                        if (dataMatch) {
                            var cur = parseFloat(dataMatch[1]);
                            if (!isNaN(cur)) {
                                if (cur < min) min = cur;
                                if (cur > max) max = cur;
                            }
                        }

                        if (min !== Infinity) {
                            self.cards.push({
                                label: $.t('Minimum'),
                                value: min.toFixed(decimals) + ' ' + unit,
                                delta: '',
                                deltaColor: ''
                            });
                            self.cards.push({
                                label: $.t('Maximum'),
                                value: max.toFixed(decimals) + ' ' + unit,
                                delta: '',
                                deltaColor: ''
                            });
                        }

                        if (self.cards.length === 0) {
                            $element.hide();
                        }
                    }

                    // Rebuild when either the short-log (day) or long-log (month) data refreshes
                    $scope.$watchGroup([
                        function () { return self.logCtrl.dayGraphData; },
                        function () { return self.logCtrl.longGraphData; }
                    ], function () {
                        rebuildCards();
                    });

                    // Rebuild on live device update
                    $scope.$on('device_update', function (event, updatedDevice) {
                        if (updatedDevice.idx === device.idx) {
                            device = updatedDevice;
                            self.device = updatedDevice;
                            rebuildCards();
                        }
                    });
                };
            }
        });

        app.component('deviceShortChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    var params = chartParams(
                        domoticzGlobals,
                        self,
                        true,
                        function (dataItem, yearOffset = 0) {
                            return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                        },
                        [
                            {
                                id: 'power',
                                valueKeySuffix: '',
                                colorIndex: 1,
                                template: {
                                    name: domoticzGlobals.sensorNameForDevice(self.device),
                                    showInLegend: false
                                }
                            }
                        ]
                    );
                    // Share day data with conditions component
                    params.dataSupplier.preprocessData = function (data) {
                        self.logCtrl.dayGraphData = data.result;
                    };
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        params
                    );
                }
            }
        });

        app.component('deviceLongChart', {
            require: {
                logCtrl: '^deviceGraphLog'
            },
            bindings: {
                device: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi) {
                const self = this;

                self.$onInit = function () {
                    var params = chartParams(
                        domoticzGlobals,
                        self,
                        false,
                        function (dataItem, yearOffset = 0) {
                            return GetLocalDateFromString(dataItem.d, yearOffset);
                        },
                        [
                            {
                                id: 'min',
                                valueKeySuffix: '_min',
                                colorIndex: 3,
                                template: {
                                    name: $.t('Minimum')
                                }
                            },
                            {
                                id: 'max',
                                valueKeySuffix: '_max',
                                colorIndex: 2,
                                template: {
                                    name: $.t('Maximum')
                                }
                            },
                            {
                                id: 'avg',
                                valueKeySuffix: '_avg',
                                colorIndex: 0,
                                template: {
                                    name: $.t('Average')
                                }
                            }
                        ]
                    );
                    // Share month history with the current-conditions cards so Min/Max reflect
                    // the full visible range, not just today's short-log
                    if (self.range === 'month') {
                        params.dataSupplier.preprocessData = function (data) {
                            self.logCtrl.longGraphData = data.result;
                        };
                    }
                    new RefreshingChart(
                        baseParams($),
                        angularParams($location, $route, $scope, $timeout, $element),
                        domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        params
                    );
                }
            }
        });

		app.directive('deviceCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceGraphLog'
				},
				scope: {
					device: '<',
					range: '@'
				},
				templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
				replace: true,
				bindToController: true,
				controllerAs: 'vm',
				controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
					const self = this;

					self.$onInit = function() {
						self.groupingBy = 'month';
						self.sensorType = domoticzGlobals.sensorTypeForDevice(self.device);
						self.chart = new RefreshingChart(
							chart.baseParams($),
							chart.angularParams($location, $route, $scope, $timeout, $element),
							chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
							chart.chartParamsCompare(
								domoticzGlobals,
								self,
								chart.chartParamsCompareTemplate(self, self.device.Name, self.device.getUnit()),
								{
									isShortLogChart: false,
									yAxes: [{
												title: {
													text: domoticzGlobals.axisTitleForDevice(self.device)
												}
											}],
									extendDataRequest: function (dataRequest) {
										dataRequest['groupby'] = self.groupingBy;
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
                ctrl: ctrl,
                range: ctrl.range,
                device: ctrl.device,
				sensorType: domoticzGlobals.sensorTypeForDevice(ctrl.device),
                autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
                dataSupplier: {
                    yAxes: [
                        {
                            title: {
                                text: domoticzGlobals.axisTitleForDevice(ctrl.device)
                            },
                            labels: {
                                formatter: function () {
                                    return ctrl.device.getUnit() === 'vM' ? Highcharts.numberFormat(this.value, 0) : this.value;
                                }
                            }
                        }
                    ],
                    valueSuffix: ' ' + ctrl.device.getUnit(),
                    timestampFromDataItem: timestampFromDataItem,
                    isShortLogChart: isShortLogChart,
                    seriesSuppliers: seriesSuppliers.map(function (seriesSupplier) {
                        return _.merge(
                            {
                                dataItemKeys: [domoticzGlobals.valueKeyForDevice(ctrl.device) + seriesSupplier.valueKeySuffix]
                            },
                            seriesSupplier
                        );
                    })
                }
            };
        }
    }
);
