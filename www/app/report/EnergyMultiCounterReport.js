define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceEnergyMultiCounterReportData', function ($q, domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(deviceIdx, year, month) {
            var costs = domoticzApi.sendCommand('getcosts', { idx: deviceIdx });

            var stats = domoticzApi.sendRequest({
                type: 'graph',
                sensor: 'counter',
                range: 'year',
                idx: deviceIdx,
                actyear: year,
                actmonth: month
            });

            return $q.all([costs, stats]).then(function (responses) {
                var costs = responses[0];
                var stats = responses[1];

                if (!stats.result || !stats.result.length) {
                    return null;
                }

                var includeReturn = typeof stats.delivered !== 'undefined';
                var data = getGroupedData(stats.result, costs, includeReturn);
                var source = month
                    ? data.years[year].months.find(function (item) {
                        return (new Date(item.date)).getMonth() + 1 === month;
                    })
                    : data.years[year];

                if (!source) {
                    return null;
                }

                return Object.assign({}, source, {
                    items: month ? source.days : source.months
                });
            });
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

                var dayRecord = {
                    date: item.d,
                    usage1: {
                        usage: parseFloat(item.v),
                        cost: parseFloat(item.v) * costs.CostEnergy / 10000,
                        counter: parseFloat(item.c1) || 0
                    },
                    usage2: {
                        usage: parseFloat(item.v2),
                        cost: parseFloat(item.v2) * costs.CostEnergyT2 / 10000,
                        counter: parseFloat(item.c3) || 0
                    },
                };

                if (includeReturn) {
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
                dayRecord.cost = dayRecord.usage1.cost + dayRecord.usage2.cost -
                    (includeReturn ? (dayRecord.return1.cost + dayRecord.return2.cost) : 0);
                result.years[year].months[month].days[day] = dayRecord
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
            device: '<',
            selectedYear: '<',
            selectedMonth: '<'
        },
        templateUrl: 'app/report/EnergyMultiCounterReport.html',
        controller: DeviceCounterReportController
    });


    function DeviceCounterReportController($element, DeviceEnergyMultiCounterReportData, dataTableDefaultSettings) {
        var vm = this;
        vm.$onInit = init;

        function init() {
            vm.unit = vm.device.getUnit();
            vm.isMonthView = vm.selectedMonth > 0;
            vm.degreeType = $.myglobals.tempsign;

            getData();
        }

        function getData() {
            DeviceEnergyMultiCounterReportData
                .fetch(vm.device.idx, vm.selectedYear, vm.selectedMonth)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    vm.data = data;
                    vm.hasReturn = checkDataKey(data, 'return1');

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
            var table = $element.find('table');
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
                    width: 150,
                    data: 'date',
                    render: function (data) {
                        var date = new Date(data);
                        var link = '<a href="#/Devices/' + vm.device.idx + '/Report/' + vm.selectedYear + '/' + (date.getMonth() + 1) + '"><img src="images/next.png" /></a>';
                        return dateFormat(data, 'mm. mmmm') + ' ' + link;
                    }
                });
            }

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
                    columns.push({ title: $.t(item[2]), data: item[0]+'.counter', render: counterRenderer });
                }

                columns.push({ title: $.t(item[1]), data: item[0]+'.usage', render: counterRenderer });
                columns.push({ title: $.t('Costs'), data: item[0]+'.cost', render: costRenderer });
            });

            columns.push({ title: $.t('Total'), data: 'cost', render: costRenderer });

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
            var hasUsage2 = checkDataKey(data, 'usage2');

            var series = [];

            series.push({
                name: hasUsage2 ? $.t('Usage') + ' 1' : $.t('Usage'),
                color: hasUsage2 ? 'rgba(60,130,252,0.8)' : 'rgba(3,190,252,0.8)',
                stack: 'susage',
                yAxis: 0,
                data: data.items.map(function (item) {
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
                    yAxis: 0,
                    data: data.items.map(function (item) {
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
                    stack: 'sreturn',
                    yAxis: 0,
                    data: data.items.map(function (item) {
                        return {
                            x: +(new Date(item.date)),
                            y: item.return1.usage
                        }
                    })
                });
            }

            if (checkDataKey(data, 'return2')) {
                series.push({
                    name: $.t('Return') + ' 2',
                    color: 'rgba(3,252,190,0.8)',
                    stack: 'sreturn',
                    yAxis: 0,
                    data: data.items.map(function (item) {
                        return {
                            x: +(new Date(item.date)),
                            y: item.return2.usage
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
                    type: 'datetime',
                    minTickInterval: vm.isMonthView ? 24 * 3600 * 1000 : 28 * 24 * 3600 * 1000
                },
                yAxis: {
                    title: {
                        text: $.t('Energy') + ' (' + vm.unit + ')'
                    },
                    maxPadding: 0.2,
                    min: 0
                },
                tooltip: {
					formatter: function () {
						return $.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%Y-%m-%d', this.x) + '<br/>' + $.t(this.series.name) + ': ' + Highcharts.numberFormat(this.y,3) + ' ' + vm.unit + '<br/>Total: ' + this.point.stackTotal + ' ' + vm.unit;
					}
                },
                plotOptions: {
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
