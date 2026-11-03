define(['app'], function (app) {
	'use strict';

	// The house has no device of its own: its consumption per source for a day comes from the
	// server's energy balance (short log). Shared by the Energy Dashboard and the dynamic
	// dashboard widgets. A day is "YYYY-MM-DD", an empty day is today.
	app.factory('dzHouseBalance', ['$http', '$timeout', '$uibModal', function ($http, $timeout, $uibModal) {
		function pad(n) {
			return (n < 10 ? '0' : '') + n;
		}

		function fmt(dt) {
			return dt.getFullYear() + '-' + pad(dt.getMonth() + 1) + '-' + pad(dt.getDate());
		}

		function today() {
			return fmt(new Date());
		}

		// The day iDelta days from szDay (today when empty); today itself is returned as ''
		function step(szDay, iDelta) {
			let p = (szDay || today()).split('-');
			let day = fmt(new Date(+p[0], +p[1] - 1, +p[2] + iDelta));
			return (day >= today()) ? '' : day;
		}

		function load(szDay) {
			let url = 'json.htm?type=command&param=getenergybalance&series=1' + (szDay ? '&day=' + szDay : '');
			return $http.get(url).then(function (response) {
				let eb = response.data;
				let series = (eb && eb.Series) || [];
				return {
					hasData: series.length > 0,
					day: (eb && eb.Day) || szDay || today(),
					oldestDay: (eb && eb.OldestDay) || '',
					selfSufficiency: (eb && typeof eb.SelfSufficiency == 'number') ? eb.SelfSufficiency : null,
					solar: eb && eb.SolarToHouse,
					battery: eb && eb.BatteryToHouse,
					grid: eb && eb.GridToHouse,
					series: series
				};
			});
		}

		function render(containerId, series) {
			let points = function (key) {
				return series.map(function (item) { return [GetUTCFromString(item.d), item[key]]; });
			};
			return Highcharts.chart(containerId, {
				chart: { type: 'area', backgroundColor: 'transparent', zoomType: 'x' },
				credits: { enabled: false },
				title: { text: null },
				xAxis: { type: 'datetime', labels: { style: { color: '#ccc' } } },
				yAxis: { title: { text: 'Watt', style: { color: '#ccc' } }, min: 0, labels: { style: { color: '#ccc' } }, gridLineColor: 'rgba(255,255,255,0.1)' },
				legend: { itemStyle: { color: '#ddd' }, itemHoverStyle: { color: '#fff' } },
				tooltip: { shared: true, valueSuffix: ' W', xDateFormat: '%H:%M' },
				plotOptions: { area: { stacking: 'normal', lineWidth: 1, marker: { enabled: false } } },
				series: [
					{ name: $.t('Grid'), color: '#00c6da', data: points('g') },
					{ name: $.t('Battery') + ' (' + $.t('charged from the grid') + ')', color: '#4f8f8f', data: points('bg') },
					{ name: $.t('Battery') + ' (' + $.t('charged from solar') + ')', color: '#90e090', data: points('bs') },
					{ name: $.t('Solar'), color: '#ffd000', data: points('s') }
				]
			});
		}

		// Fills the popup state of a scope (house: { day, hasData, ... }) for szDay and draws the chart
		// into the container once the panel is visible. isOpen() tells whether the popup is still there.
		let showSeq = 0;

		function show(house, szDay, containerId, isOpen, setChart) {
			// Only the answer to the last request counts, when previous/next is clicked faster than the server answers
			let seq = ++showSeq;
			house.hasData = false;
			house.message = '...';
			load(szDay).then(function (d) {
				if (seq !== showSeq) {
					return;
				}
				house.day = d.day;
				house.isToday = !szDay;
				house.canPrev = !!d.oldestDay && d.day > d.oldestDay;
				house.canNext = !house.isToday;
				house.selfSufficiency = d.selfSufficiency;
				if (!d.hasData) {
					setChart(null);
					house.message = $.t('No data available for this day');
					return;
				}
				house.solar = d.solar;
				house.battery = d.battery;
				house.grid = d.grid;
				house.hasData = true;
				// Rendered after the panel is visible, so Highcharts can measure its container
				$timeout(function () {
					if (isOpen() && (seq === showSeq)) {
						// Highcharts destroys a chart that already sits in the container itself, so clear the old one first
						setChart(null);
						setChart(render(containerId, d.series));
					}
				});
			}, function () {
				if (seq === showSeq) {
					house.message = $.t('Error communicating to server!');
				}
			});
		}

		// Modal variant for the dynamic dashboard widgets (the Energy Dashboard has its own inline panel)
		function openDialog() {
			return $uibModal.open({
				windowClass: 'dd-house-modal',
				size: 'lg',
				template:
					// Bootstrap 2 sizes .modal itself (560px, ignoring size:'lg'); centre it with a transform
					// instead of the fixed -280px margin, so it can be wider and still fit small screens.
					// Its .modal-body is capped at 400px, which clips the chart legend on a phone, where
					// the header wraps: let it use the screen height instead (the dialog sits at top:10%,
					// the body adds 30px padding and a border, so 90vh - 50px keeps it inside the screen).
					'<style>.modal.dd-house-modal{width:94vw;max-width:1200px;left:50%;right:auto;margin-left:0;transform:translateX(-50%)}' +
					'.dd-house-modal .modal-body{max-height:calc(90vh - 50px)}' +
					'.dd-house-modal .modal-content{background:rgba(20,24,40,0.97);border:1px solid #ff7bff;color:rgba(255,255,255,0.85)}' +
					'.dd-house-modal .hp-head{display:flex;align-items:baseline;gap:16px;flex-wrap:wrap}' +
					'.dd-house-modal .hp-title{font-size:18px;font-weight:bold}' +
					'.dd-house-modal .hp-nav{background:none;border:1px solid rgba(255,255,255,0.4);border-radius:4px;color:inherit;cursor:pointer;padding:0 8px}' +
					'.dd-house-modal .hp-nav[disabled]{opacity:0.3;cursor:default}' +
					'.dd-house-modal .hp-close{margin-left:auto;cursor:pointer;font-size:20px;line-height:1;background:none;border:0;color:inherit}' +
					'.dd-house-modal .hp-totals{display:flex;gap:18px;flex-wrap:wrap;margin:6px 0 4px 0;font-size:13px}' +
					'.dd-house-modal .hp-totals i{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px}' +
					'.dd-house-modal .hp-chart{height:340px}' +
					'.dd-house-modal .hp-empty{padding:40px 0;text-align:center;opacity:0.7}</style>' +
					'<div class="modal-body">' +
						'<div class="hp-head">' +
							'<span class="hp-title">{{ ::\'House\' | translate }} - <span class="hp-day">{{ house.isToday !== false ? (\'Today\' | translate) : house.day }}</span></span>' +
							'<button type="button" class="hp-nav hp-prev" ng-click="go(-1)" ng-disabled="!house.canPrev" title="{{ ::\'Previous day\' | translate }}">&lsaquo;</button>' +
							'<button type="button" class="hp-nav hp-next" ng-click="go(1)" ng-disabled="!house.canNext" title="{{ ::\'Next day\' | translate }}">&rsaquo;</button>' +
							'<span ng-show="house.selfSufficiency !== null">{{ ::\'Self-sufficiency\' | translate }}: {{ house.selfSufficiency | number:0 }}%</span>' +
							'<button type="button" class="hp-close" ng-click="$dismiss()" title="{{ ::\'Close\' | translate }}">&times;</button>' +
						'</div>' +
						'<div class="hp-totals" ng-show="house.hasData">' +
							'<span><i style="background:#ffd000"></i>{{ ::\'Solar\' | translate }} {{ house.solar | number:2 }} kWh</span>' +
							'<span><i style="background:#90e090"></i>{{ ::\'Battery\' | translate }} {{ house.battery | number:2 }} kWh</span>' +
							'<span><i style="background:#00c6da"></i>{{ ::\'Grid\' | translate }} {{ house.grid | number:2 }} kWh</span>' +
						'</div>' +
						'<div id="housechart-dlg" class="hp-chart" ng-show="house.hasData"></div>' +
						'<div class="hp-empty" ng-show="!house.hasData">{{ house.message }}</div>' +
					'</div>',
				controller: ['$scope', function ($scope) {
					let chart;
					let open = true;
					let setChart = function (c) {
						if (chart) {
							chart.destroy();
						}
						chart = c || undefined;
					};
					$scope.house = { day: '', isToday: true, canPrev: false, canNext: false, hasData: false, selfSufficiency: null, message: '...' };
					$scope.go = function (delta) {
						show($scope.house, step($scope.house.day, delta), 'housechart-dlg', function () { return open; }, setChart);
					};
					show($scope.house, '', 'housechart-dlg', function () { return open; }, setChart);
					$scope.$on('$destroy', function () {
						open = false;
						setChart(null);
					});
				}]
			}).result.catch(angular.noop);
		}

		return { load: load, render: render, show: show, step: step, openDialog: openDialog };
	}]);
});
