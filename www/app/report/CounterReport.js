define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceCounterReportData', function ($q, domoticzApi) {
        return {
            fetch: fetch
        };

        function addOneYear(isoDate, direction) {
            if (!isoDate || !/^\d{4}-\d{2}-\d{2}$/.test(isoDate)) { return null; }
            var d = new Date(isoDate + 'T00:00:00');
            if (isNaN(d.getTime())) { return null; }
            var origMonth = d.getMonth();
            var origDay   = d.getDate();
            d.setFullYear(d.getFullYear() + (direction || 1));
            if (origMonth === 1 && origDay === 29 && d.getMonth() === 2) { d.setDate(28); }
            return d.getFullYear() + '-'
                 + String(d.getMonth() + 1).padStart(2, '0') + '-'
                 + String(d.getDate()).padStart(2, '0');
        }

        function formatContractMonthLabel(start, end) {
            var monthNames = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
            var endDay = new Date(end.getTime() - 1); // last ms of previous day
            return monthNames[start.getMonth()] + ' ' + start.getDate()
                 + ' \u2013 ' + monthNames[endDay.getMonth()] + ' ' + endDay.getDate()
                 + ' ' + endDay.getFullYear();
        }

        function getContractMonthData(rawData, cost, startDateISO) {
            var base = new Date(startDateISO + 'T00:00:00');
            var periods = [];
            for (var i = 0; i < 12; i++) {
                var pStart = new Date(base.getFullYear(), base.getMonth() + i, base.getDate());
                var pEnd   = new Date(base.getFullYear(), base.getMonth() + i + 1, base.getDate());
                periods.push({
                    start:       pStart,
                    end:         pEnd,
                    label:       formatContractMonthLabel(pStart, pEnd),
                    date:        +pStart,
                    periodIndex: i + 1,
                    days:        [],
                    usage:       0,
                    cost:        0,
                    counter:     0
                });
            }

            rawData.forEach(function(item) {
                var d = new Date(item.d.substring(0, 10) + 'T00:00:00');
                for (var i = 0; i < periods.length; i++) {
                    if (d >= periods[i].start && d < periods[i].end) {
                        var dayUsage = parseFloat(item.v) || 0;
                        var cprice   = parseFloat(item.p);
                        var dayCost  = (cprice !== 0 && !isNaN(cprice)) ? cprice : dayUsage * cost;
                        periods[i].days.push({ date: item.d, usage: dayUsage, counter: parseFloat(item.c) || 0, cost: dayCost });
                        periods[i].usage   += dayUsage;
                        periods[i].cost    += dayCost;
                        periods[i].counter  = Math.max(periods[i].counter, parseFloat(item.c) || 0);
                        break;
                    }
                }
            });

            periods = reportHelpers.addTrendData(periods, 'usage');

            return {
                months:  periods,
                usage:   periods.reduce(function(s, p) { return s + p.usage; }, 0),
                cost:    periods.reduce(function(s, p) { return s + p.cost; }, 0),
                counter: Math.max.apply(null, periods.map(function(p) { return p.counter; }))
            };
        }

        function fetch(device, year, month, customStartDate) {
            if (customStartDate && !/^\d{4}-\d{2}-\d{2}$/.test(customStartDate)) {
                return $q.resolve(null);
            }
            var costs = domoticzApi.sendCommand('getcosts', { idx: device.idx });

            var graphParams = customStartDate
                ? { sensor: 'counter', range: 'year', idx: device.idx,
                    actstart: customStartDate, actend: addOneYear(customStartDate) }
                : { sensor: 'counter', range: 'year', idx: device.idx,
                    actyear: year, actmonth: month };

            var stats = domoticzApi.sendCommand('graph', graphParams);

            return $q.all([costs, stats]).then(function (responses) {
                var cost = getCost(device, responses[0]);

                var stats = responses[1];

                if (!stats.result || !stats.result.length) {
                    return null;
                }

                var source;
                if (customStartDate) {
                    var contractData = getContractMonthData(stats.result, cost, customStartDate);
                    source = contractData;
                } else {
                    var data = getGroupedData(stats.result, cost);
                    source = month
                        ? data.years[year].months.find(function (item) {
                            return (new Date(item.date)).getMonth() + 1 === month;
                          })
                        : data.years[year];
                }

                if (!source) {
                    return null;
                }

                return {
                    cost:            source.cost,
                    usage:           source.usage,
                    decimals:        (device.SwitchTypeVal === 3) ? device.Divider.numDecimalsDiv1() : 3,
                    counter:         month ? source.counter : parseFloat(stats.counter),
                    items:           customStartDate ? source.months : (month ? source.days : source.months),
                    customStartDate: customStartDate
                };
            });
        }

        function getCost(device, costs) {
            var switchTypeVal = device.SwitchTypeVal;

            if (switchTypeVal === 0 || switchTypeVal === 4) {
                return costs.CostEnergy / 10000;
            } else if (switchTypeVal === 1) {
                return costs.CostGas / 10000;
            } else if (switchTypeVal === 2) {
                return costs.CostWater / 10000;
            }
        }

        function getGroupedData(data, cost) {
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

				let cprice = parseFloat(item.p);

                result.years[year].months[month].days[day] = {
                    date: item.d,
                    usage: parseFloat(item.v),
                    counter: parseFloat(item.c),
                    cost: (cprice) != 0 ? cprice : parseFloat(item.v) * cost
                }
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
            var keys = ['usage', 'cost'];

            return values.reduce(function (acc, item) {
                keys.forEach(function (key) {
                    acc[key] = item[key] !== null
                        ? (acc[key] || 0) + item[key]
                        : null
                });

                acc.counter = Math.max(acc.counter || 0, item.counter);
                return acc;
            }, {});
        }
    });

    app.component('deviceCounterReport', {
        bindings: {
            device:          '<',
            selectedYear:    '<',
            selectedMonth:   '<',
            isOnlyUsage:     '<',
            customStartDate: '<'
        },
        templateUrl: 'app/report/CounterReport.html',
        controller: DeviceCounterReportController
    });


    function DeviceCounterReportController($element, $scope, DeviceCounterReportData, dataTableDefaultSettings) {
        var vm = this;
        vm.$onInit = init;

        vm.exportExcel     = function () { exportTableToExcel(vm.device.Name + '_report.xls'); };
        vm.exportCSV       = function () { exportTableToCSV(vm.device.Name + '_report.csv'); };
        vm.exportClipboard = function () { exportTableToClipboard(); };

        function init() {
            vm.unit = vm.device.getUnit();
            vm.decimals = (vm.device.SwitchTypeVal == 3) ? vm.device.Divider.numDecimalsDiv1() : 3;
            vm.isMonthView = vm.selectedMonth > 0;

			$.devIdx = vm.device.idx;

            var deregisterWatch = $scope.$watch(function() { return vm.customStartDate; }, function(newVal, oldVal) {
                if (newVal !== oldVal && /^\d{4}-\d{2}-\d{2}$/.test(newVal || '')) {
                    getData();
                }
            });
            $scope.$on('$destroy', deregisterWatch);

            getData();
        }


        function getData() {
            DeviceCounterReportData
                .fetch(vm.device, vm.selectedYear, vm.selectedMonth, vm.customStartDate)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    vm.data = data;
                    showTable(data);
                    showUsageChart(data)
                });
        }

        function showTable(data) {
            var table = $element.find('#reporttable');
            // Destroy existing DataTable instance if present
            if ($.fn.dataTable.isDataTable(table)) {
                table.dataTable().api().destroy();
                table.empty();
            }
            var columns = [];

            var counterRendererDecimals = function (data) {
                return data.toFixed(3);
            };
            var counterRenderer = function (data) {
                return data.toFixed(vm.device.Divider.numDecimalsDiv1());
            };

            var costRenderer = function (data) {
                return data.toFixed(2) + ' ' + $.myglobals.currencysign;
            };

            if (vm.isMonthView) {
                columns.push({
                    title: $.t('Day'),
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, 'd'));
                    }
                });

                columns.push({
                    title: '',
                    data: 'date',
                    render: function (data) {
                        return $.t(dateFormat(data, 'dddd'));
                    }
                });
            } else {
                columns.push({
                    title: $.t(vm.data && vm.data.customStartDate ? 'Period' : 'Month'),
                    data: 'date',
                    render: function (data, type, row) {
                        if (type === 'sort' || type === 'type') { return data; }  // sort by raw timestamp
                        if (vm.data && vm.data.customStartDate) {
                            var link = '<a href="#/Devices/' + vm.device.idx + '/Report/'
                                     + 'custom-' + vm.data.customStartDate + '/' + (row.periodIndex || '')
                                     + '"><img src="images/next.png" /></a>';
                            return (row.label || '') + ' ' + link;
                        }
                        var date = new Date(data);
                        var link = '<a href="#/Devices/' + vm.device.idx + '/Report/' + vm.selectedYear + '/' + (date.getMonth() + 1) + '"><img src="images/next.png" /></a>';
                        return dateFormat(data, 'mm. mmmm') + ' ' + link;
                    }
                });
            }

            if (vm.isMonthView && !vm.isOnlyUsage) {
                columns.push({ title: $.t('Counter'), data: 'counter', render: (vm.device.SwitchTypeVal === 3) ? counterRenderer : counterRendererDecimals });
            }

            columns.push({ title: (vm.device.SwitchTypeVal === 4) ? $.t('Generated') : $.t('Usage'), data: 'usage', render: (vm.device.SwitchTypeVal === 3) ? counterRenderer : counterRendererDecimals });

            if (vm.device.SwitchTypeVal != 3)
                columns.push({ title: (vm.device.SwitchTypeVal === 4) ? $.t('Earnings') : $.t('Costs'), data: 'cost', render: costRenderer });

            columns.push({
                title: '<>',
                orderable: false,
                data: 'trend',
                render: function (data) {
                    var ret='<img src="images/';
                    if (vm.device.SwitchTypeVal === 4) ret+="g";
                    ret+=data + '.png">';
                    return ret;
                }
            });

            table.dataTable(Object.assign({}, dataTableDefaultSettings, {
                sDom: '<"H"rC>t<"F">',
                columns: columns,
                pageLength: 50,
                order: [[0, 'asc']]
            }));

            table.dataTable().api().rows
                .add(data.items)
                .draw();

            // Grand-total footer row
            var items = data.items;
            var totalUsage = items.reduce(function (s, r) { return s + (r.usage || 0); }, 0);
            var totalCost  = items.reduce(function (s, r) { return s + (r.cost  || 0); }, 0);
            var maxCounter = items.reduce(function (m, r) { return Math.max(m, r.counter || 0); }, 0);

            var cells = [];
            if (vm.isMonthView) {
                cells.push('<td style="font-weight:bold">' + $.t('Total') + '</td>');
                cells.push('<td></td>');
                if (!vm.isOnlyUsage) {
                    cells.push('<td style="font-weight:bold">' +
                        ((vm.device.SwitchTypeVal === 3) ? counterRenderer(maxCounter) : counterRendererDecimals(maxCounter)) +
                        '</td>');
                }
            } else {
                cells.push('<td style="font-weight:bold">' + $.t('Total') + '</td>');
            }

            cells.push('<td style="font-weight:bold">' +
                ((vm.device.SwitchTypeVal === 3) ? counterRenderer(totalUsage) : counterRendererDecimals(totalUsage)) +
                '</td>');

            if (vm.device.SwitchTypeVal !== 3) {
                cells.push('<td style="font-weight:bold">' + costRenderer(totalCost) + '</td>');
            }

            cells.push('<td></td>');

            var tfoot = $('<tfoot><tr style="font-weight:bold; background:var(--dz-accent-color,#337ab7); color:var(--dz-body-text,#fff);">' + cells.join('') + '</tr></tfoot>');
            table.append(tfoot);
        }

		function reloadPage() {
			window.location.reload();
		}

        function exportTableToCSV(filename) {
            var rows = [];
            var headers = [];
            $element.find('#reporttable thead th').each(function () { headers.push($(this).text()); });
            rows.push(headers.join(','));
            $element.find('#reporttable tbody tr').each(function () {
                var cols = [];
                $(this).find('td').each(function () { cols.push('"' + $(this).text().replace(/"/g, '""') + '"'); });
                rows.push(cols.join(','));
            });
            $element.find('#reporttable tfoot tr').each(function () {
                var cols = [];
                $(this).find('td').each(function () { cols.push('"' + $(this).text().replace(/"/g, '""') + '"'); });
                rows.push(cols.join(','));
            });
            var blob = new Blob([rows.join('\n')], { type: 'text/csv' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a'); a.href = url; a.download = filename; a.click();
            URL.revokeObjectURL(url);
        }

        function exportTableToExcel(filename) {
            function escXml(s) {
                return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
                        .replace(/"/g, '&quot;').replace(/'/g, '&apos;');
            }
            function xmlCell(text) {
                var t = text.trim();
                var isNum = /^-?\d+(\.\d+)?$/.test(t);
                return '<Cell><Data ss:Type="' + (isNum ? 'Number' : 'String') + '">' + escXml(t) + '</Data></Cell>';
            }
            var xml = '<?xml version="1.0" encoding="UTF-8"?>\n'
                + '<?mso-application progid="Excel.Sheet"?>\n'
                + '<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet"'
                + ' xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">\n'
                + '<Worksheet ss:Name="Report"><Table>\n';
            xml += '<Row>';
            $element.find('#reporttable thead th').each(function () { xml += xmlCell($(this).text()); });
            xml += '</Row>\n';
            $element.find('#reporttable tbody tr, #reporttable tfoot tr').each(function () {
                xml += '<Row>';
                $(this).find('td').each(function () { xml += xmlCell($(this).text()); });
                xml += '</Row>\n';
            });
            xml += '</Table></Worksheet></Workbook>';
            var blob = new Blob([xml], { type: 'application/vnd.ms-excel' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a'); a.href = url; a.download = filename; a.click();
            URL.revokeObjectURL(url);
        }

        function exportTableToClipboard() {
            var rows = [];
            var headers = [];
            $element.find('#reporttable thead th').each(function () { headers.push($(this).text()); });
            rows.push(headers.join('\t'));
            $element.find('#reporttable tbody tr').each(function () {
                var cols = [];
                $(this).find('td').each(function () { cols.push($(this).text()); });
                rows.push(cols.join('\t'));
            });
            $element.find('#reporttable tfoot tr').each(function () {
                var cols = [];
                $(this).find('td').each(function () { cols.push($(this).text()); });
                rows.push(cols.join('\t'));
            });
            navigator.clipboard.writeText(rows.join('\n'));
        }

        function showUsageChart(data) {
            var chartElement = $element.find('#usagegraph');
            var series = [];
            var valueQuantity = "Custom";
            if (typeof vm.device.ValueQuantity != 'undefined') {
                    valueQuantity = vm.device.ValueQuantity;
            }

            var chartName = vm.device.SwitchTypeVal === 4 ? 'Generated' : 'Usage';
            var yAxisName = ['Energy', 'Gas', 'Water', valueQuantity, 'Energy'][vm.device.SwitchTypeVal];

            series.push({
                name: $.t(chartName),
                color: 'rgba(3,190,252,0.8)',
                stack: 'susage',
                yAxis: 0,
                data: data.items.map(function (item) {
                    return {
                        x: +(new Date(item.date)),
                        y: parseFloat(item.usage.toFixed(vm.decimals))
                    }
                })
            });
			if (vm.device.SwitchTypeVal != 3) {
				series.push({
					id: 'CRP',
					type: 'spline',
					name: $.t('Costs'),
					zIndex: 3,
					tooltip: {
						valueSuffix: ' ' + $.myglobals.currencysign
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
					data: data.items.map(function (item) {
						return {
							x: +(new Date(item.date)),
							y: parseFloat(item.cost.toFixed(vm.decimals))
						}
					})
				});
			}

            chartElement.highcharts({
                chart: {
                    type: 'column',
                },
                title: {
                    text: ''
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: [{
						labels: {
							formatter: function () {
								return Highcharts.numberFormat(this.value, 0, '', '');
							}
						},
						title: {
							text: $.t(yAxisName) + ' (' + vm.unit + ')'
						},
						maxPadding: 0.2,
						//min: 0
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
                    valueSuffix: ' ' + vm.unit,
                    valueDecimals: vm.decimals,
					outside: true,
					crosshairs: true,
					shared: true
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
                        minPointLength: 4,
                        pointPadding: 0.1,
                        groupPadding: 0,
                        dataLabels: {
                            enabled: !vm.isMonthView,
                            color: 'white'
                        }
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
