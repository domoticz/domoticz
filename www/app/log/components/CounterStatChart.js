define(['app', 'luxon', 'ChartWatermark'], function (app, luxon, ChartWatermark) {
    var DateTime = luxon.DateTime;

    app.component('counterStatChart', {
        bindings: {
            device: '<',
            view: '@'
        },
		templateUrl: 'app/log/components/chart-counter-stat.html',
        controller: CounterStatChartController
    });

    function CounterStatChartController($scope, $element, $http, $interval, domoticzGlobals, domoticzApi, dzSettings) {
        const self = this;
		
		self.$element = $element;
		self.$scope = $scope;
		
		$scope.idx = 1768;//8830;//7953;//1768;
		
		$scope.actDay = -2;
		$scope.currentDay = -1;
		$scope.lastHour = -1;
		
		$scope.daily_hour_kwh = [];
		$scope.weekday_hour_kwh = [];
		$scope.weekday_kwh = [];
		$scope.weekday_hour_kwh_raw = [];
		$scope.chart_weekday_hour_kwh = [];

		$scope.chartSeriesDailyHour = {
			id: 'dailyhour',
			name: 'Usage',
			type: 'column',
			yAxis: 0,
			pointInterval: 3600000, // one hour
			color: Highcharts.getOptions().colors[0],
			tooltip: {
				valueSuffix: ' Wh',
			}
		}

		$scope.chartSeriesWeekday = {
			id: 'weekdayhour',
			name: 'Usage',
			type: 'column',
			yAxis: 0,
			color: Highcharts.getOptions().colors[0],
			tooltip: {
				valueSuffix: ' Wh',
			},
			colorByPoint: true,
			groupPadding: 0,
			colors: [
				'#0a9eaa', '#9215ac', '#861ec9', '#7a17e6', '#7010f9', '#691af3', '#00f194'
			],
			data: [
				[$.t('Sunday'), 0],
				[$.t('Monday'), 0],
				[$.t('Tuesday'), 0],
				[$.t('Wednesday'), 0],
				[$.t('Thursday'), 0],
				[$.t('Friday'), 0],
				[$.t('Saturday'), 0]
			]
		}

		$scope.chartDefinitionDay = {};
		
		$scope.chartDefinitionBase = {
			chart: {
				events: {
					load: function () {
						ChartWatermark.init(this);
						$scope.chartRef = this;
					},
					redraw: function () {
						ChartWatermark.refresh(this);
					}
				}
			},
			title: {
				text: 'Hourly Energy Usage'
			},
			xAxis: {
				type: 'datetime',
				labels: {
					format: '{value:%H:%M}',
					overflow: 'justify'
				},
				minRange: 23 * 3600 * 1000
			},
			tooltip: {
				headerFormat: '<span style="font-size: 10px">{point.x:%H:%M}</span><br/>'
			},
			yAxis: [{
				labels: {
					format: '{value} Watt',
					style: {
						color: 'white'
					}
				},
				title: {
					text: 'Usage (Wh)',
					style: {
						color: 'white'
					}
				}
			}],
			legend: {
				enabled: false
			},
			exporting: {
				buttons: {
					contextButton: {
						menuItems: $scope.chart_buttons
					}
				}
			},
			plotOptions: {
				series: {
					animation: false,
				},
				column: {
					pointPadding: 0.2,
					borderWidth: 0
				}
			}
		};
		
		$scope.chartDefinitionWeek = {
			title: {
				text: 'Weekly Energy Usage'
			},
			xAxis: {
				type: 'category',
				labels: {
					autoRotation: [-45, -90],
					style: {
						fontSize: '13px',
						fontFamily: 'Verdana, sans-serif'
					}			
				}
			},		
			yAxis: {
				labels: {
					format: '{value} Watt',
					style: {
						color: Highcharts.getOptions().colors[1]
					}
				},
				title: {
					text: 'Usage (Wh)',
					style: {
						color: Highcharts.getOptions().colors[1]
					}
				}
			},
			legend: {
				enabled: false
			},
			plotOptions: {
				series: {
					animation: false,
				},
				column: {
					pointPadding: 0.2,
					borderWidth: 0
				}
			},
			series: [
				$scope.chartSeriesWeekday
			]
		};

		$scope.setWeekday = function(actDay) {
			$scope.actDay = actDay;
			if ((actDay == $scope.currentDay) || (actDay == -1)) {
				//Force refresh
				self.getStats();
			} else {
				$scope.setWeekdayInt(actDay);
			}
		}
		
		$scope.setWeekdayInt = function(actDay) {
			$scope.actDay = actDay;
			if ($scope.actDay >= 0) {
				var dayName = $scope.chartDefinitionWeek.series[0].data[actDay][0];
				$scope.chartDefinitionDay.title.text = dayName + ' ' + 'Hourly Energy Usage';
				$scope.chartDefinitionDay.tooltip.headerFormat = '<span style="font-size: 10px">' + dayName + ' {point.x:%H:%M}</span><br/>';
				$scope.chart_weekday_hour_kwh = JSON.parse(JSON.stringify($scope.weekday_hour_kwh[actDay]));
				$scope.chartDefinitionDay.series[0].data = $scope.chart_weekday_hour_kwh;
			} else {
				if ($scope.actDay == -1) {
					$scope.chartDefinitionDay.title.text = 'Hourly Energy Usage';
					$scope.chartDefinitionDay.tooltip.headerFormat = '<span style="font-size: 10px">{point.x:%H:%M}</span><br/>';
					$scope.chart_weekday_hour_kwh = JSON.parse(JSON.stringify($scope.daily_hour_kwh));
					$scope.chartDefinitionDay.series[0].data = $scope.chart_weekday_hour_kwh;
				}
			}
		}
		$scope.isActDay = function(day) {
			return ($scope.actDay == day) ? "zoom-button-active" : "";
		}

		self.parseStats = function(data) {
			if (data.errormessage !== undefined) {
				$scope.chartRef.renderError(data.errormessage)
			}
			else if (data.warningmessage !== undefined) {
				$scope.chartRef.renderWarning(data.warningmessage)
			}
			if (typeof data.result != 'undefined') {
				if (typeof data.status != 'undefined') {
					if (data.status == "OK") {
						$scope.daily_hour_kwh = data.result.daily_hour_kwh;
						$scope.weekday_hour_kwh = data.result.weekday_hour_kwh;
						$scope.weekday_hour_kwh_raw = data.result.weekday_hour_kwh_raw;
						$scope.weekday_kwh = data.result.weekday_kwh;
						
						//average today
						const today = new Date();
						$scope.currentDay = today.getDay();
						
						const total_today = $scope.weekday_hour_kwh_raw.reduce((partialSum, a) => partialSum + a, 0);
						
						$scope.weekday_kwh[$scope.currentDay] = ($scope.weekday_kwh[$scope.currentDay]!=0) ? ($scope.weekday_kwh[$scope.currentDay] + total_today) / 2 : total_today;
						
						$.each($scope.weekday_kwh, function (i, item) {
							$scope.chartDefinitionWeek.series[0].data[i][1] = item;
						});
						
						if ($scope.actDay == -2) {
							$scope.lastHour = today.getHours();
							$scope.actDay = $scope.currentDay;
						}
						$scope.setWeekdayInt($scope.actDay);
						return;
					}
				}
			}
			$scope.daily_hour_kwh = [];
			$scope.weekday_hour_kwh = [];
			$scope.weekday_kwh = [];
		}

		self.getStats = function() {
			$http({
				url: "json.htm?type=command&param=getkwhstats&idx=" + $scope.idx,
				async: false,
				dataType: 'json'
			}).then(function successCallback(response) {
				self.parseStats(response.data);
			}, function errorCallback(response) {
				self.parseStats([]);
			});
		}
		
		$scope.OnTimer = function() {
			const today = new Date();
			const actHour = today.getHours();
			const actMinute = today.getMinutes();
			if (
				($scope.lastHour = actHour)
				&& (actMinute == 1)
				) {
				$scope.lastHour = actHour;
				self.getStats();
			}
		}
		
		self.ResetStats = function() {
			bootbox.confirm($.t("Are you sure to delete the Log?\n\nThis action can not be undone!"), function (result) {
				if (result == true) {
					$http({
						url: "json.htm?type=command&param=resetkwhstats&idx=" + $scope.idx,
						async: false,
						dataType: 'json'
					}).then(function successCallback(response) {
						self.getStats();
					}, function errorCallback(response) {
						self.getStats();
					});
				}
			});
		}
		
		self.$onInit = function () {
			$scope.idx = self.device.idx;
			$scope.chartDefinitionDay = Object.assign([], $scope.chartDefinitionBase);
			$scope.chartDefinitionDay.series = [
				JSON.parse(JSON.stringify($scope.chartSeriesDailyHour))
			];
			$scope.chartDefinitionDay.series[0].data = $scope.chart_weekday_hour_kwh;
			
			$scope.chart_buttons = Highcharts.getOptions().exporting.buttons.contextButton.menuItems.slice();
			$scope.chart_buttons.push({
				separator: true
			});
			$scope.chart_buttons.push({
				text: $.t('Reset Internal Statistics'),
				onclick: function () {
					self.ResetStats();
				},
				separator: false
			});
			$scope.chartDefinitionDay.exporting.buttons.contextButton.menuItems = $scope.chart_buttons;
	
			self.getStats();
			
			$scope.mytimer = $interval(function () { $scope.OnTimer(); }, 60 *1000);
		}
		
		$scope.$on('$destroy', function () {
			//stop timers and cleanup here
			if (typeof $scope.mytimer !== "undefined") {
				$interval.cancel($scope.mytimer);
				$scope.mytimer = undefined;
			}
		});


        self.$onChanges = function (changes) {
            if (changes.device && changes.device.currentValue) {
				//console.log("stat device change..");
            }
        };
    }
});
