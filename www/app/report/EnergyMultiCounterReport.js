define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceEnergyMultiCounterReportData', function ($q, domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(deviceIdx, year, month, customStartDate) {
            if (customStartDate && !/^\d{4}-\d{2}-\d{2}$/.test(customStartDate)) {
                return $q.resolve(null);
            }
            var costs = domoticzApi.sendCommand('getcosts', { idx: deviceIdx });

            var graphParams;
            if (customStartDate) {
                var actend = reportHelpers.addOneYear(customStartDate);
                if (!actend) { return $q.resolve(null); }
                graphParams = { sensor: 'counter', range: 'year', idx: deviceIdx,
                    actstart: customStartDate, actend: actend };
            } else {
                graphParams = { sensor: 'counter', range: 'year', idx: deviceIdx,
                    actyear: year, actmonth: month };
            }

            var stats = domoticzApi.sendCommand('graph', graphParams);

            var allPromises;
            if (customStartDate) {
                var prevStart = reportHelpers.addOneYear(customStartDate, -1);
                if (!prevStart) { return $q.resolve(null); }
                var prevStats = domoticzApi.sendCommand('graph', {
                    sensor: 'counter', range: 'year', idx: deviceIdx,
                    actstart: prevStart, actend: customStartDate
                });
                allPromises = $q.all([costs, stats, prevStats]);
            } else {
                allPromises = $q.all([costs, stats]);
            }

            return allPromises.then(function (responses) {
                var costs = responses[0];
                var stats = responses[1];
                var prevStats = responses[2] || null;

                if (!stats.result || !stats.result.length) {
                    return null;
                }

                var includeReturn = typeof stats.delivered !== 'undefined';
                var P1DisplayType = stats.P1DisplayType;

                var source;
                if (customStartDate) {
                    var prevYearData = (prevStats && prevStats.result && prevStats.result.length)
                        ? prevStats.result
                        : null;
                    source = getContractMonthData(stats.result, costs, includeReturn, customStartDate, prevYearData);
                } else {
                    var data = getGroupedData(stats.result, costs, includeReturn);
                    source = month
                        ? data.years[year].months.find(function (item) {
                            return (new Date(item.date)).getUTCMonth() + 1 === month;
                          })
                        : data.years[year];
                }

                if (!source) {
                    return null;
                }

                return Object.assign({}, source, {
                    items: customStartDate ? source.months : (month ? source.days : source.months),
                    P1DisplayType: P1DisplayType,
                    customStartDate: customStartDate
                });
            });
        }

        function buildDayRecord(item, costs, includeReturn) {
            var dayRecord = {
                date: item.d,
                usage: {
                    usage: parseFloat(item.v),
                    cost: parseFloat(item.v) * costs.CostEnergy / 10000,
                    counter: parseFloat(item.c1) || 0
                },
                usage1: {
                    usage: parseFloat(item.v1),
                    cost: parseFloat(item.v1) * costs.CostEnergy / 10000,
                    counter: parseFloat(item.c1) || 0
                },
                usage2: {
                    usage: parseFloat(item.v2),
                    cost: parseFloat(item.v2) * costs.CostEnergyT2 / 10000,
                    counter: parseFloat(item.c3) || 0
                },
                price: parseFloat(item.p)
            };

            if (includeReturn) {
                dayRecord.return = {
                    usage: parseFloat(item.r),
                    cost: parseFloat(item.r) * costs.CostEnergyR1 / 10000,
                    counter: parseFloat(item.c2) || 0,
                };
                dayRecord.return1 = {
                    usage: parseFloat(item.r1),
                    cost: parseFloat(item.r1) * costs.CostEnergyR1 / 10000,
                    counter: parseFloat(item.c2) || 0
                };
                dayRecord.return2 = {
                    usage: parseFloat(item.r2),
                    cost: parseFloat(item.r2) * costs.CostEnergyR2 / 10000,
                    counter: parseFloat(item.c4) || 0
                };

                dayRecord.totalReturn = dayRecord.return1.usage + dayRecord.return2.usage;
            }

            dayRecord.totalUsage = dayRecord.usage1.usage + dayRecord.usage2.usage;
            dayRecord.usage = dayRecord.totalUsage -
                (includeReturn ? dayRecord.totalReturn : 0);
            dayRecord.cost = (dayRecord.price != 0) ? dayRecord.price : (dayRecord.usage1.cost + dayRecord.usage2.cost -
                (includeReturn ? (dayRecord.return1.cost + dayRecord.return2.cost) : 0));

            return dayRecord;
        }

        function getContractMonthData(rawData, costs, includeReturn, startDateISO, prevYearData) {
            var base = new Date(startDateISO + 'T00:00:00');
            var subKeys = ['usage1', 'usage2'];
            if (includeReturn) { subKeys = subKeys.concat(['return1', 'return2']); }

            var today = new Date();
            today.setHours(0, 0, 0, 0);

            var periods = [];
            for (var i = 0; i < 12; i++) {
                var pStart = new Date(base.getFullYear(), base.getMonth() + i, base.getDate());
                var pEnd   = new Date(base.getFullYear(), base.getMonth() + i + 1, base.getDate());
                // Clamp pEnd if month overflowed (e.g. Jan 31 → Mar 3 instead of Feb 28)
                var expectedEndMonth = (base.getMonth() + i + 1) % 12;
                if (pEnd.getMonth() !== expectedEndMonth) {
                    pEnd = new Date(base.getFullYear(), base.getMonth() + i + 2, 0);
                }
                var p = {
                    start: pStart, end: pEnd,
                    label: reportHelpers.formatContractMonthLabel(pStart, pEnd),
                    date: +pStart, periodIndex: i + 1,
                    days: [], totalUsage: 0, totalReturn: 0, usage: 0, cost: 0,
                    forecast: false
                };
                subKeys.forEach(function(k) { p[k] = { usage: 0, cost: 0, counter: 0 }; });
                periods.push(p);
            }

            rawData.forEach(function(item) {
                var d = new Date(item.d.substring(0, 10) + 'T00:00:00');
                for (var i = 0; i < periods.length; i++) {
                    if (d >= periods[i].start && d < periods[i].end) {
                        var dayRecord = buildDayRecord(item, costs, includeReturn);
                        periods[i].days.push(dayRecord);
                        subKeys.forEach(function(k) {
                            if (dayRecord[k]) {
                                periods[i][k].usage  += dayRecord[k].usage;
                                periods[i][k].cost   += dayRecord[k].cost;
                                periods[i][k].counter = Math.max(periods[i][k].counter, dayRecord[k].counter);
                            }
                        });
                        periods[i].totalUsage  += dayRecord.totalUsage || 0;
                        periods[i].totalReturn += dayRecord.totalReturn || 0;
                        periods[i].usage       += dayRecord.usage || 0;
                        periods[i].cost        += dayRecord.cost || 0;
                        break;
                    }
                }
            });

            // Determine which periods are entirely in the future
            var futurePeriods = periods.filter(function(p) { return p.start >= today; });
            var hasFuturePeriods = futurePeriods.length > 0;

            // TODO: implement meter replacement detection
            // Currently always false; counter-reset detection was removed when switching
            // to the daily-sum approach. As a result, the meterReplaced warning banner
            // and the '~value ⚠' display in table cells will never show.
            var meterReplaced = false;
            var noHistory = false;
            var forecastFullYear = null;   // { t1, t2, r1, r2, total }
            var prevMonthBuckets = null;

            if (hasFuturePeriods && prevYearData) {
                var prevStartISO = reportHelpers.addOneYear(startDateISO, -1);

                // Sum daily v1/v2/r1/r2 values from the previous year — this is
                // robust against the "today-partial" record appended by the backend
                // (that record's c1/c3 reflect the current live counter, not the
                // counter at actend, which would corrupt any counter-delta approach).
                // Intentionally one level of recursion: this (current-year) call passes
                // prevYearData, but that recursive call receives no prevYearData so it
                // will not recurse further. prevYearData contains raw API records (no
                // forecast flags), making it safe to aggregate directly.
                var prevAgg = getContractMonthData(prevYearData, costs, includeReturn, prevStartISO);
                var delta = {
                    t1: prevAgg.usage1 ? prevAgg.usage1.usage : 0,
                    t2: prevAgg.usage2 ? prevAgg.usage2.usage : 0,
                    r1: (includeReturn && prevAgg.return1) ? prevAgg.return1.usage : 0,
                    r2: (includeReturn && prevAgg.return2) ? prevAgg.return2.usage : 0
                };
                delta.total = delta.t1 + delta.t2;
                prevMonthBuckets = prevAgg.months;

                // Less than 50 kWh for a full year suggests missing/incomplete previous
                // year data rather than actual consumption — disable forecast in that case.
                var MIN_YEARLY_KWH = 50;
                if (delta.total < MIN_YEARLY_KWH) {
                    noHistory = true;
                } else {
                    // Pre-compute forecast full-year cost from tariff rates
                    delta.forecastCost = delta.t1 * costs.CostEnergy / 10000
                                       + delta.t2 * costs.CostEnergyT2 / 10000
                                       - (includeReturn
                                           ? (delta.r1 * costs.CostEnergyR1 / 10000
                                              + delta.r2 * costs.CostEnergyR2 / 10000)
                                           : 0);

                    forecastFullYear = delta;

                    // Apply forecast to future periods
                    var prevTotal = delta.total;

                    futurePeriods.forEach(function(p) {
                        var periodIdx = periods.indexOf(p);
                        var prevBucket = (prevMonthBuckets && prevMonthBuckets[periodIdx]) || null;
                        var share = (prevBucket && prevTotal > 0)
                            ? prevBucket.totalUsage / prevTotal
                            : 1 / 12;

                        p.forecast = true;
                        p.meterReplaced = meterReplaced;

                        // Forecast usage values
                        p.usage1 = {
                            usage: delta.t1 * share,
                            cost:  delta.t1 * share * costs.CostEnergy / 10000,
                            counter: 0
                        };
                        p.usage2 = {
                            usage: delta.t2 * share,
                            cost:  delta.t2 * share * costs.CostEnergyT2 / 10000,
                            counter: 0
                        };
                        p.totalUsage = p.usage1.usage + p.usage2.usage;

                        if (includeReturn) {
                            p.return1 = {
                                usage: delta.r1 * share,
                                cost:  delta.r1 * share * costs.CostEnergyR1 / 10000,
                                counter: 0
                            };
                            p.return2 = {
                                usage: delta.r2 * share,
                                cost:  delta.r2 * share * costs.CostEnergyR2 / 10000,
                                counter: 0
                            };
                            p.totalReturn = p.return1.usage + p.return2.usage;
                        } else {
                            p.totalReturn = 0;
                        }

                        p.usage = p.totalUsage - p.totalReturn;
                        p.cost  = p.usage1.cost + p.usage2.cost
                                - (includeReturn ? (p.return1.cost + p.return2.cost) : 0);
                    });
                }
            } else if (hasFuturePeriods && !prevYearData) {
                noHistory = true;
            }

            periods = reportHelpers.addTrendData(periods, 'usage');

            var agg = {
                months: periods,
                meterReplaced: meterReplaced,
                noHistory: noHistory && hasFuturePeriods,
                forecastFullYear: forecastFullYear
            };
            subKeys.forEach(function(k) { agg[k] = { usage: 0, cost: 0, counter: 0 }; });

            // Aggregate only actual (non-forecast) periods for the summary block
            periods.forEach(function(p) {
                if (!p.forecast) {
                    subKeys.forEach(function(k) {
                        agg[k].usage  += p[k].usage;
                        agg[k].cost   += p[k].cost;
                        agg[k].counter = Math.max(agg[k].counter, p[k].counter);
                    });
                    agg.totalUsage  = (agg.totalUsage  || 0) + p.totalUsage;
                    agg.totalReturn = (agg.totalReturn || 0) + p.totalReturn;
                    agg.usage       = (agg.usage       || 0) + p.usage;
                    agg.cost        = (agg.cost        || 0) + p.cost;
                } else {
                    // Still initialise the accumulators if not yet set (all-forecast edge case)
                    subKeys.forEach(function(k) {
                        if (!agg[k]) { agg[k] = { usage: 0, cost: 0, counter: 0 }; }
                    });
                    if (agg.totalUsage  === undefined) { agg.totalUsage  = 0; }
                    if (agg.totalReturn === undefined) { agg.totalReturn = 0; }
                    if (agg.usage       === undefined) { agg.usage       = 0; }
                    if (agg.cost        === undefined) { agg.cost        = 0; }
                }
            });

            // Update forecast summary: actual to-date + sum of forecast remaining months
            // (rather than showing just last year's total as the full-year estimate)
            if (forecastFullYear && futurePeriods.length > 0) {
                var fUsage = futurePeriods.reduce(function(s, p) { return s + (p.totalUsage || 0); }, 0);
                var fCost  = futurePeriods.reduce(function(s, p) { return s + (p.cost  || 0); }, 0);
                forecastFullYear.total       = (agg.totalUsage || 0) + fUsage;
                forecastFullYear.forecastCost = (agg.cost || 0) + fCost;
            }

            return agg;
        }

        function getGroupedData(data, costs, includeReturn) {
            var result = {
                years: {}
            };

            data.forEach(function (item) {
                var month = parseInt(item.d.substring(5, 7), 10);
                var year = parseInt(item.d.substring(0, 4), 10);
                var day = parseInt(item.d.substring(8, 10), 10);

                if (!result.years[year]) {
                    result.years[year] = {
                        months: {}
                    }
                }

                if (!result.years[year].months[month]) {
                    result.years[year].months[month] = {
                        date: Date.UTC(year, month - 1, 1),
                        days: {}
                    }
                }

                var dayRecord = buildDayRecord(item, costs, includeReturn);
                result.years[year].months[month].days[day] = dayRecord;
            });

            Object.keys(result.years).forEach(function (year) {
                var yearsData = result.years[year];
                yearsData.months = Object.values(yearsData.months);

                yearsData.months.sort(function (a, b) {
                    return a.date < b.date ? -1 : 1;
                });

                yearsData.months.forEach(function (month) {
                    month.days = Object.values(month.days);

                    month.days.sort(function (a, b) {
                        return a.date < b.date ? -1 : 1;
                    });

                    month.days = reportHelpers.addTrendData(month.days, 'usage');
                    Object.assign(month, getGroupStats(month.days))
                });

                yearsData.months = reportHelpers.addTrendData(yearsData.months, 'usage');
                Object.assign(yearsData, getGroupStats(yearsData.months));
            });

            return result;
        }

        function getGroupStats(values) {
            var keys = [
                'usage1', 'usage2', 'return1', 'return2',
            ];

            return values.reduce(function (acc, item) {
                keys.forEach(function (key) {
                    if (!item[key]) {
                        return;
                    }

                    if (!acc[key]) {
                        acc[key] = {};
                    }

                    acc[key].usage = (acc[key].usage || 0) + item[key].usage;
                    acc[key].cost = (acc[key].cost || 0) + item[key].cost;
                    acc[key].counter = Math.max(acc[key].counter || 0, item[key].counter)
                });

                acc.totalUsage = (acc.totalUsage || 0) + item.totalUsage;
                acc.totalReturn = (acc.totalReturn || 0) + item.totalReturn;
                acc.usage = (acc.usage || 0) + item.usage;
                acc.cost = (acc.cost || 0) + item.cost;
                return acc;
            }, {});
        }
    });

    app.component('deviceEnergyMultiCounterReport', {
        bindings: {
            device:          '<',
            selectedYear:    '<',
            selectedMonth:   '<',
            customStartDate: '<'
        },
        templateUrl: 'app/report/EnergyMultiCounterReport.html',
        controller: DeviceCounterReportController
    });


    function DeviceCounterReportController($element, $scope, DeviceEnergyMultiCounterReportData, dataTableDefaultSettings) {
        var vm = this;
        vm.$onInit = init;

        vm.exportExcel     = function () { reportHelpers.exportTableToExcel($element, vm.device.Name + '_report'); };
        vm.exportCSV       = function () { reportHelpers.exportTableToCSV($element, vm.device.Name + '_report'); };
        vm.exportClipboard = function () { reportHelpers.exportTableToClipboard($element); };

        function init() {
            vm.unit = vm.device.getUnit();
            vm.currencySign = ($.myglobals.currencysign || '').replace(/[<>"'&]/g, '');
            vm.isMonthView = vm.selectedMonth > 0;
            vm.degreeType = $.myglobals.tempsign;

            $.devIdx = vm.device.idx;

            getData();

            var deregisterWatch = $scope.$watch(function() { return vm.customStartDate; }, function(newVal, oldVal) {
                if (newVal !== oldVal && /^\d{4}-\d{2}-\d{2}$/.test(newVal || '')) {
                    getData();
                }
            });
            $scope.$on('$destroy', deregisterWatch);
        }

        function getData() {
            DeviceEnergyMultiCounterReportData
                .fetch(vm.device.idx, vm.selectedYear, vm.selectedMonth, vm.customStartDate)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    vm.data = data;
                    vm.hasReturn = checkDataKey(data, 'return1');
					vm.P1DisplayType = data.P1DisplayType;
                    vm.forecastWarning = data.meterReplaced
                        ? $.t('Previous meter was replaced — forecast based on partial data. Actual values may differ.')
                        : null;
                    vm.noForecastHistory = data.noHistory || false;
                    vm.forecastFullYear = data.forecastFullYear || null;

                    showTable(data);
                    showUsageChart(data)
                });
        }

        function checkDataKey(data, key) {
            return data.items.every(function (item) {
                return item[key] !== undefined
            });
        }

        function showTable(data) {
			//console.log(data);
            var table = $element.find('#reporttable');
            // Destroy existing DataTable instance if present
            if ($.fn.dataTable.isDataTable(table)) {
                table.dataTable().api().destroy();
                table.empty();
            }
            var columns = [];

            var counterRenderer = function (data) {
                return (data || 0).toFixed(3);
            };

            var costRenderer = function (data) {
                return (data || 0).toFixed(2) + " " + $.myglobals.currencysign;
            };

            if (vm.isMonthView) {
                columns.push({
                    title: $.t('Day'),
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, 'UTC:d'));
                    }
                });

                columns.push({
                    title: '',
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, 'UTC:dddd'));
                    }
                });
            } else {
                columns.push({
                    title: $.t('Month'),
                    width: 150,
                    data: 'date',
                    render: function (data, type, row) {
                        if (type === 'sort' || type === 'type') { return data; }  // sort by raw timestamp
                        if (vm.customStartDate) {
                            return row.label || '';
                        }
                        var date = new Date(data);
                        var link = '<a href="#/Devices/' + vm.device.idx + '/Report/' + vm.selectedYear + '/' + (date.getUTCMonth() + 1) + '"><img src="images/next.png" /></a>';
                        return dateFormat(data, 'UTC:mm. mmmm') + ' ' + link;
                    }
                });
            }

			if (data.P1DisplayType == 0) {
				[
					['usage1', 'Usage T1', 'Counter T1'],
					['usage2', 'Usage T2', 'Counter T2'],
					['return1', 'Return T1', 'Counter R1'],
					['return2', 'Return T2', 'Counter R2']
				].forEach(function (item) {
					if (!checkDataKey(data, item[0])) {
						return;
					}
					if (vm.isMonthView) {
						columns.push({
							title: $.t(item[2]),
							data: item[0]+'.counter',
							render: function(counterRenderer) {
								return function(data, type, row) {
									if (row.forecast) { return '&mdash;'; }
									return counterRenderer(data);
								};
							}(counterRenderer)
						});
					}

					columns.push({
						title: $.t(item[1]),
						data: item[0]+'.usage',
						render: function (counterRenderer) {
							return function (val, type, row) {
								if (row.forecast) {
									return row.meterReplaced
										? '~' + counterRenderer(val) + ' <span class="report-forecast-warning" title="' + $.t('Meter replaced') + '">\u26a0</span>'
										: '~' + counterRenderer(val);
								}
								return counterRenderer(val);
							};
						}(counterRenderer)
					});
					columns.push({
						title: $.t('Costs'),
						data: item[0]+'.cost',
						render: function (costRenderer) {
							return function (val, type, row) {
								if (row.forecast) {
									return row.meterReplaced
										? '~' + costRenderer(val) + ' <span class="report-forecast-warning" title="' + $.t('Meter replaced') + '">\u26a0</span>'
										: '~' + costRenderer(val);
								}
								return costRenderer(val);
							};
						}(costRenderer)
					});
				});
			} else {
				columns.push({
					title: $.t('Usage'),
					data: 'totalUsage',
					render: function (val, type, row) {
						if (row.forecast) {
							return row.meterReplaced
								? '~' + counterRenderer(val) + ' <span class="report-forecast-warning" title="' + $.t('Meter replaced') + '">\u26a0</span>'
								: '~' + counterRenderer(val);
						}
						return counterRenderer(val);
					}
				});
				columns.push({
					title: $.t('Return'),
					data: 'totalReturn',
					render: function (val, type, row) {
						if (row.forecast) { return '~' + counterRenderer(val); }
						return counterRenderer(val);
					}
				});
				columns.push({
					title: $.t('Total'),
					data: 'usage',
					render: function (val, type, row) {
						if (row.forecast) { return '~' + counterRenderer(val); }
						return counterRenderer(val);
					}
				});
			}


            columns.push({
                title: $.t('Costs'),
                data: 'cost',
                render: function (val, type, row) {
                    if (row.forecast) {
                        return row.meterReplaced
                            ? '~' + costRenderer(val) + ' <span class="report-forecast-warning" title="' + $.t('Meter replaced') + '">\u26a0</span>'
                            : '~' + costRenderer(val);
                    }
                    return costRenderer(val);
                }
            });

            columns.push({
                title: '<>',
                orderable: false,
                data: 'trend',
                render: function (data) {
                    return '<img src="images/' + data + '.png">'
                }
            });

            table.dataTable(Object.assign({}, dataTableDefaultSettings, {
                sDom: '<"H"rC>t<"F">',
                columns: columns,
                pageLength: 50,
                order: [[0, 'asc']],
                createdRow: function (row, rowData) {
                    if (rowData.forecast) {
                        $(row).addClass('report-forecast-row');
                    }
                }
            }));

            table.dataTable().api().rows
                .add(data.items)
                .draw();

            // Grand-total footer row — only actual (non-forecast) items
            var actualItems = data.items.filter(function(r) { return !r.forecast; });
            var items = actualItems.length ? actualItems : data.items;

            function sumKey(items, key) {
                return items.reduce(function (s, r) {
                    var val = key.split('.').reduce(function (o, k) { return o && o[k]; }, r);
                    return s + (parseFloat(val) || 0);
                }, 0);
            }

            var cells = [];

            if (vm.isMonthView) {
                cells.push('<td style="font-weight:bold">' + $.t('Total') + '</td>');
                cells.push('<td></td>');
            } else {
                cells.push('<td style="font-weight:bold">' + $.t('Total') + '</td>');
            }

            if (data.P1DisplayType == 0) {
                var detailKeys = [
                    ['usage1', 'Usage T1', 'Counter T1'],
                    ['usage2', 'Usage T2', 'Counter T2'],
                    ['return1', 'Return T1', 'Counter R1'],
                    ['return2', 'Return T2', 'Counter R2']
                ];
                detailKeys.forEach(function (item) {
                    if (!checkDataKey(data, item[0])) {
                        return;
                    }
                    if (vm.isMonthView) {
                        var maxCounter = items.reduce(function (m, r) {
                            return Math.max(m, (r[item[0]] && r[item[0]].counter) || 0);
                        }, 0);
                        cells.push('<td style="font-weight:bold">' + counterRenderer(maxCounter) + '</td>');
                    }
                    cells.push('<td style="font-weight:bold">' + counterRenderer(sumKey(items, item[0] + '.usage')) + '</td>');
                    cells.push('<td style="font-weight:bold">' + costRenderer(sumKey(items, item[0] + '.cost')) + '</td>');
                });
            } else {
                cells.push('<td style="font-weight:bold">' + counterRenderer(sumKey(items, 'totalUsage')) + '</td>');
                cells.push('<td style="font-weight:bold">' + counterRenderer(sumKey(items, 'totalReturn')) + '</td>');
                cells.push('<td style="font-weight:bold">' + counterRenderer(sumKey(items, 'usage')) + '</td>');
            }

            cells.push('<td style="font-weight:bold">' + costRenderer(sumKey(items, 'cost')) + '</td>');
            cells.push('<td></td>');

            var tfoot = $('<tfoot><tr style="font-weight:bold; background:var(--dz-accent-color,#337ab7); color:var(--dz-body-text,#fff);">' + cells.join('') + '</tr></tfoot>');
            table.append(tfoot);
        }

		function reloadPage() {
			window.location.reload();
		}

        function showUsageChart(data) {
			let P1DisplayType = data.P1DisplayType;
            var chartElement = $element.find('#usagegraph');
            var hasUsage2 = checkDataKey(data, 'usage2');

            var series = [];

            // Separate actual vs forecast items for the chart
            var actualItems   = data.items.filter(function(r) { return !r.forecast; });
            var forecastItems = data.items.filter(function(r) { return  r.forecast; });

			if (P1DisplayType == 0) {
				series.push({
					name: hasUsage2 ? $.t('Usage') + ' 1' : $.t('Usage'),
					color: hasUsage2 ? 'rgba(60,130,252,0.8)' : 'rgba(3,190,252,0.8)',
					stack: 'susage',
					tooltip: {
						valueSuffix: 'kWh'
					},
					yAxis: 0,
					data: actualItems.map(function (item) {
						return {
							x: +(new Date(item.date)),
							y: item.usage1.usage
						}
					})
				});
				if (hasUsage2) {
					series.push({
						name: $.t('Usage') + ' 2',
						color: 'rgba(3,190,252,0.8)',
						stack: 'susage',
						tooltip: {
							valueSuffix: 'kWh'
						},
						yAxis: 0,
						data: actualItems.map(function (item) {
							return {
								x: +(new Date(item.date)),
								y: item.usage2.usage
							}
						})
					});
				}
				if (checkDataKey(data, 'return1')) {
					series.push({
						name: $.t('Return') + ' 1',
						color: 'rgba(30,242,110,0.8)',
						stack: 'susage',
						tooltip: {
							valueSuffix: 'kWh'
						},
						yAxis: 0,
						data: actualItems.map(function (item) {
							return {
								x: +(new Date(item.date)),
								y: -item.return1.usage
							}
						})
					});
				}
				if (checkDataKey(data, 'return2')) {
					series.push({
						name: $.t('Return') + ' 2',
						color: 'rgba(3,252,190,0.8)',
						stack: 'susage',
						tooltip: {
							valueSuffix: 'kWh'
						},
						yAxis: 0,
						data: actualItems.map(function (item) {
							return {
								x: +(new Date(item.date)),
								y: -item.return2.usage
							}
						})
					});
				}

                // Forecast series (dashed/lower opacity)
                if (forecastItems.length) {
                    series.push({
                        name: $.t('Usage') + ' 1 (' + $.t('Forecast') + ')',
                        color: 'rgba(60,130,252,0.35)',
                        stack: 'susage',
                        dashStyle: 'Dash',
                        borderWidth: 1,
                        borderColor: 'rgba(60,130,252,0.7)',
                        tooltip: { valueSuffix: 'kWh' },
                        yAxis: 0,
                        data: forecastItems.map(function(item) {
                            return { x: +(new Date(item.date)), y: item.usage1.usage };
                        })
                    });
                    if (hasUsage2) {
                        series.push({
                            name: $.t('Usage') + ' 2 (' + $.t('Forecast') + ')',
                            color: 'rgba(3,190,252,0.35)',
                            stack: 'susage',
                            dashStyle: 'Dash',
                            borderWidth: 1,
                            borderColor: 'rgba(3,190,252,0.7)',
                            tooltip: { valueSuffix: 'kWh' },
                            yAxis: 0,
                            data: forecastItems.map(function(item) {
                                return { x: +(new Date(item.date)), y: item.usage2.usage };
                            })
                        });
                    }
                    if (checkDataKey(data, 'return1')) {
                        series.push({
                            name: $.t('Return') + ' 1 (' + $.t('Forecast') + ')',
                            color: 'rgba(30,242,110,0.35)',
                            stack: 'susage',
                            dashStyle: 'Dash',
                            borderWidth: 1,
                            borderColor: 'rgba(30,242,110,0.7)',
                            tooltip: { valueSuffix: 'kWh' },
                            yAxis: 0,
                            data: forecastItems.map(function(item) {
                                return { x: +(new Date(item.date)), y: -item.return1.usage };
                            })
                        });
                    }
                    if (checkDataKey(data, 'return2')) {
                        series.push({
                            name: $.t('Return') + ' 2 (' + $.t('Forecast') + ')',
                            color: 'rgba(3,252,190,0.35)',
                            stack: 'susage',
                            dashStyle: 'Dash',
                            borderWidth: 1,
                            borderColor: 'rgba(3,252,190,0.7)',
                            tooltip: { valueSuffix: 'kWh' },
                            yAxis: 0,
                            data: forecastItems.map(function(item) {
                                return { x: +(new Date(item.date)), y: -item.return2.usage };
                            })
                        });
                    }
                }
			} else {
				series.push({
					name: $.t('Usage'),
					color: 'rgba(3,190,252,0.8)',
					stack: 'susage',
					tooltip: {
						valueSuffix: 'kWh'
					},
					yAxis: 0,
					data: actualItems.map(function (item) {
						return {
							x: +(new Date(item.date)),
							y: item.totalUsage
						}
					})
				});
				if (checkDataKey(data, 'return1')) {
					series.push({
						name: $.t('Return'),
						color: 'rgba(3,252,190,0.8)',
						stack: 'susage',
						tooltip: {
							valueSuffix: 'kWh'
						},
						yAxis: 0,
						data: actualItems.map(function (item) {
							return {
								x: +(new Date(item.date)),
								y: -item.totalReturn
							}
						})
					});
				}
                if (forecastItems.length) {
                    series.push({
                        name: $.t('Usage') + ' (' + $.t('Forecast') + ')',
                        color: 'rgba(3,190,252,0.35)',
                        stack: 'susage',
                        dashStyle: 'Dash',
                        borderWidth: 1,
                        borderColor: 'rgba(3,190,252,0.7)',
                        tooltip: { valueSuffix: 'kWh' },
                        yAxis: 0,
                        data: forecastItems.map(function(item) {
                            return { x: +(new Date(item.date)), y: item.totalUsage };
                        })
                    });
                    if (checkDataKey(data, 'return1')) {
                        series.push({
                            name: $.t('Return') + ' (' + $.t('Forecast') + ')',
                            color: 'rgba(3,252,190,0.35)',
                            stack: 'susage',
                            dashStyle: 'Dash',
                            borderWidth: 1,
                            borderColor: 'rgba(3,252,190,0.7)',
                            tooltip: { valueSuffix: 'kWh' },
                            yAxis: 0,
                            data: forecastItems.map(function(item) {
                                return { x: +(new Date(item.date)), y: -item.totalReturn };
                            })
                        });
                    }
                }
			}

			series.push({
				id: 'CP1RP',
				type: 'spline',
				name: $.t('Costs'),
				zIndex: 3,
				tooltip: {
					valueSuffix: ' ' + $.myglobals.currencysign,
					pointFormat: '<span style="color: {point.color}">●</span> {series.name}: <b>{point.y}</b><br>',
					valueDecimals: 4
				},
				marker: {
					enabled: false
				},
				lineWidth: 2,
				color: 'rgba(190,252,60,0.8)',
				showInLegend: true,
				convertZeroToNull: true,
				showWithoutDatapoints: false,
				yAxis: 1,
				data: actualItems.map(function (item) {
					return {
						x: +(new Date(item.date)),
						y: item.cost
					}
				})
			});

            chartElement.highcharts({
                chart: {
                    type: 'column',
                },
                title: {
                    text: ''
                },
                xAxis: {
                    type: 'datetime',
                    minTickInterval: vm.isMonthView ? 24 * 3600 * 1000 : 28 * 24 * 3600 * 1000
                },
                yAxis: [
					{
						labels: {
							formatter: function () {
								return Highcharts.numberFormat(this.value, 0, '', '');
							}
						},
						title: {
							text: $.t('Energy') + ' (' + vm.unit + ')'
						},
						maxPadding: 0.2,
					},
                    {
						visible: true,
						showEmpty: false,
						opposite: true,
                        title: {
                            text: $.t('Price') + ' (' + $.myglobals.currencysign + ')'
                        }
                    }
				],
                tooltip: {
					headerFormat: '{point.x:%A, %B %d, %Y}<br/>',
					pointFormat: '<span style="color: {point.color}">●</span> {series.name}: <b>{abs3 point.y} {point.series.tooltipOptions.valueSuffix}</b> ( {point.percentage:.0f}% )<br>',
					outside: true,
					crosshairs: true,
					shared: true,
					//useHTML: true
                },
                plotOptions: {
					series: {
						point: {
							events: {
								click: function (event) {
									if (vm.isMonthView) {
										chartPointClickNew(event, false, reloadPage);
									}
								}
							}
						}
					},
                    column: {
                        stacking: 'normal',
                        minPointLength: 4,
                        pointPadding: 0.1,
                        groupPadding: 0,
                    }
                },
                legend: {
                    enabled: true
                },
                series: series
            });
        }
    }
});
