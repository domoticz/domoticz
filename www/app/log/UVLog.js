define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart'],
	function (app, _, RefreshingChart, DataLoader, ChartLoader) {

		app.component('deviceUvLog', {
			bindings: {
				device: '<'
			},
			templateUrl: 'app/log/UVLog.html',
			controllerAs: '$ctrl',
			controller: function () {
				const $ctrl = this;
				$ctrl.autoRefresh = true;
			}
		});

		app.component('uvCurrentConditions', {
			require: {
				logCtrl: '^deviceUvLog'
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
						return sign + d.toFixed(decimals) + ' ' + suffix;
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

						// UV Index card
						if (device.UVI !== undefined) {
							var uvi = parseFloat(device.UVI);
							var uvi24 = closest24h ? parseFloat(closest24h.uvi) : NaN;
							if (!isNaN(uvi)) {
								var level = '';
								if (uvi <= 2) level = 'Low';
								else if (uvi <= 5) level = 'Moderate';
								else if (uvi <= 7) level = 'High';
								else if (uvi <= 10) level = 'Very High';
								else level = 'Extreme';

								self.cards.push({
									label: $.t('UV Index'),
									value: uvi.toFixed(1),
									delta: formatDelta(uvi, uvi24, 'UVI', 1),
									deltaColor: deltaColor(uvi, uvi24)
								});

								self.cards.push({
									label: $.t('Exposure Level'),
									value: $.t(level),
									delta: '',
									deltaColor: ''
								});
							}
						}

						// Temperature card
						if (device.Temp !== undefined) {
							var degreeSuffix = $.myglobals.tempsign;
							self.cards.push({
								label: $.t('Temperature'),
								value: device.Temp.toFixed(1) + ' ' + degreeSuffix,
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

		app.component('uvShortChart', {
			require: {
				logCtrl: '^deviceUvLog'
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
									id: 'uvi',
									valueKeySuffix: '',
									template: {
										name: $.t('UV'),
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

		app.component('uvLongChart', {
			require: {
				logCtrl: '^deviceUvLog'
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
									id: 'uvi',
									valueKeySuffix: '',
									template: {
										name: $.t('UV'),
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

		app.directive('uvCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceUvLog'
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
						self.sensorType = 'uv';
						self.chart = new RefreshingChart(
							chart.baseParams($),
							chart.angularParams($location, $route, $scope, $timeout, $element),
							chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
							chart.chartParamsCompare(
								domoticzGlobals,
								self,
								chart.chartParamsCompareTemplate(self, self.device.Name, 'UVI'),
								{
									isShortLogChart: false,
									yAxes: [{
										title: {
											text: $.t('UV') + ' (UVI)'
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
				sensorType: 'uv',
				chartName: $.t('UV'),
				autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
				dataSupplier: {
					yAxes: [
						{
							title: {
								text: $.t('UV') + ' (UVI)'
							},
							labels: {
								formatter: function () {
									return this.value;
								}
							}
						}
					],
					valueSuffix: ' UVI',
					timestampFromDataItem: timestampFromDataItem,
					isShortLogChart: isShortLogChart,
					seriesSuppliers: seriesSuppliers.map(function (seriesSupplier) {
						return _.merge(
							{
								dataItemKeys: ['uvi' + seriesSupplier.valueKeySuffix]
							},
							seriesSupplier
						);
					})
				}
			};
		}
	}
);
