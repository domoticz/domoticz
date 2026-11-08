define(['app', 'energyWeather.service'], function (app) {
	app.controller('EnergyWeatherController', ['$scope', '$rootScope', '$q', '$timeout', 'dzEnergyWeather', function ($scope, $rootScope, $q, $timeout, dzEnergyWeather) {
		var monthNames = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
		var settings = null;
		var cache = {};		// year -> loaded days
		var unitNames = {};	// airco idx -> name
		var currentYear = new Date().getFullYear();

		$scope.mode = 'heat';		// heat = gas vs heating degree days, hp = heat pump / airco kWh vs heating degree days, cool = the same kWh vs cooling degree days
		$scope.gran = 'month';		// day | week | month
		$scope.year = currentYear;
		$scope.month = new Date().getMonth() + 1;
		$scope.split = false;
		$scope.years = [currentYear, currentYear - 1, currentYear - 2, currentYear - 3, currentYear - 4];
		$scope.compareYear = currentYear - 1;	// the year drawn as the grey line and used for "compared to"
		$scope.months = monthNames.map(function (n, i) { return { id: i + 1, name: $.t(n) }; });
		$scope.loading = true;
		$scope.extras = [];
		$scope.outliers = [];
		$scope.currencySign = ($rootScope.config && $rootScope.config.CurrencySign) || '';
		$scope.problem = '';
		$scope.tiles = null;
		$scope.hasAirco = false;
		$scope.multiAirco = false;
		$scope.hasGas = false;
		$scope.gasKwh = 9.77;	// kWh per m3 of natural gas (Dutch calorific value), editable on the page
		try {
			var storedKwh = parseFloat(localStorage.getItem('ew_gaskwh'));
			if (storedKwh > 0) { $scope.gasKwh = storedKwh; }
		} catch (e) { /* storage blocked */ }

		function unit() {
			return $scope.mode === 'heat' ? 'm3' : 'kWh';
		}

		// kWh per m3 of gas, the default while the field is empty
		function gasPerM3() {
			return $scope.gasKwh > 0 ? $scope.gasKwh : 9.77;
		}

		function usageOf(day) {
			if ($scope.mode === 'both') {
				// total heating energy in kWh: gas converted with its calorific value plus the heat pump / airco kWh
				// days without a heat pump / airco reading are left out, like in the heat pump mode
				return (typeof day.gas == 'undefined' || day.aircoTotal === null || typeof day.aircoTotal == 'undefined') ? undefined : day.gas * gasPerM3() + day.aircoTotal;
			}
			return $scope.mode === 'heat' ? day.gas : day.aircoTotal;
		}

		function ddOf(day) {
			return $scope.mode === 'cool' ? day.cdd : day.hdd;
		}

		// Day of the year on the leap year 2000 calendar, so a date falls in the same week bucket in every year
		function dayOfYear(d) {
			var m = parseInt(d.substring(5, 7), 10) - 1, dd = parseInt(d.substring(8, 10), 10);
			return Math.round((Date.UTC(2000, m, dd) - Date.UTC(2000, 0, 1)) / 86400000);
		}

		// Common x position (year 2000) so this year and last year overlay each other
		function xFor(d) {
			var m = parseInt(d.substring(5, 7), 10) - 1, dd = parseInt(d.substring(8, 10), 10);
			if ($scope.gran === 'day') { return Date.UTC(2000, m, dd); }
			if ($scope.gran === 'week') { return Date.UTC(2000, 0, 1 + Math.floor(dayOfYear(d) / 7) * 7); }
			return Date.UTC(2000, m, 1);
		}

		function inRange(d) {
			return ($scope.gran !== 'day') || (parseInt(d.substring(5, 7), 10) === $scope.month);
		}

		// Valid days of a year for the current mode: temperature and usage both known
		function validDays(year) {
			var days = (cache[year] && cache[year].days) || {};
			return Object.keys(days).sort().filter(function (d) {
				return inRange(d) && usageOf(days[d]) !== null && typeof usageOf(days[d]) != 'undefined';
			}).map(function (d) { return { d: d, v: days[d] }; });
		}

		function bucketize(list) {
			var buckets = {};
			list.forEach(function (it) {
				var x = xFor(it.d);
				var b = buckets[x] || (buckets[x] = { x: x, usage: 0, partGas: 0, partElec: 0, grid: 0, gridN: 0, dd: 0, t: 0, n: 0, nFrac: 0, costGas: 0, costElec: 0, unitCost: {}, unitGap: {}, units: {} });
				b.usage += usageOf(it.v);
				b.partGas += (typeof it.v.gas != 'undefined') ? it.v.gas * gasPerM3() : 0;
				b.partElec += it.v.aircoTotal || 0;
				b.dd += ddOf(it.v);
				b.t += it.v.t;
				b.n++;
				b.costGas += it.v.costGas || 0;
				b.costElec += it.v.costAirco || 0;
				// a day with usage but no logged cost makes cost / usage of that series meaningless
				if (typeof it.v.gas != 'undefined' && !(it.v.costGas > 0)) { b.gasGap = true; }
				Object.keys(it.v.airco).forEach(function (idx) {
					if (!((it.v.costUnit || {})[idx] > 0)) { b.airGap = true; b.unitGap[idx] = true; }
				});
				Object.keys(it.v.costUnit || {}).forEach(function (idx) { b.unitCost[idx] = (b.unitCost[idx] || 0) + it.v.costUnit[idx]; });
				b.nFrac += (typeof it.v.frac != 'undefined') ? it.v.frac : 1;	// baseload of a partial day counts only the part that has passed
				if (it.v.partial) { b.partial = true; }
				if (typeof it.v.grid != 'undefined') { b.grid += it.v.grid; b.gridN++; }
				Object.keys(it.v.airco).forEach(function (idx) { b.units[idx] = (b.units[idx] || 0) + it.v.airco[idx]; });
			});
			return Object.keys(buckets).map(function (k) { return buckets[k]; }).sort(function (a, b) { return a.x - b.x; });
		}

		function r2(v) {
			return Math.round(v * 100) / 100;
		}

		function minDD() {
			return $scope.gran === 'day' ? 0.5 : ($scope.gran === 'week' ? 3 : 10);
		}

		function xAxisFormat() {
			return $scope.gran === 'month' ? '%b' : '%e %b';
		}

		function renderCombined(buckets, prevBuckets) {
			var isBoth = $scope.mode === 'both';
			var isCool = $scope.mode === 'hp' || $scope.mode === 'cool';	// electric only: heat pump / airco kWh, shown with the grid line
			var series = [];
			var stack = $scope.split && (isCool || isBoth) && $scope.multiAirco;
			// every series knows how to read its value from a bucket, so the tooltip can show the compare year with the same values
			// cf reads the logged cost of the same series, unit names the unit of its values for the price per unit
			// gf tells whether a bucket has usage without logged cost for this series (then no price per unit is shown)
			function add(def, f, cf, unitName, gf) {
				def.ewg = gf || null;
				def.ewf = f;
				def.ewc = cf || null;
				def.ewu = unitName || '';
				def.data = buckets.map(function (b) { var v = f(b); return [b.x, v === null ? null : r2(v)]; });
				series.push(def);
			}
			function addUnits() {
				settings.idAirco.forEach(function (idx) {
					add({ type: 'column', name: unitNames[idx] || ('#' + idx), stacking: 'normal', yAxis: 0 }, function (b) { return b.units[idx] || 0; }, function (b) { return b.unitCost[idx] || 0; }, 'kWh', function (b) { return !!b.unitGap[idx]; });
				});
			}
			if (isBoth) {
				// stacked: gas as kWh equivalent, then the heat pump / airco (total or per unit)
				add({ type: 'column', name: $.t('Gas') + ' (kWh)', stacking: 'normal', yAxis: 0, color: 'rgba(255,152,0,0.85)' }, function (b) { return b.partGas; }, function (b) { return b.costGas; }, 'kWh', function (b) { return !!b.gasGap; });
				if (stack) {
					addUnits();
				} else {
					add({ type: 'column', name: $.t('Heat Pump / Airco') + ' (kWh)', stacking: 'normal', yAxis: 0, color: 'rgba(3,190,252,0.8)' }, function (b) { return b.partElec; }, function (b) { return b.costElec; }, 'kWh', function (b) { return !!b.airGap; });
				}
			} else if (stack) {
				addUnits();
			} else {
				add({
					type: 'column', name: isCool ? $.t('Heat Pump / Airco') + ' (kWh)' : $.t('Gas') + ' (m3)', yAxis: 0,
					color: isCool ? 'rgba(3,190,252,0.8)' : 'rgba(255,152,0,0.85)'
				}, function (b) { return b.usage; }, function (b) { return isCool ? b.costElec : b.costGas; }, isCool ? 'kWh' : 'm3', function (b) { return !!(isCool ? b.airGap : b.gasGap); });
			}
			var hasGrid = isCool && buckets.some(function (b) { return b.gridN > 0; });
			if (hasGrid) {
				add({ type: 'spline', name: $.t('Grid') + ' (kWh)', yAxis: 1, color: '#9c27b0' }, function (b) { return b.gridN ? b.grid : null; });
			}
			add({ type: 'spline', name: $.t('Degree Days'), yAxis: 2, color: '#e53935', dashStyle: 'ShortDash' }, function (b) { return b.dd; });
			add({ type: 'spline', name: $.t('Outside Temperature') + ' (°' + $.myglobals.tempsign + ')', yAxis: 3, color: '#43a047' }, function (b) { return b.t / b.n; });

			var yAxis = [{ title: { text: unit() }, min: 0 }];
			yAxis.push(hasGrid ? { title: { text: 'kWh ' + $.t('Grid') }, min: 0, opposite: true } : { title: { text: '' }, visible: false });
			// degree days are a sum per bucket (tens to hundreds), the temperature an average (degrees): separate axes
			yAxis.push({ title: { text: $.t('Degree Days'), style: { color: '#e53935' } }, labels: { style: { color: '#e53935' } }, opposite: true, gridLineWidth: 0, min: 0 });
			yAxis.push({ title: { text: '°' + $.myglobals.tempsign, style: { color: '#43a047' } }, labels: { style: { color: '#43a047' } }, opposite: false, gridLineWidth: 0 });

			var prevByX = {};
			prevBuckets.forEach(function (b) { prevByX[b.x] = b; });
			// cost of a bucket: the daily cost Domoticz logged for the gas and heat pump / airco meters, null when there is none
			function costOf(b) {
				if (!b) { return null; }
				var cost = isBoth ? b.costGas + b.costElec : (isCool ? b.costElec : b.costGas);
				return cost > 0 ? cost : null;
			}
			function costLine(v) {
				if (v === null) { return ''; }
				return '<span style="color:#9e9e9e">●</span> ' + $.t('Cost') + ': <b>' + ($scope.currencySign || '') + ' ' + Highcharts.numberFormat(v, 2) + '</b><br/>';
			}
			var curByX = {};
			buckets.forEach(function (b) { curByX[b.x] = b; });
			var xFormat = $scope.gran === 'month' ? '%B' : '%e %B';
			// one tooltip line; a series with a logged cost shows it behind the value, with the price paid per unit
			function line(color, name, v, cost, unitName) {
				var sign = String($scope.currencySign || '').replace(/[&<>"']/g, function (c) { return '&#' + c.charCodeAt(0) + ';'; });
				var text = (v === null || typeof v == 'undefined' || isNaN(v)) ? '-' : Highcharts.numberFormat(v, 1);
				var costText = '';
				if (cost > 0) {
					costText = ' <span style="opacity:0.8">' + sign + ' ' + Highcharts.numberFormat(cost, 2);
					if (v > 0 && unitName) { costText += ' (' + sign + ' ' + Highcharts.numberFormat(cost / v, 3) + ' / ' + unitName + ')'; }
					costText += '</span>';
				}
				var safeName = String(name).replace(/[&<>"']/g, function (c) { return '&#' + c.charCodeAt(0) + ';'; });
				return '<span style="color:' + color + '">\u25CF</span> ' + safeName + ': <b>' + text + '</b>' + costText + '<br/>';
			}

			$('#ewCombined').highcharts({
				title: { text: (isBoth ? $.t('Gas and heat pump / airco heating vs outside temperature') : (isCool ? $.t('Heat pump / airco and grid vs outside temperature') : $.t('Gas vs outside temperature'))) },
				xAxis: { type: 'datetime', dateTimeLabelFormats: { day: '%e %b', week: '%e %b', month: '%b' } },
				yAxis: yAxis,
				tooltip: {
					shared: true,
					useHTML: true,
					formatter: function () {
						var self = this;
						var html = '<b>' + Highcharts.dateFormat(xFormat, self.x) + ' ' + $scope.year + (curByX[self.x] && curByX[self.x].partial ? ' (' + $.t('so far') + ')' : '') + '</b><br/>';
						self.points.forEach(function (pt) {
							var cb = curByX[self.x], o = pt.series.options;
							html += line(pt.series.color, pt.series.name, (cb && o.ewf) ? o.ewf(cb) : pt.y, (o.ewc && cb) ? o.ewc(cb) : null, (cb && o.ewg && o.ewg(cb)) ? '' : o.ewu);
						});
						html += costLine(costOf(curByX[self.x]));
						var pb = prevByX[self.x];
						html += '<div style="border-top:1px solid rgba(128,128,128,0.6); margin:5px 0 4px 0;"></div>';
						html += '<b>' + Highcharts.dateFormat(xFormat, self.x) + ' ' + $scope.compareYear + '</b><br/>';
						self.points.forEach(function (pt) {
							var o = pt.series.options;
							html += line(pt.series.color, pt.series.name, pb ? o.ewf(pb) : null, (o.ewc && pb) ? o.ewc(pb) : null, (pb && o.ewg && o.ewg(pb)) ? '' : o.ewu);
						});
						html += costLine(costOf(pb));
						return html;
					}
				},
				plotOptions: { column: { pointPadding: 0.05, groupPadding: 0.05 } },
				legend: { enabled: true },
				series: series
			});
		}

		function renderEfficiency(buckets, prevBuckets) {
			var threshold = minDD();
			function line(list) {
				return list.map(function (b) { return [b.x, b.dd >= threshold && !b.partial ? r2(b.usage / b.dd) : null]; });
			}
			$('#ewEfficiency').highcharts({
				title: { text: unit() + ' / ' + $.t('Degree Day') },
				xAxis: { type: 'datetime', dateTimeLabelFormats: { day: '%e %b', week: '%e %b', month: '%b' } },
				yAxis: { title: { text: unit() + ' / DD' }, min: 0 },
				tooltip: { shared: true, xDateFormat: $scope.gran === 'month' ? '%B' : '%e %B' },
				legend: { enabled: true },
				series: [
					{ type: 'spline', name: String($scope.year), color: $scope.mode === 'heat' ? '#ff9800' : '#03befc', data: line(buckets), connectNulls: false },
					{ type: 'spline', name: String($scope.compareYear), color: '#9e9e9e', dashStyle: 'Dash', data: line(prevBuckets), connectNulls: false }
				]
			});
		}

		function renderScatter(list, reg, cmpList, cmpReg) {
			var points = list.map(function (it) { return [r2(ddOf(it.v)), r2(usageOf(it.v))]; });
			var cmpPoints = (cmpList && $scope.compareYear !== $scope.year) ? cmpList.map(function (it) { return [r2(ddOf(it.v)), r2(usageOf(it.v))]; }) : [];
			var maxX = points.concat(cmpPoints).reduce(function (m, p) { return Math.max(m, p[0]); }, 0);
			var series = [];
			if (cmpPoints.length) {
				series.push({ type: 'scatter', name: String($scope.compareYear), color: 'rgba(158,158,158,0.45)', data: cmpPoints });
			}
			series.push({ type: 'scatter', name: String($scope.year), color: 'rgba(3,190,252,0.6)', data: points });
			if (cmpPoints.length && cmpReg) {
				series.push({
					type: 'line', name: $.t('Trend') + ' ' + $scope.compareYear, color: '#9e9e9e', dashStyle: 'Dash', marker: { enabled: false }, enableMouseTracking: false,
					data: [[0, r2(cmpReg.intercept)], [r2(maxX), r2(cmpReg.intercept + cmpReg.slope * maxX)]]
				});
			}
			if (reg) {
				series.push({
					type: 'line', name: $.t('Trend') + ' ' + $scope.year, color: '#e53935', marker: { enabled: false }, enableMouseTracking: false,
					data: [[0, r2(reg.intercept)], [r2(maxX), r2(reg.intercept + reg.slope * maxX)]]
				});
			}
			$('#ewScatter').highcharts({
				title: { text: $.t('Daily usage vs degree days') },
				xAxis: { title: { text: $.t('Degree Days') }, min: 0 },
				yAxis: { title: { text: unit() }, min: 0 },
				tooltip: { pointFormat: '{point.x} DD, {point.y} ' + unit() },
				legend: { enabled: true },
				series: series
			});
		}

		function render() {
			if (!settings || !cache[$scope.year] || !cache[$scope.compareYear]) { return; }
			var list = validDays($scope.year);
			var prevList = validDays($scope.compareYear);
			// compare with the same calendar days last year, only days that have data in both years
			var thisKeys = {};
			// today is only partly known, so it is drawn but never compared, fitted or ranked
			list.forEach(function (it) { if (!it.v.partial) { thisKeys[it.d.substring(5)] = true; } });
			var prevSame = prevList.filter(function (it) { return thisKeys[it.d.substring(5)]; });
			var prevKeys = {};
			prevSame.forEach(function (it) { prevKeys[it.d.substring(5)] = true; });
			var listSame = list.filter(function (it) { return prevKeys[it.d.substring(5)]; });

			$scope.noData = !list.length;
			if ($scope.noData) {
				$scope.tiles = null;
				return;
			}

			var buckets = bucketize(list);
			var prevBuckets = bucketize(prevList);

			// scatter and trend use the whole year of daily values, not just the visible month
			var yearAll = Object.keys(cache[$scope.year].days).sort().filter(function (d) {
				var u = usageOf(cache[$scope.year].days[d]);
				return u !== null && typeof u != 'undefined' && !cache[$scope.year].days[d].partial;
			}).map(function (d) { return { d: d, v: cache[$scope.year].days[d] }; });
			var reg = dzEnergyWeather.regression(yearAll.map(function (it) { return [ddOf(it.v), usageOf(it.v)]; }));

			var sum = function (arr, f) { return arr.reduce(function (s, it) { return s + f(it); }, 0); };
			var dd = sum(list, function (it) { return ddOf(it.v); });
			var usage = sum(list, function (it) { return usageOf(it.v); });
			var prevDD = sum(prevSame, function (it) { return ddOf(it.v); });
			var prevUsage = sum(prevSame, function (it) { return usageOf(it.v); });
			var perDD = dd > 0 ? usage / dd : null;
			var prevPerDD = prevDD > 0 ? prevUsage / prevDD : null;
			var sameDD = sum(listSame, function (it) { return ddOf(it.v); });
			var sameUsage = sum(listSame, function (it) { return usageOf(it.v); });
			var samePerDD = sameDD > 0 ? sameUsage / sameDD : null;

			$scope.tiles = {
				unit: unit(),
				dd: dd.toFixed(1),
				usage: usage.toFixed(1),
				perDD: perDD !== null ? perDD.toFixed(2) : '-',
				change: (samePerDD !== null && prevPerDD && $scope.compareYear !== $scope.year) ? (((samePerDD / prevPerDD) - 1) * 100) : null,
				prevPerDD: prevPerDD !== null ? prevPerDD.toFixed(2) : '-',
				baseload: reg ? reg.intercept.toFixed(1) : '-',
				slope: reg ? reg.slope.toFixed(2) : '-',
				r2: reg ? reg.r2.toFixed(2) : '-'
			};

			var compareAll = Object.keys(cache[$scope.compareYear].days).sort().filter(function (d) {
				var u = usageOf(cache[$scope.compareYear].days[d]);
				return u !== null && typeof u != 'undefined' && !cache[$scope.compareYear].days[d].partial;
			}).map(function (d) { return { d: d, v: cache[$scope.compareYear].days[d] }; });
			var compareReg = dzEnergyWeather.regression(compareAll.map(function (it) { return [ddOf(it.v), usageOf(it.v)]; }));

			computeExtras({
				list: list, listSame: listSame, prevSame: prevSame, yearAll: yearAll, compareAll: compareAll,
				reg: reg, compareReg: compareReg, usage: usage, sameUsage: sameUsage, prevUsage: prevUsage,
				sameDD: sameDD, prevDD: prevDD
			});

			$timeout(function () {
				renderCombined(buckets, prevBuckets);
				renderExpected(buckets, reg);
				renderEfficiency(buckets, prevBuckets);
				renderScatter(yearAll, reg, compareAll, compareReg);
			}, 0);
		}

		// Expected usage from the weather alone (baseload + slope x degree days) against what was really used
		function renderExpected(buckets, reg) {
			if (!reg) { $('#ewExpected').empty(); return; }
			$('#ewExpected').highcharts({
				title: { text: $.t('Actual vs expected from the weather') },
				xAxis: { type: 'datetime', dateTimeLabelFormats: { day: '%e %b', week: '%e %b', month: '%b' } },
				yAxis: { title: { text: unit() }, min: 0 },
				tooltip: { shared: true, xDateFormat: $scope.gran === 'month' ? '%B' : '%e %B' },
				plotOptions: { column: { pointPadding: 0.05, groupPadding: 0.05 } },
				legend: { enabled: true },
				series: [
					{ type: 'column', name: $.t('Actual'), color: $scope.mode === 'heat' ? 'rgba(255,152,0,0.85)' : 'rgba(3,190,252,0.8)',
					  data: buckets.map(function (b) { return [b.x, r2(b.usage)]; }) },
					{ type: 'spline', name: $.t('Expected'), color: '#e53935', marker: { enabled: true },
					  data: buckets.map(function (b) { return [b.x, r2(Math.max(0, reg.intercept * b.nFrac + reg.slope * b.dd))]; }) }
				]
			});
		}

		// Mindergas style yearly forecast, cost / CO2, weather versus other effect, balance temperature, per airco unit
		var CO2_GAS = 1.88;		// kg per m3 natural gas, approximate
		var CO2_ELEC = 0.4;		// kg per kWh, approximate grid average
		function computeExtras(c) {
			var heat = $scope.mode === 'heat';
			var both = $scope.mode === 'both';
			var extras = [];
			// the cost Domoticz logged per day for these meters, summed over the visible range
			var costGasSum = c.list.reduce(function (s, it) { return s + (it.v.costGas || 0); }, 0);
			var costElecSum = c.list.reduce(function (s, it) { return s + (it.v.costAirco || 0); }, 0);
			var costSum = both ? costGasSum + costElecSum : (heat ? costGasSum : costElecSum);
			var price = (costSum > 0 && c.usage > 0 && !both) ? costSum / c.usage : null;	// average price paid per unit
			var co2f = heat ? CO2_GAS : CO2_ELEC;
			// combined heating: cost and CO2 from the gas m3 and the airco kWh separately
			var partGasKwh = c.list.reduce(function (s, it) { return s + (typeof it.v.gas != 'undefined' ? it.v.gas * gasPerM3() : 0); }, 0);
			var partElec = c.list.reduce(function (s, it) { return s + (it.v.aircoTotal || 0); }, 0);
			var cur = $scope.currencySign || '';
			var sumOf = function (arr, f) { return arr.reduce(function (s, it) { return s + f(it); }, 0); };

			// forecast of the full year (only meaningful for the running year)
			if ($scope.year === currentYear) {
				var fcReg = (c.reg && c.reg.n >= 30) ? c.reg : c.compareReg;
				if (fcReg) {
					var normal = {};	// mm-dd -> degree days of the comparison year
					Object.keys(cache[$scope.compareYear].days).forEach(function (d) { normal[d.substring(5)] = ddOf(cache[$scope.compareYear].days[d]); });
					var actual = sumOf(c.yearAll, function (it) { return usageOf(it.v); });
					var rest = 0;
					var dt = new Date();
					for (var i = 0; i < 366; i++) {
						var day = new Date(dt.getFullYear(), dt.getMonth(), dt.getDate() + i);
						if (day.getFullYear() !== currentYear) { break; }
						var key = ('0' + (day.getMonth() + 1)).slice(-2) + '-' + ('0' + day.getDate()).slice(-2);
						rest += Math.max(0, fcReg.intercept + fcReg.slope * (normal[key] || 0));
					}
					var total = actual + rest;
					var cmpTotal = sumOf(c.compareAll, function (it) { return usageOf(it.v); });
					extras.push({
						label: $.t('Forecast full year'),
						value: total.toFixed(0) + ' ' + unit(),
						sub: (cmpTotal > 0 ? ($.t('Compared to') + ' ' + $scope.compareYear + ': ' + cmpTotal.toFixed(0) + ' ' + unit()) : '')
					});
				}
			}

			// cost and CO2 of the visible range
			if (both) {
				var gasM3 = partGasKwh / gasPerM3();
				if (costSum > 0) {
					extras.push({ label: $.t('Cost'), value: cur + ' ' + costSum.toFixed(2), sub: $.t('Gas') + ' ' + cur + ' ' + costGasSum.toFixed(2) + ', ' + $.t('Heat Pump / Airco') + ' ' + cur + ' ' + costElecSum.toFixed(2) });
				}
				extras.push({ label: 'CO₂ (' + $.t('approx.') + ')', value: (gasM3 * CO2_GAS + partElec * CO2_ELEC).toFixed(0) + ' kg', sub: CO2_GAS + ' kg / m3, ' + CO2_ELEC + ' kg / kWh' });
				if (c.usage > 0) {
					extras.unshift({ label: $.t('Heat pump / airco share of the heating'), value: (partElec / c.usage * 100).toFixed(0) + ' %', sub: partElec.toFixed(0) + ' kWh ' + $.t('of') + ' ' + c.usage.toFixed(0) + ' kWh' });
				}
			} else {
				if (price) {
					extras.push({ label: $.t('Cost'), value: cur + ' ' + costSum.toFixed(2), sub: price.toFixed(3) + ' ' + cur + ' / ' + unit() });
				}
				extras.push({ label: 'CO₂ (' + $.t('approx.') + ')', value: (c.usage * co2f).toFixed(0) + ' kg', sub: co2f + ' kg / ' + unit() });
			}

			// change versus the comparison year, split into weather and other
			if (c.compareReg && c.prevSame.length && c.listSame.length && $scope.compareYear !== $scope.year) {
				var weather = (c.sameDD - c.prevDD) * c.compareReg.slope;	// what the colder or warmer weather alone should have changed
				var total2 = c.sameUsage - c.prevUsage;
				var other = total2 - weather;
				var fmt = function (v) { return (v > 0 ? '+' : '') + v.toFixed(1) + ' ' + unit(); };
				// the cost change is the difference of the costs Domoticz logged on the same calendar days
				var dayCost = function (it) { return both ? (it.v.costGas || 0) + (it.v.costAirco || 0) : (heat ? (it.v.costGas || 0) : (it.v.costAirco || 0)); };
				// only days with a logged cost in both years are compared, so a price added mid-year cannot skew it
				var costKeys = {};
				c.prevSame.forEach(function (it) { if (dayCost(it) > 0) { costKeys[it.d.substring(5)] = true; } });
				var pairedSame = c.listSame.filter(function (it) { return dayCost(it) > 0 && costKeys[it.d.substring(5)]; });
				var pairedKeys = {};
				pairedSame.forEach(function (it) { pairedKeys[it.d.substring(5)] = true; });
				var pairedPrev = c.prevSame.filter(function (it) { return pairedKeys[it.d.substring(5)]; });
				var sameCost = sumOf(pairedSame, dayCost), prevCost = sumOf(pairedPrev, dayCost);
				var costDelta = sameCost - prevCost;
				var costText = (sameCost > 0 && prevCost > 0) ?', ' + $.t('Cost') + ' ' + (costDelta >= 0 ? '+' : '-') + cur + ' ' + Math.abs(costDelta).toFixed(2) : '';	// only when both years have a logged cost
				extras.push({
					label: $.t('Change vs') + ' ' + $scope.compareYear,
					value: fmt(total2),
					sub: $.t('Weather') + ' ' + fmt(weather) + ', ' + $.t('other') + ' ' + fmt(other) + costText
				});
			}

			if ($scope.mode !== 'cool') {
				// balance temperature: where the fitted heating use drops to the summer baseload
				var cold = c.yearAll.filter(function (it) { return it.v.t < settings.baseHeating; });
				var warm = c.yearAll.filter(function (it) { return it.v.t >= settings.baseHeating; });
				var tReg = dzEnergyWeather.regression(cold.map(function (it) { return [it.v.t, usageOf(it.v)]; }));
				if (tReg && tReg.slope < 0 && warm.length >= 10) {
					var base0 = sumOf(warm, function (it) { return usageOf(it.v); }) / warm.length;
					var tBal = (base0 - tReg.intercept) / tReg.slope;
					if (tBal > -5 && tBal < settings.baseHeating + 5) {
						extras.push({ label: $.t('Balance temperature'), value: tBal.toFixed(1) + ' °' + $.myglobals.tempsign, sub: $.t('Heating starts below this outside temperature') });
					}
				}
			}
			if (!heat) {
				// per heat pump / airco unit: kWh per degree day over the visible range
				settings.idAirco.forEach(function (idx) {
					var days = c.list.filter(function (it) { return ddOf(it.v) > 0 && typeof it.v.airco[idx] != 'undefined'; });
					var degDays = sumOf(days, function (it) { return ddOf(it.v); });
					if (degDays > 0) {
						extras.push({ label: unitNames[idx] || ('#' + idx), value: (sumOf(days, function (it) { return it.v.airco[idx]; }) / degDays).toFixed(2) + ' kWh / DD', sub: '' });
					}
				});
			}
			$scope.extras = extras;

			// days that used the most above what the weather explains
			if (c.reg) {
				$scope.outliers = c.list.filter(function (it) { return !it.v.partial; }).map(function (it) {
					var exp = Math.max(0, c.reg.intercept + c.reg.slope * ddOf(it.v));
					return { date: it.d, usage: usageOf(it.v).toFixed(1), expected: exp.toFixed(1), diff: usageOf(it.v) - exp };
				}).filter(function (o) { return o.diff > 0; }).sort(function (a, b) { return b.diff - a.diff; }).slice(0, 5).map(function (o) {
					o.diffText = '+' + o.diff.toFixed(1);
					return o;
				});
			} else {
				$scope.outliers = [];
			}
		}

		function ensureYear(year) {
			if (cache[year]) { return $q.resolve(); }
			return dzEnergyWeather.loadYear(settings, year).then(function (res) { cache[year] = res; });
		}

		var refreshSeq = 0;
		var loadedAt = Date.now();
		function refresh() {
			if (!settings) { return; }
			$scope.loading = true;
			loadedAt = Date.now();
			var seq = ++refreshSeq;
			$q.all([ensureYear($scope.year), ensureYear($scope.compareYear)]).then(function () {
				if (seq !== refreshSeq) { return; }	// a newer selection is already loading
				$scope.loading = false;
				$scope.problem = '';
				render();
			}, function () {
				if (seq !== refreshSeq) { return; }
				$scope.loading = false;
				$scope.problem = $.t('Failed to load data');
			});
		}

		// a page left open must pick up the new day at midnight, and today's partial values every few minutes
		var loadedDay = new Date().toDateString();
		var REFRESH_MS = 5 * 60 * 1000;
		$scope.$on('time_update', function () {
			var today = new Date().toDateString();
			if (loadedDay !== today) {
				loadedDay = today;
				var oldYear = currentYear;
				currentYear = new Date().getFullYear();
				cache = {};
				if (!settings) { return; }	// still loading: the first load already uses the new day
				if (currentYear === oldYear) { refresh(); return; }
				// a new year: list it and follow the current year when that was the selection
				var applyYears = function (years) {
					$scope.years = years;
					if ($scope.year === oldYear) { $scope.year = currentYear; }
					if (years.indexOf($scope.compareYear) === -1) { $scope.compareYear = years.length > 1 ? years[1] : years[0]; }
					refresh();
				};
				// when the probe fails the new year is added to the list we already have
				dzEnergyWeather.availableYears(settings).then(applyYears, function () { applyYears([currentYear].concat($scope.years.filter(function (y) { return y !== currentYear; }))); });
			} else if (Date.now() - loadedAt > REFRESH_MS && ($scope.year === currentYear || $scope.compareYear === currentYear) && cache[currentYear]) {
				// today's partial values: fetch quietly and swap them in, the charts stay on screen and an error is ignored
				loadedAt = Date.now();
				var quietYear = currentYear;
				dzEnergyWeather.loadYear(settings, quietYear).then(function (res) {
					if (quietYear !== currentYear || loadedDay !== new Date().toDateString()) { return; }
					cache[quietYear] = res;
					render();
				}, function () { /* keep what is on screen, try again after the next interval */ });
			}
		});

		// easter egg: five quick clicks on the title show the coldest day of the year
		var eggClicks = 0, eggTimer = null;
		$(document).on('click.ewegg', '#energyweathercontent h1', function () {
			eggClicks++;
			clearTimeout(eggTimer);
			eggTimer = setTimeout(function () { eggClicks = 0; }, 2500);
			if (eggClicks < 5) { return; }
			eggClicks = 0;
			var EE = window.dzEasterEggs;
			var data = cache[$scope.year];
			if (!EE || !EE.isEnabled() || !data) { return; }
			var coldest = null;
			Object.keys(data.days).forEach(function (d) {
				var day = data.days[d], u = usageOf(day);
				if (day.partial || u === null || typeof u == 'undefined') { return; }
				if (!coldest || day.t < coldest.t) { coldest = { year: $scope.year, date: d, t: day.t, usage: u, unit: unit() }; }
			});
			EE._triggerEnergyWeather(coldest);
		});
		$scope.$on('$destroy', function () { $(document).off('click.ewegg'); clearTimeout(eggTimer); });

		$scope.setMode = function (m) { $scope.mode = m; render(); };
		$scope.gasKwhChanged = function () {
			if (!($scope.gasKwh > 0)) { return; }	// usageOf falls back to 9.77 while the field is empty
			try { localStorage.setItem('ew_gaskwh', String($scope.gasKwh)); } catch (e) { /* storage blocked */ }
			render();
		};
		$scope.setGran = function (g) { $scope.gran = g; render(); };
		$scope.$watchGroup(['month', 'split'], function () { render(); });
		// picking a year compares it with the year before it; the oldest available year has none, so it compares with itself
		$scope.yearChanged = function () {
			$scope.compareYear = ($scope.years.indexOf($scope.year - 1) !== -1) ? $scope.year - 1 : $scope.year;
		};

		$scope.$watchGroup(['year', 'compareYear'], function (n, o) { if (n[0] !== o[0] || n[1] !== o[1]) { refresh(); } });

		dzEnergyWeather.loadSettings().then(function (s) {
			settings = s;
			$scope.hasGas = s.idGas >= 0;
			$scope.hasAirco = s.idAirco.length > 0;
			$scope.multiAirco = s.idAirco.length > 1;
			if (s.idOutside < 0) {
				$scope.problem = $.t('Please select an Outside Temperature Sensor in the Energy Dashboard settings.');
				$scope.loading = false;
				return;
			}
			if (!$scope.hasGas && $scope.hasAirco) { $scope.mode = 'hp'; }
			var jobs = s.idAirco.map(function (idx) {
				return dzEnergyWeather.deviceName(idx).then(function (n) { unitNames[idx] = n; });
			});
			jobs.push(dzEnergyWeather.availableYears(s).then(function (years) {
				$scope.years = years;
				// keep the comparison year valid: the previous year when it has data, otherwise the newest older one
				if (years.indexOf($scope.compareYear) === -1) { $scope.compareYear = years.length > 1 ? years[1] : years[0]; }
			}, function () { /* keep the default year list when the probe fails */ }));
			$q.all(jobs).then(refresh);
		}, function () {
			$scope.problem = $.t('Failed to load data');
			$scope.loading = false;
		});
	}]);
});
