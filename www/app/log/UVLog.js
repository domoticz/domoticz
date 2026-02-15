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
					new RefreshingChart(
						chart.baseParams($),
						chart.angularParams($location, $route, $scope, $timeout, $element),
						chart.domoticzParams(domoticzGlobals, domoticzApi, domoticzDataPointApi),
						chartParams(
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
						)
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
