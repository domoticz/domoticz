define(['app'], function (app) {
	'use strict';

	// Weather corrected energy figures: daily gas / airco / grid usage joined with the daily average
	// temperature of the Energy Dashboard outside sensor, turned into (weighted) degree days.
	// Shared by the Energy vs Weather page and the dynamic dashboard widget.
	app.factory('dzEnergyWeather', ['$http', '$q', function ($http, $q) {
		// Dutch weighting for heating degree days: November to February 1.1, March and October 1.0, April to September 0.8
		var WEIGHTS = [1.1, 1.1, 1.0, 0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 1.0, 1.1, 1.1];

		function pad(n) {
			return (n < 10 ? '0' : '') + n;
		}

		function todayStr() {
			var n = new Date();
			return n.getFullYear() + '-' + pad(n.getMonth() + 1) + '-' + pad(n.getDate());
		}

		function graph(idx, sensor, year) {
			return $http.get('json.htm', {
				params: { type: 'command', param: 'graph', sensor: sensor, idx: idx, range: 'year', actyear: year }
			}).then(function (resp) {
				return (resp.data && resp.data.result) ? resp.data.result : [];
			});
		}

		function deviceName(idx) {
			return $http.get('json.htm', { params: { type: 'command', param: 'getdevices', rid: idx } }).then(function (resp) {
				var d = resp.data && resp.data.result && resp.data.result[0];
				return d ? d.Name : ('#' + idx);
			}, function () {
				return '#' + idx;
			});
		}

		function loadSettings() {
			return $http.get('json.htm?type=command&param=getenergydashboarddevices').then(function (resp) {
				var s = (resp.data && resp.data.result && resp.data.result.ESettings) || {};
				return {
					idOutside: (typeof s.idOutsideTempSensor != 'undefined') ? s.idOutsideTempSensor : -1,
					idGas: (typeof s.idGas != 'undefined') ? s.idGas : -1,
					idP1: (typeof s.idP1 != 'undefined') ? s.idP1 : -1,
					idAirco: Array.isArray(s.idAirco) ? s.idAirco : [],
					weighted: s.DegreeDaysWeighted == 1,
					baseHeating: parseFloat($.myglobals.DegreeDaysBaseTemperature),
					baseCooling: parseFloat($.myglobals.CoolingDegreeDaysBaseTemperature)
				};
			});
		}

		// Daily usage of a counter device as { 'YYYY-MM-DD': value }, P1 meters add their second tariff (import only)
		function loadUsage(idx, year) {
			return graph(idx, 'counter', year).then(function (rows) {
				var out = {};
				rows.forEach(function (r) {
					// a P1 smart meter reports its first tariff as v1 (import), other meters as v
					var v = parseFloat(typeof r.v != 'undefined' ? r.v : r.v1);
					if (isNaN(v)) { return; }
					var v2 = parseFloat(r.v2);
					out[r.d] = v + (isNaN(v2) ? 0 : v2);
				});
				return out;
			});
		}

		function loadTemps(idx, year) {
			return graph(idx, 'temp', year).then(function (rows) {
				var out = {};
				rows.forEach(function (r) {
					var t = parseFloat(r.ta);
					if (!isNaN(t)) { out[r.d] = t; }
				});
				return out;
			});
		}

		// Loads one year. Resolves to { days: { date: { t, hdd, cdd, gas, grid, airco: {idx: v}, aircoTotal } } }
		var yearCache = {};	// settings + year -> { time, promise }, shared by every widget and the page
		var CACHE_MS = 60000;

		function loadYear(settings, year) {
			var key = JSON.stringify(settings) + '|' + year;
			var hit = yearCache[key];
			if (hit && (Date.now() - hit.time) < CACHE_MS) { return hit.promise; }
			var promise = loadYearUncached(settings, year);
			yearCache[key] = { time: Date.now(), promise: promise };
			promise.catch(function () { delete yearCache[key]; });
			return promise;
		}

		function loadYearUncached(settings, year) {
			if (settings.idOutside < 0) { return $q.resolve({ days: {} }); }
			var jobs = {
				temps: loadTemps(settings.idOutside, year),
				gas: settings.idGas >= 0 ? loadUsage(settings.idGas, year) : $q.resolve({}),
				grid: settings.idP1 >= 0 ? loadUsage(settings.idP1, year) : $q.resolve({}),
				airco: $q.all(settings.idAirco.map(function (idx) { return loadUsage(idx, year); }))
			};
			return $q.all(jobs).then(function (r) {
				var today = todayStr();
				var days = {};
				Object.keys(r.temps).forEach(function (d) {
					if (d >= today) { return; }	// today is still incomplete
					var t = r.temps[d];
					var month = parseInt(d.substring(5, 7), 10) - 1;
					var hdd = Math.max(0, settings.baseHeating - t) * (settings.weighted ? WEIGHTS[month] : 1);
					var cdd = Math.max(0, t - settings.baseCooling);
					var day = { t: t, hdd: hdd, cdd: cdd, airco: {}, aircoTotal: null };
					if (typeof r.gas[d] != 'undefined') { day.gas = r.gas[d]; }
					if (typeof r.grid[d] != 'undefined') { day.grid = r.grid[d]; }
					settings.idAirco.forEach(function (idx, i) {
						if (typeof r.airco[i][d] != 'undefined') {
							day.airco[idx] = r.airco[i][d];
							day.aircoTotal = (day.aircoTotal || 0) + r.airco[i][d];
						}
					});
					days[d] = day;
				});
				return { days: days };
			});
		}

		// Least squares fit usage = intercept + slope * degreeDays. points: [[dd, usage], ...]
		function regression(points) {
			var n = points.length;
			if (n < 3) { return null; }
			var sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0;
			points.forEach(function (p) {
				sx += p[0]; sy += p[1]; sxx += p[0] * p[0]; sxy += p[0] * p[1]; syy += p[1] * p[1];
			});
			var den = n * sxx - sx * sx;
			if (den === 0) { return null; }
			var slope = (n * sxy - sx * sy) / den;
			var intercept = (sy - slope * sx) / n;
			var ssTot = syy - sy * sy / n;
			var ssRes = 0;
			points.forEach(function (p) {
				var e = p[1] - (intercept + slope * p[0]);
				ssRes += e * e;
			});
			return { slope: slope, intercept: intercept, r2: ssTot > 0 ? 1 - ssRes / ssTot : 0, n: n };
		}

		// Price per m3 (gas) and per kWh (electricity, tariff 1) from the cost settings; null when not set
		function loadPrices(settings) {
			function costs(idx) {
				if (idx < 0) { return $q.resolve(null); }
				return $http.get('json.htm', { params: { type: 'command', param: 'getcosts', idx: idx } }).then(function (resp) {
					return resp.data || null;
				}, function () { return null; });
			}
			var elecIdx = settings.idAirco.length ? settings.idAirco[0] : settings.idP1;
			return $q.all([costs(settings.idGas), costs(elecIdx)]).then(function (r) {
				var gas = r[0] && parseFloat(r[0].CostGas) / 10000;
				var elec = r[1] && parseFloat(r[1].CostEnergy) / 10000;
				return { gas: gas > 0 ? gas : null, elec: elec > 0 ? elec : null };
			});
		}

		// Years that have outside temperature data (a year without it cannot be used), newest first.
		// Probes backwards from the current year and stops at the first year without data.
		var yearsCache = {};	// outside sensor + current year -> promise, past years do not change
		function availableYears(settings) {
			var key = settings.idOutside + '|' + new Date().getFullYear();
			if (!yearsCache[key]) {
				yearsCache[key] = probeYears(settings);
				yearsCache[key].catch(function () { delete yearsCache[key]; });
			}
			return yearsCache[key];
		}

		function probeYears(settings) {
			var current = new Date().getFullYear();
			var years = [current];
			function probe(year) {
				if (settings.idOutside < 0 || year < current - 20) { return $q.resolve(years); }
				return graph(settings.idOutside, 'temp', year).then(function (rows) {
					if (!rows.length) { return years; }
					years.push(year);
					return probe(year - 1);
				}, function () { return years; });
			}
			return probe(current - 1);
		}

		return {
			weights: WEIGHTS,
			availableYears: availableYears,
			loadPrices: loadPrices,
			loadSettings: loadSettings,
			loadYear: loadYear,
			deviceName: deviceName,
			regression: regression
		};
	}]);
});
