define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart'],
	function (app, _, RefreshingChart, DataLoader, ChartLoader) {

		app.component('deviceBarometerLog', {
			bindings: {
				device: '<'
			},
			templateUrl: 'app/log/BarometerLog.html',
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

		app.component('barometerShortChart', {
			require: {
				logCtrl: '^deviceBarometerLog'
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
					var params = chartParams(
						domoticzGlobals,
						self,
						true,
						function (dataItem, yearOffset = 0) {
							return GetLocalDateTimeFromString(dataItem.d, yearOffset);
						},
						[
							{
								id: 'ba',
								valueKeySuffix: '',
								template: {
									name: $.t('Barometer'),
									showInLegend: false,
									color: 'rgba(3,190,252,0.8)'
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

		app.component('barometerLongChart', {
			require: {
				logCtrl: '^deviceBarometerLog'
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
					new RefreshingChart(
						chart.baseParams($),
						chart.angularParams($location, $route, $scope, $timeout, $element),
						chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
						chartParams(
							domoticzGlobals,
							self,
							false,
							function (dataItem, yearOffset = 0) {
								return GetLocalDateFromString(dataItem.d, yearOffset);
							},
							[
								{
									id: 'ba',
									valueKeySuffix: '',
									template: {
										name: $.t('Barometer'),
										showInLegend: false,
										color: 'rgba(3,190,252,0.8)'
									}
								}
							]
						)
					);
				}
			}
		});

		app.directive('barometerCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceBarometerLog'
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
						self.sensorType = 'temp';
						self.chart = new RefreshingChart(
							chart.baseParams($),
							chart.angularParams($location, $route, $scope, $timeout, $element),
							chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
							chart.chartParamsCompare(
								domoticzGlobals,
								self,
								_.merge(chart.chartParamsCompareTemplate(self, self.device.Name, 'hPa'), {
								highchartTemplate: {
									chart: { type: 'spline' },
									plotOptions: {
										spline: {
											marker: { enabled: true, radius: 3 },
											lineWidth: 2
										}
									}
								}
							}),
								{
									isShortLogChart: false,
									yAxes: [{
										title: {
											text: $.t('Pressure') + ' (hPa)'
										}
									}],
									extendDataRequest: function (dataRequest) {
										dataRequest['groupby'] = self.groupingBy;
										dataRequest['var_name'] = 'Barometer';
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

	var baroDegreeSuffix = '\u00B0' + $.myglobals.tempsign;

	app.component('barometerCurrentConditions', {
		require: {
			logCtrl: '^deviceBarometerLog'
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
		controller: function ($element, $scope) {
			const self = this;
			self.cards = [];

			self.$onInit = function () {
				var device = self.device;

				function formatDelta(current, previous, suffix, decimals) {
					if (isNaN(current) || isNaN(previous)) return '';
					var d = current - previous;
					var sign = d >= 0 ? '+' : '';
					var ret = sign + d.toFixed(decimals) + ' ' + suffix;

					// EU ., replacements
					if ($.myglobals.EUNumberFormat) {
						ret = formatEUValue(ret);
					}

					return ret;
				}

				function deltaColor(current, previous) {
					if (isNaN(current) || isNaN(previous)) return '';
					var d = current - previous;
					if (Math.abs(d) < 0.1) return '#aaa';
					return d > 0 ? '#ff6b6b' : '#4ecdc4';
				}

				function find24hAgo(items) {
					var latest = items[items.length - 1];
					var target24h = GetLocalDateTimeFromString(latest.d) - 24 * 3600000;
					var closest = items[0];
					var closestDiff = Math.abs(GetLocalDateTimeFromString(items[0].d) - target24h);
					for (var i = 1; i < items.length; i++) {
						var diff = Math.abs(GetLocalDateTimeFromString(items[i].d) - target24h);
						if (diff < closestDiff) {
							closestDiff = diff;
							closest = items[i];
						}
					}
					return closest;
				}

				function rebuildCards() {
					self.cards.length = 0;
					var items = self.logCtrl.dayGraphData;
					var closest24h = (items && items.length >= 2) ? find24hAgo(items) : null;

					// Barometer card
					if (device.Barometer !== undefined) {
						var ba = parseFloat(device.Barometer);
						var ba24 = closest24h ? parseFloat(closest24h.ba) : NaN;
						var cardValue = ba.toFixed(1) + ' hPa';

						// EU ., replacements
						if ($.myglobals.EUNumberFormat) {
							cardValue = formatEUValue(cardValue);
						}

						self.cards.push({
							label: $.t('Barometer'),
							value: cardValue,
							delta: formatDelta(ba, ba24, 'hPa', 1),
							deltaColor: deltaColor(ba, ba24)
						});
					}

					// Temperature card
					if (device.Temp !== undefined) {
						var te = parseFloat(device.Temp);
						var te24 = closest24h ? parseFloat(closest24h.te) : NaN;
						var cardValue = te.toFixed(1) + ' ' + baroDegreeSuffix;

						// EU ., replacements
						if ($.myglobals.EUNumberFormat) {
							cardValue = formatEUValue(cardValue);
						}

						if (!isNaN(te)) {
							self.cards.push({
								label: $.t('Temperature'),
								value: cardValue,
								delta: formatDelta(te, te24, baroDegreeSuffix, 1),
								deltaColor: deltaColor(te, te24)
							});
						}
					}

					// Humidity card
					if (device.Humidity !== undefined) {
						var hu = parseInt(device.Humidity, 10);
						var hu24 = closest24h ? parseInt(closest24h.hu, 10) : NaN;
						self.cards.push({
							label: $.t('Humidity'),
							value: hu + ' %',
							delta: formatDelta(hu, hu24, '%', 0),
							deltaColor: deltaColor(hu, hu24)
						});
					}

					if (self.cards.length === 0) {
						$element.hide();
					}
				}

				// Rebuild when chart data refreshes (updates deltas)
				$scope.$watch(function () {
					return self.logCtrl.dayGraphData;
				}, function (result) {
					if (result && result.length >= 2) {
						rebuildCards();
					}
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

		// Shared inline template for the custom day-range charts
	var customChartTemplate =
		'<div class="chart noselect">' +
			'<div class="chart-title-center">' +
				'<div class="chart-title-container"><h2>{{vm.chartTitle}}</h2></div>' +
			'</div>' +
			'<div class="chartarea">' +
				'<div class="chartcontainer" style="height:300px;"></div>' +
			'</div>' +
		'</div>';

	// Chart 1: Pressure Change Rate
	app.component('barometerChangeRateChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Pressure Change Rate');

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length < 2) {
						return;
					}

					var positiveData = [];
					var negativeData = [];

					for (var i = 1; i < data.result.length; i++) {
						var prev = data.result[i - 1];
						var curr = data.result[i];
						var prevBa = parseFloat(prev.ba);
						var currBa = parseFloat(curr.ba);

						if (isNaN(prevBa) || isNaN(currBa)) {
							continue;
						}

						var prevTime = GetLocalDateTimeFromString(prev.d);
						var currTime = GetLocalDateTimeFromString(curr.d);
						var hoursDiff = (currTime - prevTime) / 3600000;

						if (hoursDiff <= 0) {
							continue;
						}

						var rate = (currBa - prevBa) / hoursDiff;
						var ts = GetLocalDateTimeFromString(curr.d);

						if (rate >= 0) {
							positiveData.push([ts, Math.round(rate * 100) / 100]);
							negativeData.push([ts, null]);
						} else {
							positiveData.push([ts, null]);
							negativeData.push([ts, Math.round(rate * 100) / 100]);
						}
					}

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-change-rate');
					Highcharts.chart(chartElement[0], {
						chart: {
							type: 'column',
							zoomType: 'x'
						},
						title: {
							text: null
						},
						xAxis: {
							type: 'datetime',
							crosshair: true
						},
						yAxis: {
							title: {
								text: $.t('Pressure Change') + ' (hPa/hr)'
							},
							plotLines: [
								{
									value: 1,
									color: 'rgba(0,180,0,0.4)',
									width: 1,
									dashStyle: 'Dash',
									label: { text: '+1 hPa/hr', align: 'right', style: { color: 'rgba(0,180,0,0.7)' } }
								},
								{
									value: -1,
									color: 'rgba(220,0,0,0.4)',
									width: 1,
									dashStyle: 'Dash',
									label: { text: '-1 hPa/hr', align: 'right', style: { color: 'rgba(220,0,0,0.7)' } }
								}
							]
						},
						tooltip: {
							shared: false,
							formatter: function () {
								return Highcharts.dateFormat('%Y-%m-%d %H:%M', this.x) + '<br/>' +
									this.series.name + ': <b>' + this.y + ' hPa/hr</b>';
							}
						},
						legend: {
							enabled: true
						},
						plotOptions: {
							column: {
								groupPadding: 0,
								pointPadding: 0,
								borderWidth: 0,
								grouping: false
							}
						},
						series: [
							{
								id: 'rising',
								name: $.t('Rising'),
								color: 'rgba(0,180,0,0.8)',
								data: positiveData
							},
							{
								id: 'falling',
								name: $.t('Falling'),
								color: 'rgba(220,0,0,0.8)',
								data: negativeData
							}
						]
					});
				});
			};
		}
	});

	// Chart 2: Multi-Axis Weather Overview
	app.component('barometerWeatherOverviewChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Weather Overview');

			self.$onInit = function () {
				var hasTemp = self.device.Temp !== undefined;
				var hasHum = self.device.Humidity !== undefined;

				if (!hasTemp && !hasHum) {
					$element.hide();
					return;
				}

				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length === 0) {
						$element.hide();
						return;
					}

					var baData = [];
					var teData = [];
					var huData = [];

					data.result.forEach(function (item) {
						var ts = GetLocalDateTimeFromString(item.d);
						var ba = parseFloat(item.ba);
						if (!isNaN(ba)) {
							baData.push([ts, ba]);
						}
						if (hasTemp && item.te !== undefined) {
							teData.push([ts, parseFloat(item.te)]);
						}
						if (hasHum && item.hu !== undefined) {
							huData.push([ts, parseInt(item.hu, 10)]);
						}
					});

					var yAxes = [
						{
							title: { text: $.t('Pressure') + ' (hPa)' },
							opposite: false
						}
					];
					if (hasTemp) {
						yAxes.push({
							title: { text: $.t('Temperature') + ' (\u00b0C)' },
							opposite: true
						});
					}
					if (hasHum) {
						yAxes.push({
							title: { text: $.t('Humidity') + ' (%)' },
							opposite: true
						});
					}

					var series = [
						{
							id: 'pressure',
							name: $.t('Pressure'),
							type: 'spline',
							yAxis: 0,
							color: 'rgba(3,190,252,0.9)',
							data: baData,
							tooltip: { valueSuffix: ' hPa' },
							marker: { enabled: false }
						}
					];

					if (hasTemp) {
						series.push({
							id: 'temperature',
							name: $.t('Temperature'),
							type: 'spline',
							yAxis: 1,
							color: 'rgba(255,80,80,0.9)',
							data: teData,
							tooltip: { valueSuffix: ' \u00b0C' },
							marker: { enabled: false }
						});
					}

					if (hasHum) {
						series.push({
							id: 'humidity',
							name: $.t('Humidity'),
							type: 'spline',
							yAxis: hasTemp ? 2 : 1,
							color: 'rgba(0,180,0,0.9)',
							data: huData,
							tooltip: { valueSuffix: ' %' },
							marker: { enabled: false }
						});
					}

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-weather-overview');
					Highcharts.chart(chartElement[0], {
						chart: {
							zoomType: 'x'
						},
						title: {
							text: null
						},
						xAxis: {
							type: 'datetime',
							crosshair: true
						},
						yAxis: yAxes,
						tooltip: {
							shared: true,
							crosshairs: true
						},
						legend: {
							enabled: true
						},
						series: series
					});
				});
			};
		}
	});

	// Chart 3: Pressure vs Temperature Scatter Plot
	app.component('barometerVsTemperatureChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Pressure vs Temperature');

			self.$onInit = function () {
				if (self.device.Temp === undefined) {
					$element.hide();
					return;
				}

				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length === 0) {
						$element.hide();
						return;
					}

					var n = data.result.length;
					var scatterData = [];

					data.result.forEach(function (item, idx) {
						var ba = parseFloat(item.ba);
						var te = item.te !== undefined ? parseFloat(item.te) : NaN;
						if (isNaN(ba) || isNaN(te)) {
							return;
						}
						// Gradient: older points are lighter (higher lightness), newer are darker
						var ratio = idx / Math.max(n - 1, 1);
						var lightness = Math.round(80 - ratio * 55); // 80% (light) → 25% (dark)
						scatterData.push({
							x: te,
							y: ba,
							color: 'hsl(220,' + Math.round(60 + ratio * 30) + '%,' + lightness + '%)',
							name: item.d
						});
					});

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-vs-temp');
					Highcharts.chart(chartElement[0], {
						chart: {
							type: 'scatter',
							zoomType: 'xy'
						},
						title: {
							text: null
						},
						xAxis: {
							title: { text: $.t('Temperature') + ' (\u00b0C)' },
							crosshair: true
						},
						yAxis: {
							title: { text: $.t('Pressure') + ' (hPa)' }
						},
						tooltip: {
							formatter: function () {
								return this.point.name + '<br/>' +
									$.t('Temperature') + ': <b>' + this.x + ' \u00b0C</b><br/>' +
									$.t('Pressure') + ': <b>' + this.y + ' hPa</b>';
							}
						},
						legend: {
							enabled: false
						},
						plotOptions: {
							scatter: {
								marker: {
									radius: 4,
									symbol: 'circle'
								}
							}
						},
						series: [
							{
								name: $.t('Pressure vs Temperature'),
								data: scatterData,
								colorByPoint: true
							}
						]
					});
				});
			};
		}
	});

	// Chart 4: Pressure vs Humidity Scatter Plot
	app.component('barometerVsHumidityChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Pressure vs Humidity');

			self.$onInit = function () {
				if (self.device.Humidity === undefined) {
					$element.hide();
					return;
				}

				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length === 0) {
						$element.hide();
						return;
					}

					var n = data.result.length;
					var scatterData = [];

					data.result.forEach(function (item, idx) {
						var ba = parseFloat(item.ba);
						var hu = item.hu !== undefined ? parseInt(item.hu, 10) : NaN;
						if (isNaN(ba) || isNaN(hu)) {
							return;
						}
						// Gradient: older points are lighter, newer are darker
						var ratio = idx / Math.max(n - 1, 1);
						var lightness = Math.round(80 - ratio * 55);
						scatterData.push({
							x: hu,
							y: ba,
							color: 'hsl(120,' + Math.round(50 + ratio * 30) + '%,' + lightness + '%)',
							name: item.d
						});
					});

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-vs-humidity');
					Highcharts.chart(chartElement[0], {
						chart: {
							type: 'scatter',
							zoomType: 'xy'
						},
						title: {
							text: null
						},
						xAxis: {
							title: { text: $.t('Humidity') + ' (%)' },
							min: 0,
							max: 100,
							crosshair: true
						},
						yAxis: {
							title: { text: $.t('Pressure') + ' (hPa)' }
						},
						tooltip: {
							formatter: function () {
								return this.point.name + '<br/>' +
									$.t('Humidity') + ': <b>' + this.x + ' %</b><br/>' +
									$.t('Pressure') + ': <b>' + this.y + ' hPa</b>';
							}
						},
						legend: {
							enabled: false
						},
						plotOptions: {
							scatter: {
								marker: {
									radius: 4,
									symbol: 'circle'
								}
							}
						},
						series: [
							{
								name: $.t('Pressure vs Humidity'),
								data: scatterData,
								colorByPoint: true
							}
						]
					});
				});
			};
		}
	});

	// Chart 5: Rolling Pressure Averages
	app.component('barometerRollingAveragesChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Rolling Pressure Averages');

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length < 2) {
						return;
					}

					var rawData = [];
					data.result.forEach(function (item) {
						var ba = parseFloat(item.ba);
						if (!isNaN(ba)) {
							rawData.push([GetLocalDateTimeFromString(item.d), ba]);
						}
					});

					// Compute moving averages for different window sizes
					function movingAverage(data, windowMs) {
						var result = [];
						for (var i = 0; i < data.length; i++) {
							var sum = 0;
							var count = 0;
							var ts = data[i][0];
							// Look back within the window
							for (var j = i; j >= 0; j--) {
								if (ts - data[j][0] > windowMs) break;
								sum += data[j][1];
								count++;
							}
							result.push([ts, count > 0 ? Math.round(sum / count * 10) / 10 : null]);
						}
						return result;
					}

					var ma3h = movingAverage(rawData, 3 * 3600000);
					var ma6h = movingAverage(rawData, 6 * 3600000);
					var ma12h = movingAverage(rawData, 12 * 3600000);

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-rolling-avg');
					Highcharts.chart(chartElement[0], {
						chart: { zoomType: 'x' },
						title: { text: null },
						xAxis: { type: 'datetime', crosshair: true },
						yAxis: {
							title: { text: $.t('Pressure') + ' (hPa)' }
						},
						tooltip: { shared: true, valueSuffix: ' hPa' },
						legend: { enabled: true },
						plotOptions: {
							spline: { marker: { enabled: false } }
						},
						series: [
							{
								id: 'pressure',
								name: $.t('Pressure'),
								type: 'spline',
								data: rawData,
								color: 'rgba(3,190,252,0.4)',
								lineWidth: 1
							},
							{
								id: 'ma-3h',
								name: '3h ' + $.t('Average'),
								type: 'spline',
								data: ma3h,
								color: 'rgba(255,127,39,0.9)',
								lineWidth: 2
							},
							{
								id: 'ma-6h',
								name: '6h ' + $.t('Average'),
								type: 'spline',
								data: ma6h,
								color: 'rgba(220,0,0,0.9)',
								lineWidth: 2
							},
							{
								id: 'ma-12h',
								name: '12h ' + $.t('Average'),
								type: 'spline',
								data: ma12h,
								color: 'rgba(128,0,128,0.9)',
								lineWidth: 2
							}
						]
					});
				});
			};
		}
	});

	// Chart 6: Pressure Volatility
	app.component('barometerVolatilityChart', {
		require: {
			logCtrl: '^deviceBarometerLog'
		},
		bindings: {
			device: '<'
		},
		template: customChartTemplate,
		controllerAs: 'vm',
		controller: function ($element, domoticzApi) {
			const self = this;
			self.chartTitle = $.t('Pressure Volatility');

			self.$onInit = function () {
				domoticzApi.sendCommand('graph', {
					sensor: 'temp',
					idx: self.device.idx,
					range: 'day'
				}).then(function (data) {
					if (!data.result || data.result.length < 2) {
						return;
					}

					var rawData = [];
					data.result.forEach(function (item) {
						var ba = parseFloat(item.ba);
						if (!isNaN(ba)) {
							rawData.push([GetLocalDateTimeFromString(item.d), ba]);
						}
					});

					// Compute rolling standard deviation
					function rollingStdDev(data, windowMs) {
						var result = [];
						for (var i = 0; i < data.length; i++) {
							var ts = data[i][0];
							var values = [];
							for (var j = i; j >= 0; j--) {
								if (ts - data[j][0] > windowMs) break;
								values.push(data[j][1]);
							}
							if (values.length < 2) {
								result.push([ts, 0]);
								continue;
							}
							var mean = values.reduce(function (a, b) { return a + b; }, 0) / values.length;
							var variance = values.reduce(function (a, b) { return a + (b - mean) * (b - mean); }, 0) / (values.length - 1);
							result.push([ts, Math.round(Math.sqrt(variance) * 100) / 100]);
						}
						return result;
					}

					var std3h = rollingStdDev(rawData, 3 * 3600000);
					var std6h = rollingStdDev(rawData, 6 * 3600000);

					var chartElement = $element.find('.chartcontainer');
					chartElement.attr('id', 'chart-' + self.device.idx + '-baro-volatility');
					Highcharts.chart(chartElement[0], {
						chart: { zoomType: 'x' },
						title: { text: null },
						xAxis: { type: 'datetime', crosshair: true },
						yAxis: {
							title: { text: $.t('Std Dev') + ' (hPa)' },
							min: 0
						},
						tooltip: { shared: true, valueSuffix: ' hPa' },
						legend: { enabled: true },
						series: [
							{
								id: 'vol-3h',
								name: '3h ' + $.t('Volatility'),
								type: 'area',
								data: std3h,
								color: 'rgba(255,127,39,0.8)',
								fillColor: 'rgba(255,127,39,0.2)',
								lineWidth: 2,
								marker: { enabled: false }
							},
							{
								id: 'vol-6h',
								name: '6h ' + $.t('Volatility'),
								type: 'area',
								data: std6h,
								color: 'rgba(220,0,0,0.8)',
								fillColor: 'rgba(220,0,0,0.1)',
								lineWidth: 2,
								marker: { enabled: false }
							}
						]
					});
				});
			};
		}
	});

	function chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
			return {
				ctrl: ctrl,
				range: ctrl.range,
				device: ctrl.device,
				sensorType: 'temp',
				chartName: $.t('Barometer'),
				autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
				dataSupplier: {
					yAxes: [
						{
							title: {
								text: $.t('Pressure') + ' (hPa)'
							},
							labels: {
								formatter: function () {
									return this.value;
								}
							}
						}
					],
					valueSuffix: ' hPa',
					timestampFromDataItem: timestampFromDataItem,
					isShortLogChart: isShortLogChart,
					seriesSuppliers: seriesSuppliers.map(function (seriesSupplier) {
						return _.merge(
							{
								dataItemKeys: ['ba' + seriesSupplier.valueKeySuffix]
							},
							seriesSupplier
						);
					})
				}
			};
		}
	}
);
