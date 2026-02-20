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

		var wind_directions = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];

		app.component('windDirectionChart', {
			require: {
				logCtrl: '^deviceWindLog'
			},
			bindings: {
				device: '<'
			},
			template: '<h2 data-i18n="Wind Direction"></h2><div id="winddirectiongraph" style="height: 400px;"></div>',
			controllerAs: 'vm',
			controller: function ($element, domoticzApi) {
				const self = this;

				self.$onInit = function () {
					var chartElement = $element.find('#winddirectiongraph');

					chartElement.highcharts({
						chart: {
							polar: true,
							type: 'column'
						},
						title: {
							text: $.t('Wind') + ' ' + $.t('Direction') + ' ' + Get5MinuteHistoryDaysGraphTitle()
						},
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
