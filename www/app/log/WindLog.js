define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart'],
	function (app, _, RefreshingChart, DataLoader, ChartLoader) {

		app.component('deviceWindLog', {
			bindings: {
				device: '<'
			},
			templateUrl: 'app/log/WindLog.html',
			controllerAs: '$ctrl',
			controller: function () {
				const $ctrl = this;
				$ctrl.autoRefresh = true;
			}
		});

		app.component('windCurrentConditions', {
			require: {
				logCtrl: '^deviceWindLog'
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

					var unit = device.getUnit();

					function rebuildCards() {
						self.cards.length = 0;
						var items = self.logCtrl.dayGraphData;
						var closest24h = (items && items.length >= 2) ? find24hAgo(items) : null;

						// Wind Speed card
						if (device.Speed !== undefined) {
							var sp = parseFloat(device.Speed);
							var sp24 = closest24h ? parseFloat(closest24h.sp) : NaN;
							if (!isNaN(sp)) {
								var cardValue = sp.toFixed(1) + ' ' + unit;

								// EU ., replacements
								if ($.myglobals.EUNumberFormat) {
									cardValue = formatEUValue(cardValue);
								}

								self.cards.push({
									label: $.t('Speed'),
									value: cardValue,
									delta: formatDelta(sp, sp24, unit, 1),
									deltaColor: deltaColor(sp, sp24)
								});
							}
						}

						// Wind Gust card
						if (device.Gust !== undefined) {
							var gu = parseFloat(device.Gust);
							var gu24 = closest24h ? parseFloat(closest24h.gu) : NaN;
							if (!isNaN(gu)) {
								var cardValue = gu.toFixed(1) + ' ' + unit;

								// EU ., replacements
								if ($.myglobals.EUNumberFormat) {
									cardValue = formatEUValue(cardValue);
								}

								self.cards.push({
									label: $.t('Gust'),
									value: cardValue,
									delta: formatDelta(gu, gu24, unit, 1),
									deltaColor: deltaColor(gu, gu24)
								});
							}
						}

						// Direction card
						if (device.Direction !== undefined) {
							self.cards.push({
								label: $.t('Direction'),
								value: device.DirectionStr + ' (' + device.Direction + '°)',
								delta: '',
								deltaColor: ''
							});
						}

						// Wind Chill card
						if (device.Chill !== undefined) {
							var degreeSuffix = $.myglobals.tempsign;
							var cardValue = device.Chill.toFixed(1) + ' ' + degreeSuffix;

							// EU ., replacements
							if ($.myglobals.EUNumberFormat) {
								cardValue = formatEUValue(cardValue);
							}

							self.cards.push({
								label: $.t('Chill'),
								value: cardValue,
								delta: '',
								deltaColor: ''
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

		app.component('windShortChart', {
			require: {
				logCtrl: '^deviceWindLog'
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
									id: 'sp',
									valueKeySuffix: '',
									template: {
										name: $.t('Speed'),
										color: 'rgba(3,190,252,0.8)'
									}
								},
								{
									id: 'gu',
									valueKeySuffix: '',
									template: {
										name: $.t('Gust'),
										color: 'rgba(255,127,39,0.8)'
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

		var wind_directions = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];

		app.component('windDirectionChart', {
			require: {
				logCtrl: '^deviceWindLog'
			},
			bindings: {
				device: '<'
			},
			template: '<div class="chart noselect"><div class="chart-title-center"><div class="chart-title-container"><h2 ng-bind="vm.chartTitle"></h2></div></div><div class="chartarea"><div id="winddirectiongraph" style="height: 400px;"></div></div></div>',
			controllerAs: 'vm',
			controller: function ($element, domoticzApi) {
				const self = this;

				self.$onInit = function () {
					var chartElement = $element.find('#winddirectiongraph');

					self.chartTitle = $.t('Wind') + ' ' + $.t('Direction') + ' ' + Get5MinuteHistoryDaysGraphTitle();

					chartElement.highcharts({
						chart: {
							polar: true,
							type: 'column'
						},
						title: null,
						pane: {
							size: '85%'
						},
						xAxis: {
							tickmarkPlacement: 'on',
							tickWidth: 1,
							tickPosition: 'outside',
							tickLength: 5,
							tickColor: '#999',
							tickInterval: 1,
							categories: wind_directions,
							labels: {
								formatter: function () {
									return this.value;
								}
							}
						},
						yAxis: {
							min: 0,
							showLastLabel: true,
							title: {
								text: $.t('Frequency') + ' (%)'
							},
							labels: {
								formatter: function () {
									return this.value + '%';
								}
							},
							reversedStacks: false
						},
						tooltip: {
							formatter: function () {
								return this.x + ': ' + this.y + ' %';
							}
						},
						plotOptions: {
							series: {
								stacking: 'normal',
								shadow: false,
								groupPadding: 0,
								pointPlacement: 'on'
							}
						},
						legend: {
							align: 'right',
							verticalAlign: 'top',
							y: 100,
							layout: 'vertical'
						},
						series: []
					});

					domoticzApi.sendCommand('graph', {
						sensor: 'winddir',
						idx: self.device.idx,
						range: 'day'
					}).then(function (data) {
						if (typeof data.result_speed === 'undefined') {
							return;
						}

						var hchart = chartElement.highcharts();

						data.result_speed.forEach(function (item, i) {
							var seriesData = [];
							for (var j = 0; j < 16; j++) {
								seriesData.push(parseFloat(item.sp[j]));
							}
							hchart.addSeries({
								name: item.label,
								data: seriesData
							}, false);
						});

						hchart.redraw();
					});
				};
			}
		});

		app.component('windSpeedFrequencyChart', {
			require: {
				logCtrl: '^deviceWindLog'
			},
			bindings: {
				device: '<'
			},
			template: '<div class="chart noselect"><div class="chart-title-center"><div class="chart-title-container"><h2 ng-bind="vm.chartTitle"></h2></div></div><div class="chartarea"><div id="windspeedfreqgraph" style="height: 400px;"></div></div></div>',
			controllerAs: 'vm',
			controller: function ($element, domoticzApi) {
				const self = this;

				self.$onInit = function () {
					var chartElement = $element.find('#windspeedfreqgraph');
					var unit = self.device.getUnit();

					self.chartTitle = $.t('Wind Speed Frequency') + ' ' + Get5MinuteHistoryDaysGraphTitle();

					domoticzApi.sendCommand('graph', {
						sensor: 'wind',
						idx: self.device.idx,
						range: 'day'
					}).then(function (data) {
						if (typeof data.result === 'undefined' || data.result.length === 0) {
							return;
						}

						var speeds = data.result.map(function (item) {
							return parseFloat(item.sp);
						}).filter(function (v) { return !isNaN(v); });

						if (speeds.length === 0) return;

						var maxSpeed = Math.max.apply(null, speeds);
						var numBins = Math.max(10, Math.min(30, Math.round(Math.sqrt(speeds.length))));
						var binWidth = maxSpeed > 0 ? maxSpeed / numBins : 1;
						// Round binWidth to a nice number
						var magnitude = Math.pow(10, Math.floor(Math.log10(binWidth)));
						binWidth = Math.ceil(binWidth / magnitude) * magnitude;
						if (binWidth === 0) binWidth = 1;
						numBins = Math.ceil(maxSpeed / binWidth) + 1;

						var bins = new Array(numBins).fill(0);
						speeds.forEach(function (speed) {
							var bin = Math.floor(speed / binWidth);
							if (bin >= numBins) bin = numBins - 1;
							bins[bin]++;
						});

						var categories = [];
						var histData = [];
						for (var i = 0; i < numBins; i++) {
							categories.push(Math.round(i * binWidth * 10) / 10);
							histData.push(Math.round(bins[i] / speeds.length * 10000) / 100);
						}

						// Calculate Weibull distribution fit
						var mean = speeds.reduce(function (a, b) { return a + b; }, 0) / speeds.length;
						var variance = speeds.reduce(function (a, b) { return a + (b - mean) * (b - mean); }, 0) / speeds.length;
						var stddev = Math.sqrt(variance);

						// Estimate Weibull parameters using method of moments
						var cv = stddev / mean; // coefficient of variation
						var k = Math.pow(cv, -1.086); // shape parameter approximation
						var c = mean / gamma(1 + 1 / k); // scale parameter

						var weibullData = [];
						for (var j = 0; j < numBins; j++) {
							var x = (j + 0.5) * binWidth;
							var pdf = (k / c) * Math.pow(x / c, k - 1) * Math.exp(-Math.pow(x / c, k));
							weibullData.push(Math.round(pdf * binWidth * 10000) / 100);
						}

						chartElement.highcharts({
							chart: {
								type: 'column'
							},
							title: null,
							xAxis: {
								categories: categories,
								title: {
									text: $.t('Wind Speed') + ' (' + unit + ')'
								},
								crosshair: true
							},
							yAxis: {
								min: 0,
								title: {
									text: $.t('Frequency') + ' (%)'
								}
							},
							tooltip: {
								shared: true
							},
							plotOptions: {
								column: {
									groupPadding: 0,
									pointPadding: 0,
									borderWidth: 1
								}
							},
							legend: {
								align: 'right',
								verticalAlign: 'top',
								y: 40,
								layout: 'vertical'
							},
							series: [{
								type: 'column',
								name: $.t('Histogram'),
								data: histData,
								color: 'rgba(3,190,252,0.8)'
							}, {
								type: 'spline',
								name: 'Weibull',
								data: weibullData,
								color: 'rgba(255,80,80,0.9)',
								lineWidth: 2,
								marker: { enabled: false },
								tooltip: {
									valueSuffix: ' %'
								}
							}]
						});
					});
				};

				// Gamma function approximation (Lanczos)
				function gamma(z) {
					if (z < 0.5) {
						return Math.PI / (Math.sin(Math.PI * z) * gamma(1 - z));
					}
					z -= 1;
					var g = 7;
					var coef = [
						0.99999999999980993, 676.5203681218851, -1259.1392167224028,
						771.32342877765313, -176.61502916214059, 12.507343278686905,
						-0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7
					];
					var x = coef[0];
					for (var i = 1; i < g + 2; i++) {
						x += coef[i] / (z + i);
					}
					var t = z + g + 0.5;
					return Math.sqrt(2 * Math.PI) * Math.pow(t, z + 0.5) * Math.exp(-t) * x;
				}
			}
		});

		app.component('windLongChart', {
			require: {
				logCtrl: '^deviceWindLog'
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
									id: 'sp',
									valueKeySuffix: '',
									template: {
										name: $.t('Speed'),
										color: 'rgba(3,190,252,0.8)'
									}
								},
								{
									id: 'gu',
									valueKeySuffix: '',
									template: {
										name: $.t('Gust'),
										color: 'rgba(255,127,39,0.8)'
									}
								}
							]
						)
					);
				}
			}
		});

		app.directive('windCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceWindLog'
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
						self.sensorType = 'wind';
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
											text: $.t('Speed') + ' (' + self.device.getUnit() + ')'
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

		function getWindScales() {
			var lscales = [];
			if ($.myglobals.windsign === 'bf') {
				lscales.push({ from: 0, to: 1 });
				lscales.push({ from: 1, to: 2 });
				lscales.push({ from: 2, to: 3 });
				lscales.push({ from: 3, to: 4 });
				lscales.push({ from: 4, to: 5 });
				lscales.push({ from: 5, to: 6 });
				lscales.push({ from: 6, to: 7 });
				lscales.push({ from: 7, to: 8 });
				lscales.push({ from: 8, to: 9 });
				lscales.push({ from: 9, to: 10 });
				lscales.push({ from: 10, to: 11 });
				lscales.push({ from: 11, to: 12 });
				lscales.push({ from: 12, to: 100 });
			} else {
				var s = $.myglobals.windscale;
				lscales.push({ from: 0.3 * s, to: 1.5 * s });
				lscales.push({ from: 1.5 * s, to: 3.3 * s });
				lscales.push({ from: 3.3 * s, to: 5.5 * s });
				lscales.push({ from: 5.5 * s, to: 8 * s });
				lscales.push({ from: 8.0 * s, to: 10.8 * s });
				lscales.push({ from: 10.8 * s, to: 13.9 * s });
				lscales.push({ from: 13.9 * s, to: 17.2 * s });
				lscales.push({ from: 17.2 * s, to: 20.8 * s });
				lscales.push({ from: 20.8 * s, to: 24.5 * s });
				lscales.push({ from: 24.5 * s, to: 28.5 * s });
				lscales.push({ from: 28.5 * s, to: 32.7 * s });
				lscales.push({ from: 32.7 * s, to: 100 * s });
			}
			return lscales;
		}

		function getWindPlotBands() {
			var lscales = getWindScales();
			var labels = [
				'Light air', 'Light breeze', 'Gentle breeze', 'Moderate breeze',
				'Fresh breeze', 'Strong breeze', 'High wind', 'Fresh gale',
				'Strong gale', 'Storm', 'Violent storm', 'Hurricane'
			];
			return labels.map(function (label, i) {
				return {
					from: lscales[i].from,
					to: lscales[i].to,
					color: i % 2 === 0 ? 'rgba(68, 170, 213, 0.1)' : 'rgba(68, 170, 213, 0.2)',
					label: {
						text: $.t(label),
						style: {
							color: '#CCCCCC'
						}
					}
				};
			});
		}

		function chartParams(domoticzGlobals, ctrl, isShortLogChart, timestampFromDataItem, seriesSuppliers) {
			return {
				ctrl: ctrl,
				range: ctrl.range,
				device: ctrl.device,
				sensorType: 'wind',
				chartName: $.t('Wind'),
				autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
				dataSupplier: {
					yAxes: [
						{
							title: {
								text: $.t('Speed') + ' (' + ctrl.device.getUnit() + ')'
							},
							min: 0,
							minorGridLineWidth: 0,
							gridLineWidth: 0,
							alternateGridColor: null,
							plotBands: getWindPlotBands(),
							labels: {
								formatter: function () {
									return this.value;
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
								dataItemKeys: [seriesSupplier.id]
							},
							seriesSupplier
						);
					})
				}
			};
		}
	}
);
