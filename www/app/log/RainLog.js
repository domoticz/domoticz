define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart', 'log/CounterLogParams'],
    function (app, _, RefreshingChart, DataLoader, ChartLoader) {

        app.component('deviceRainLog', {
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/RainLog.html',
            controllerAs: '$ctrl',
            controller: function () {
                const $ctrl = this;
                $ctrl.autoRefresh = true;
                $ctrl.showAdvancedCharts = false;
                $ctrl.toggleAdvancedCharts = function () {
                    $ctrl.showAdvancedCharts = !$ctrl.showAdvancedCharts;
                };
            }
        });

        app.component('rainCurrentConditions', {
            require: {
                logCtrl: '^deviceRainLog'
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
                self._deviceCardCount = 0;

                self.$onInit = function () {
                    var device = self.device;
                    var unit = device.getUnit();
                    var valueKey = domoticzGlobals.valueKeyForDevice(device);

                    function buildDeviceCards() {
                        self.cards.length = 0;

                        // Rain Rate card
                        if (device.RainRate !== undefined) {
                            var rainRate = parseFloat(device.RainRate);
                            if (!isNaN(rainRate)) {
                                self.cards.push({
                                    label: $.t('Rain rate'),
                                    value: rainRate.toFixed(1) + ' ' + unit + '/h',
                                    delta: '',
                                    deltaColor: ''
                                });
                            }
                        }

                        // Calculate today's total from graph data (filter to today only)
                        var result = self.logCtrl.dayGraphData;
                        if (result && result.length > 0) {
                            var now = new Date();
                            var todayStr = now.getFullYear() + '-' +
                                String(now.getMonth() + 1).padStart(2, '0') + '-' +
                                String(now.getDate()).padStart(2, '0');
                            var todayTotal = 0;
                            for (var i = 0; i < result.length; i++) {
                                if (result[i].d.substring(0, 10) === todayStr) {
                                    var val = parseFloat(result[i][valueKey]);
                                    if (!isNaN(val)) todayTotal += val;
                                }
                            }
                            self.cards.push({
                                label: $.t('Today'),
                                value: todayTotal.toFixed(1) + ' ' + unit,
                                delta: '',
                                deltaColor: ''
                            });
                        }

                        self._deviceCardCount = self.cards.length;

                        if (self.cards.length === 0) {
                            $element.hide();
                        }
                    }

                    // Rebuild when chart data refreshes (updates today total)
                    $scope.$watch(function () {
                        return self.logCtrl.dayGraphData;
                    }, function (result) {
                        if (result && result.length > 0) {
                            var yearCards = self.cards.splice(self._deviceCardCount);
                            buildDeviceCards();
                            self.cards.push.apply(self.cards, yearCards);
                        }
                    });

                    // Rebuild on live device update
                    $scope.$on('device_update', function (event, updatedDevice) {
                        if (updatedDevice.idx === device.idx) {
                            device = updatedDevice;
                            self.device = updatedDevice;
                            var yearCards = self.cards.splice(self._deviceCardCount);
                            buildDeviceCards();
                            self.cards.push.apply(self.cards, yearCards);
                        }
                    });

                    // Show This Year card when year chart data is available
                    $scope.$watch(function () {
                        return self.logCtrl.yearGraphData;
                    }, function (result) {
                        if (!result || result.length === 0) return;

                        self.cards.splice(self._deviceCardCount);

                        var currentYear = String(new Date().getFullYear());
                        var yearTotal = 0;
                        result.forEach(function (item) {
                            if (item.d.substring(0, 4) === currentYear) {
                                var v = parseFloat(item.mm || 0);
                                if (!isNaN(v)) yearTotal += v;
                            }
                        });

                        self.cards.push({
                            label: $.t('This Year'),
                            value: yearTotal.toFixed(1) + ' ' + unit,
                            delta: '',
                            deltaColor: ''
                        });
                    });
                };
            }
        });

        app.component('rainDayChart', {
            require: {
                logCtrl: '^deviceRainLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-day.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
                const self = this;
                self.range = 'day';

                self.$onInit = function () {
                    var params = chartParamsCol(
                            domoticzGlobals,
                            self,
                            true,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateTimeFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'rain',
                                    valueKeySuffix: '',
                                    template: {
										color: 'rgba(3,190,252,0.8)',
										showInLegend: false,
                                        name: $.t('mm')
                                    }
                                }
                            ]
                        );
                    params.dataSupplier.preprocessData = function (data) {
                        self.logCtrl.dayGraphData = data.result;
                    };
                    new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        params
                    );
                }
            }
        });

        app.component('rainWeekChart', {
            require: {
                logCtrl: '^deviceRainLog'
            },
            bindings: {
                device: '<'
            },
            templateUrl: 'app/log/chart-week.html',
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
                const self = this;
                self.range = 'week';
                self.$onInit = function () {
                    self.chart = new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        chartParamsWeek(
                            domoticzGlobals,
                            self,
                            false,
                            function (dataItem, yearOffset = 0) {
                                return GetLocalDateFromString(dataItem.d, yearOffset);
                            },
                            [
                                {
                                    id: 'mm',
                                    valueKeySuffix: '',
                                    template: {
										color: 'rgba(3,190,252,0.8)',
                                        name: $.t('mm')
                                    }
                                }
                            ]
                        )
                    );
                }
            }
        });

        app.component('rainLongChart', {
            require: {
                logCtrl: '^deviceRainLog'
            },
            bindings: {
                device: '<',
                range: '@'
            },
            templateUrl: function($element, $attrs) { return 'app/log/chart-' + $attrs.range + '.html'; },
            controllerAs: 'vm',
            controller: function ($location, $route, $scope, $timeout, $element, domoticzGlobals, domoticzApi, domoticzDataPointApi, chart) {
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
                                id: 'mm',
                                valueKeySuffix: '',
                                template: {
									color: 'rgba(3,190,252,0.8)',
                                    name: $.t('mm')
                                }
                            }
                        ]
                    );
                    if (self.range === 'year') {
                        params.dataSupplier.preprocessData = function (data) {
                            self.logCtrl.yearGraphData = data.result;
                        };
                    }
                    new RefreshingChart(
                        chart.baseParams($),
                        chart.angularParams($location, $route, $scope, $timeout, $element),
                        chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
                        params
                    );
                }
            }
        });

	var customChartTemplate =
		'<div class="chart noselect">' +
			'<div class="chart-title-center">' +
				'<div class="chart-title-container"><h2>{{vm.chartTitle}}</h2></div>' +
			'</div>' +
			'<div class="chartarea">' +
				'<div class="chartcontainer" style="height:300px;"></div>' +
			'</div>' +
		'</div>';

	app.component('rainCumulativeChart', {
		require: {
			logCtrl: '^deviceRainLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi, domoticzGlobals) {
			const self = this;
			self.chartTitle = $.t('Cumulative Rainfall') + ' ' + Get5MinuteHistoryDaysGraphTitle();

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'rain', idx: self.device.idx, range: 'day'
				}).then(function (data) {
					if (!data || !data.result || data.result.length === 0) {
						$element.hide();
						return;
					}

					var items = data.result;
					var unit = self.device.getUnit();
					var valueKey = domoticzGlobals.valueKeyForDevice(self.device);
					var cumulative = 0;
					var seriesData = [];

					for (var i = 0; i < items.length; i++) {
						var val = parseFloat(items[i][valueKey]);
						if (!isNaN(val)) {
							cumulative += val;
						}
						seriesData.push([
							GetLocalDateTimeFromString(items[i].d),
							Math.round(cumulative * 10) / 10
						]);
					}

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-rain-cumulative');
					Highcharts.chart(chartElement[0], {
						chart: { type: 'area', zoomType: 'x' },
						title: { text: '' },
						xAxis: {
							type: 'datetime'
						},
						yAxis: {
							title: { text: $.t('Cumulative') + ' (' + unit + ')' },
							min: 0
						},
						tooltip: {
							xDateFormat: '%A %Y-%m-%d %H:%M',
							valueSuffix: ' ' + unit,
							shared: true
						},
						legend: { enabled: false },
						series: [{
							name: $.t('Cumulative Rainfall'),
							data: seriesData,
							color: 'rgba(3,190,252,0.8)',
							fillColor: {
								linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
								stops: [
									[0, 'rgba(3,190,252,0.4)'],
									[1, 'rgba(3,190,252,0.05)']
								]
							},
							lineWidth: 2,
							marker: { enabled: false }
						}]
					});
				});
			};
		}
	});

	app.component('rainRateChart', {
		require: {
			logCtrl: '^deviceRainLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi, domoticzGlobals) {
			const self = this;
			self.chartTitle = $.t('Rain rate') + ' ' + Get5MinuteHistoryDaysGraphTitle();

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'rain', idx: self.device.idx, range: 'day'
				}).then(function (data) {
					if (!data || !data.result || data.result.length < 2) {
						$element.hide();
						return;
					}

					var items = data.result;
					var unit = self.device.getUnit();
					var valueKey = domoticzGlobals.valueKeyForDevice(self.device);
					var rateData = [];

					for (var i = 1; i < items.length; i++) {
						var rain = parseFloat(items[i][valueKey]);
						var ts = GetLocalDateTimeFromString(items[i].d);
						var tsPrev = GetLocalDateTimeFromString(items[i - 1].d);
						var dtHours = (ts - tsPrev) / 3600000;

						if (!isNaN(rain) && dtHours > 0) {
							var rate = rain / dtHours;
							rateData.push([ts, Math.round(rate * 10) / 10]);
						}
					}

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-rain-rate');
					Highcharts.chart(chartElement[0], {
						chart: { type: 'spline', zoomType: 'x' },
						title: { text: '' },
						xAxis: {
							type: 'datetime'
						},
						yAxis: {
							title: { text: $.t('Rain rate') + ' (' + unit + '/h)' },
							min: 0
						},
						tooltip: {
							xDateFormat: '%A %Y-%m-%d %H:%M',
							valueSuffix: ' ' + unit + '/h',
							shared: true
						},
						legend: { enabled: false },
						plotOptions: {
							spline: {
								lineWidth: 2,
								marker: { enabled: false }
							}
						},
						series: [{
							name: $.t('Rain rate'),
							data: rateData,
							color: 'rgba(3,190,252,0.8)'
						}]
					});
				});
			};
		}
	});

	app.component('rainIntensityChart', {
		require: {
			logCtrl: '^deviceRainLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi, domoticzGlobals) {
			const self = this;
			self.chartTitle = $.t('Rain Intensity Distribution') + ' ' + Get5MinuteHistoryDaysGraphTitle();

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'rain', idx: self.device.idx, range: 'day'
				}).then(function (data) {
					if (!data || !data.result || data.result.length < 2) {
						$element.hide();
						return;
					}

					var items = data.result;
					var unit = self.device.getUnit();
					var valueKey = domoticzGlobals.valueKeyForDevice(self.device);

					// Calculate rain rates (mm/h) from consecutive data points
					var rates = [];
					for (var i = 1; i < items.length; i++) {
						var rain = parseFloat(items[i][valueKey]);
						var ts = GetLocalDateTimeFromString(items[i].d);
						var tsPrev = GetLocalDateTimeFromString(items[i - 1].d);
						var dtHours = (ts - tsPrev) / 3600000;
						if (!isNaN(rain) && dtHours > 0) {
							var rate = rain / dtHours;
							if (rate > 0) {
								rates.push(rate);
							}
						}
					}

					if (rates.length === 0) {
						$element.hide();
						return;
					}

					// Standard meteorological intensity categories (mm/h)
					var bins = [
						{ label: $.t('Light') + '\n(0-2.5)',   min: 0,    max: 2.5  },
						{ label: $.t('Moderate') + '\n(2.5-7.5)', min: 2.5,  max: 7.5  },
						{ label: $.t('Heavy') + '\n(7.5-50)',  min: 7.5,  max: 50   },
						{ label: $.t('Violent') + '\n(50+)',   min: 50,   max: Infinity }
					];

					var counts = new Array(bins.length).fill(0);
					rates.forEach(function (rate) {
						for (var b = bins.length - 1; b >= 0; b--) {
							if (rate >= bins[b].min) {
								counts[b]++;
								break;
							}
						}
					});

					var categories = bins.map(function (b) { return b.label; });
					var pctData = counts.map(function (c) {
						return Math.round(c / rates.length * 10000) / 100;
					});
					var colors = [
						'rgba(3,190,252,0.8)',    // light - blue
						'rgba(52,152,219,0.8)',   // moderate - darker blue
						'rgba(255,165,0,0.8)',    // heavy - orange
						'rgba(255,80,80,0.8)'     // violent - red
					];

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-rain-intensity');
					self.chartTitle += ' (' + rates.length + ' ' + $.t('rain intervals') + ')';

					Highcharts.chart(chartElement[0], {
						chart: { type: 'column' },
						title: { text: '' },
						xAxis: {
							categories: categories,
							crosshair: true,
							labels: {
								style: { whiteSpace: 'pre-line', textAlign: 'center' }
							}
						},
						yAxis: {
							min: 0,
							title: { text: $.t('Frequency') + ' (%)' }
						},
						tooltip: {
							headerFormat: '<b>{point.key}</b><br/>',
							pointFormat: '{point.y:.1f}% ({point.count} intervals)',
							shared: false
						},
						plotOptions: {
							column: {
								groupPadding: 0.1,
								pointPadding: 0.05,
								borderWidth: 1,
								colorByPoint: true,
								colors: colors
							}
						},
						legend: { enabled: false },
						series: [{
							name: $.t('Frequency'),
							data: pctData.map(function (pct, idx) {
								return { y: pct, count: counts[idx] };
							})
						}]
					});
				});
			};
		}
	});

		app.directive('rainCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceRainLog'
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

        function chartParamsCol(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
            return _.merge({
					highchartTemplate: {
						chart: {
							type: 'column',
							zoomType: false,
							marginRight: 10
						},
						plotOptions: {
							column: {
								pointPlacement: 0,
								stacking: undefined,
								dataLabels: {
									enabled: false,
									color: 'white'
								}
							},
							series: {
								// colorByPoint: true
								stacking: undefined
							}
						},
						tooltip: {
							shared: false,
							crosshairs: false
						}
					}
				}, chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers)
			);
        }

        function chartParamsWeek(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
            return _.merge({
                    highchartTemplate: {
                        chart: {
                            type: 'column',
                            zoomType: false,
                            marginRight: 10
                        },
                        xAxis: {
                            dateTimeLabelFormats: {
                                day: '%a'
                            },
                            tickInterval: 24 * 3600 * 1000
                        },
                        plotOptions: {
                            column: {
                                pointPlacement: 0,
                                stacking: undefined,
								dataLabels: {
									enabled: true,
									color: 'white'
								}
                            },
                            series: {
                                // colorByPoint: true
                                stacking: undefined
                            }
                        },
                        tooltip: {
                            shared: false,
                            crosshairs: false
                        }
                    }
				}, chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers)
			);
        }
		
    }
);
