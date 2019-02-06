define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceCounterReportData', function ($q, domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(device, year, month) {
            var costs = domoticzApi.sendCommand('getcosts', { idx: device.idx });

            var stats = domoticzApi.sendRequest({
                type: 'graph',
                sensor: 'counter',
                range: 'year',
                idx: device.idx,
                actyear: year,
                actmonth: month
            });

            return $q.all([costs, stats]).then(function (responses) {
                var cost = getCost(device, responses[0]);
                var stats = responses[1];

                if (!stats.result || !stats.result.length) {
                    return null;
                }

                var data = getGroupedData(stats.result, cost);
                var source = month
                    ? data.years[year].months.find(function (item) {
                        return (new Date(item.date)).getMonth() + 1 === month;
                    })
                    : data.years[year];

                if (!source) {
                    return null;
                }

                return {
                    cost: source.cost,
                    usage: source.usage,
                    counter: month ? source.counter : parseFloat(stats.counter),
                    items: month ? source.days : source.months
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

                result.years[year].months[month].days[day] = {
                    date: item.d,
                    usage: parseFloat(item.v),
                    counter: parseFloat(item.c),
                    cost: parseFloat(item.v) * cost
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
            device: '<',
            selectedYear: '<',
            selectedMonth: '<',
            isOnlyUsage: '<',
        },
        templateUrl: 'app/report/CounterReport.html',
        controller: DeviceCounterReportController
    });


    function DeviceCounterReportController($element, DeviceCounterReportData, dataTableDefaultSettings) {
        var vm = this;
        vm.$onInit = init;

        function init() {
            vm.unit = vm.device.getUnit();
            vm.isMonthView = vm.selectedMonth > 0;

            getData();
        }

        function getData() {
            DeviceCounterReportData
                .fetch(vm.device, vm.selectedYear, vm.selectedMonth)
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
            var columns = [];

            var counterRenderer = function (data) {
                return data.toFixed(3);
            };

            var costRenderer = function (data) {
                return data.toFixed(2);
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
                    title: $.t('Month'),
                    data: 'date',
                    render: function (data) {
                        var date = new Date(data);
                        var link = '<a href="#/Devices/' + vm.device.idx + '/Report/' + vm.selectedYear + '/' + (date.getMonth() + 1) + '"><img src="images/next.png" /></a>';
                        return dateFormat(data, 'mm. mmmm') + ' ' + link;
                    }
                });
            }

            if (vm.isMonthView && !vm.isOnlyUsage) {
                columns.push({ title: $.t('Counter'), data: 'counter', render: counterRenderer });
            }

            columns.push({ title: $.t('Usage'), data: 'usage', render: counterRenderer });

			if (!['Counter Incremental'].includes(vm.device.SubType) && (vm.device.SwitchTypeVal != 3))
				columns.push({ title: $.t('Costs'), data: 'cost', render: costRenderer });

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
                order: [[0, 'asc']]
            }));

            table.dataTable().api().rows
                .add(data.items)
                .draw();
        }

        function showUsageChart(data) {
            var chartElement = $element.find('#usagegraph');
            var series = [];
            var valueQuantity = "Count";
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
                        y: parseFloat(item.usage.toFixed(3))
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
                    type: 'datetime'
                },
                yAxis: {
                    title: {
                        text: $.t(yAxisName) + ' (' + vm.unit + ')'
                    },
                    maxPadding: 0.2,
                    min: 0
                },
                tooltip: {
                    valueSuffix: ' ' + vm.unit,
                    valueDecimals: 3
                },
                plotOptions: {
                    column: {
                        minPointLength: 4,
                        pointPadding: 0.1,
                        groupPadding: 0,
                        dataLabels: {
                            enabled: true,
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
