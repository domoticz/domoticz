define(['app', 'lodash', 'RefreshingChart', 'DataLoader', 'ChartLoader', 'log/Chart'],
	function (app, _, RefreshingChart, DataLoader, ChartLoader) {

		app.component('deviceCurrentLog', {
			bindings: {
				device: '<'
			},
			templateUrl: 'app/log/CurrentLog.html',
			controllerAs: '$ctrl',
			controller: function () {
				const $ctrl = this;
				$ctrl.autoRefresh = true;
			}
		});

		app.component('currentShortChart', {
			require: {
				logCtrl: '^deviceCurrentLog'
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
					var shortSeriesDefinitions = [
						{
							id: 'v1',
							phase: 'L1',
							valueKeySuffix: '',
							template: {
								name: 'L1',
								color: 'rgba(3,190,252,0.8)'
							}
						},
						{
							id: 'v2',
							phase: 'L2',
							valueKeySuffix: '',
							template: {
								name: 'L2',
								color: 'rgba(252,190,3,0.8)'
							}
						},
						{
							id: 'v3',
							phase: 'L3',
							valueKeySuffix: '',
							template: {
								name: 'L3',
								color: 'rgba(190,252,3,0.8)'
							}
						}
					];

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
							function (data) {
								return shortSeriesDefinitions
									.filter(function (s) {
										if (s.phase === 'L1') { return data.haveL1 !== false; }
										if (s.phase === 'L2') { return data.haveL2 !== false; }
										if (s.phase === 'L3') { return data.haveL3 !== false; }
										return true;
									})
									.map(function (seriesSupplier) {
										return _.merge(
											{
												dataItemKeys: [seriesSupplier.id]
											},
											seriesSupplier
										);
									});
							}
						)
					);
				}
			}
		});

		app.component('currentLongChart', {
			require: {
				logCtrl: '^deviceCurrentLog'
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
					var longSeriesDefinitions = [
						{
							id: 'v1',
							phase: 'L1',
							valueKeySuffix: '',
							template: {
								name: 'L1_Min',
								color: 'rgba(3,190,252,0.8)'
							}
						},
						{
							id: 'v2',
							phase: 'L1',
							valueKeySuffix: '',
							template: {
								name: 'L1_Max',
								color: 'rgba(3,252,190,0.8)'
							}
						},
						{
							id: 'v3',
							phase: 'L2',
							valueKeySuffix: '',
							template: {
								name: 'L2_Min',
								color: 'rgba(252,190,3,0.8)'
							}
						},
						{
							id: 'v4',
							phase: 'L2',
							valueKeySuffix: '',
							template: {
								name: 'L2_Max',
								color: 'rgba(252,3,190,0.8)'
							}
						},
						{
							id: 'v5',
							phase: 'L3',
							valueKeySuffix: '',
							template: {
								name: 'L3_Min',
								color: 'rgba(190,252,3,0.8)'
							}
						},
						{
							id: 'v6',
							phase: 'L3',
							valueKeySuffix: '',
							template: {
								name: 'L3_Max',
								color: 'rgba(3,252,3,0.8)'
							}
						}
					];

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
							function (data) {
								return longSeriesDefinitions
									.filter(function (s) {
										if (s.phase === 'L1') { return data.haveL1 !== false; }
										if (s.phase === 'L2') { return data.haveL2 !== false; }
										if (s.phase === 'L3') { return data.haveL3 !== false; }
										return true;
									})
									.map(function (seriesSupplier) {
										return _.merge(
											{
												dataItemKeys: [seriesSupplier.id]
											},
											seriesSupplier
										);
									});
							}
						)
					);
				}
			}
		});

		app.directive('currentCompareChart', function () {
			return {
				require: {
					logCtrl: '^deviceCurrentLog'
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
						self.sensorType = 'counter';
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
											text: $.t('Current') + ' (' + self.device.getUnit() + ')'
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
				sensorType: 'counter',
				chartName: $.t('Current'),
				autoRefreshIsEnabled: function() { return ctrl.logCtrl.autoRefresh; },
				dataSupplier: {
					yAxes: [
						{
							title: {
								text: $.t('Current') + ' (' + ctrl.device.getUnit() + ')'
							},
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
					seriesSuppliers: seriesSuppliers
				}
			};
		}
	}
);
