define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceRainReportData', function (domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(deviceIdx, year, month) {
            return domoticzApi.sendCommand('graph', {
                sensor: 'rain',
                range: 'year',
                idx: deviceIdx,
                actyear: year,
                actmonth: month
            }).then(function (response) {
                if (!response.result || !response.result.length) {
                    return null;
                }

                var data = getGroupedData(response.result);
                var source = month
                    ? data.years[year] && data.years[year].months.find(function (item) {
                        return item.date === month;
                    })
                    : data.years[year];

                if (!source) {
                    return null;
                }

                return {
                    total: source.total,
                    highestDay: source.highestDay,
                    highestMonth: source.highestMonth,
                    lowestMonth: source.lowestMonth,
                    items: month ? source.days : source.months
                };
            });
        }

        function getGroupedData(data) {
            var result = {
                years: {}
            };

            data.forEach(function (item) {
                var month = parseInt(item.d.substring(5, 7), 10);
                var year = parseInt(item.d.substring(0, 4), 10);
                var day = parseInt(item.d.substring(8, 10), 10);
                var mm = parseFloat(item.mm);

                if (!result.years[year]) {
                    result.years[year] = {
                        months: {}
                    };
                }

                if (!result.years[year].months[month]) {
                    result.years[year].months[month] = {
                        date: month,
                        days: {}
                    };
                }

                result.years[year].months[month].days[day] = {
                    date: item.d,
                    value: mm
                };
            });

            Object.keys(result.years).forEach(function (year) {
                var yearsData = result.years[year];
                yearsData.months = Object.values(yearsData.months);

                yearsData.months.sort(function (a, b) {
                    return a.date < b.date ? -1 : 1;
                });

                var yearHighestDay = null;

                yearsData.months.forEach(function (month) {
                    month.days = Object.values(month.days);

                    month.days.sort(function (a, b) {
                        return a.date < b.date ? -1 : 1;
                    });

                    month.days = reportHelpers.addTrendData(month.days, 'value');

                    var stats = month.days.reduce(function (acc, item) {
                        acc.total = (acc.total || 0) + item.value;

                        if (!acc.highestDay || item.value > acc.highestDay.value) {
                            acc.highestDay = {
                                value: item.value,
                                date: item.date
                            };
                        }

                        return acc;
                    }, {});

                    month.total = parseFloat((stats.total || 0).toFixed(1));
                    month.highestDay = stats.highestDay;

                    if (!yearHighestDay || stats.highestDay.value > yearHighestDay.value) {
                        yearHighestDay = stats.highestDay;
                    }
                });

                yearsData.months = reportHelpers.addTrendData(yearsData.months, 'total');

                var yearStats = yearsData.months.reduce(function (acc, item) {
                    acc.total = (acc.total || 0) + item.total;

                    if (!acc.highestMonth || item.total > acc.highestMonth.total) {
                        acc.highestMonth = item;
                    }

                    if (!acc.lowestMonth || item.total < acc.lowestMonth.total) {
                        acc.lowestMonth = item;
                    }

                    return acc;
                }, {});

                yearsData.total = parseFloat((yearStats.total || 0).toFixed(1));
                yearsData.highestDay = yearHighestDay;
                yearsData.highestMonth = yearStats.highestMonth
                    ? { date: yearStats.highestMonth.date, total: yearStats.highestMonth.total }
                    : null;
                yearsData.lowestMonth = yearStats.lowestMonth
                    ? { date: yearStats.lowestMonth.date, total: yearStats.lowestMonth.total }
                    : null;
            });

            return result;
        }
    });

    app.component('deviceRainReport', {
        bindings: {
            device: '<',
            selectedYear: '<',
            selectedMonth: '<'
        },
        templateUrl: 'app/report/RainReport.html',
        controller: DeviceRainReportController
    });

    function DeviceRainReportController($element, dataTableDefaultSettings, DeviceRainReportData) {
        var vm = this;
        var monthNames = ["January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"];

        vm.$onInit = init;

        vm.exportExcel     = function () { reportHelpers.exportTableToExcel($element, vm.device.Name + '_report'); };
        vm.exportCSV       = function () { reportHelpers.exportTableToCSV($element, vm.device.Name + '_report'); };
        vm.exportClipboard = function () { reportHelpers.exportTableToClipboard($element); };

        function init() {
            vm.isMonthView = vm.selectedMonth > 0;
            getData();
        }

        function getData() {
            DeviceRainReportData
                .fetch(vm.device.idx, vm.selectedYear, vm.selectedMonth)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    vm.totalRain = data.total.toFixed(1);

                    if (data.highestDay) {
                        vm.highestDay = {
                            date: dateFormat(data.highestDay.date, 'UTC:d') + ' ' + $.t(dateFormat(data.highestDay.date, 'UTC:mmmm')),
                            value: data.highestDay.value.toFixed(1)
                        };
                    }

                    if (!vm.isMonthView && data.highestMonth) {
                        vm.highestMonth = {
                            name: $.t(monthNames[data.highestMonth.date - 1]),
                            value: data.highestMonth.total.toFixed(1)
                        };
                        vm.lowestMonth = {
                            name: $.t(monthNames[data.lowestMonth.date - 1]),
                            value: data.lowestMonth.total.toFixed(1)
                        };
                    }

                    showTable(data);
                    showChart(data);
                });
        }

        function showTable(data) {
            var table = $element.find('#reporttable');
            var columns = [];

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

                columns.push({
                    title: $.t('Rain (mm)'),
                    data: 'value',
                    render: function (data) {
                        return data.toFixed(1);
                    }
                });
            } else {
                columns.push({
                    title: $.t('Month'),
                    data: 'date',
                    render: function (data) {
                        var link = '<a href="#/Devices/' + vm.device.idx + '/Report/' + vm.selectedYear + '/' + data + '"><i class="fa-solid fa-chevron-right dz-chrome-icon"></i></a>';
                        return data.toString().padStart(2, '0') + '. ' + $.t(monthNames[data - 1]) + ' ' + link;
                    }
                });

                columns.push({
                    title: $.t('Rain (mm)'),
                    data: 'total',
                    render: function (data) {
                        return data.toFixed(1);
                    }
                });
            }

            columns.push({
                title: '<>',
                orderable: false,
                data: 'trend',
                render: function (data) {
                    return reportHelpers.trendIconHtml(data, false);
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

            var totalRain = data.items.reduce(function (s, r) {
                return s + (vm.isMonthView ? (r.value || 0) : (r.total || 0));
            }, 0);
            var cells = [];
            cells.push('<td style="font-weight:bold">' + $.t('Total') + '</td>');
            if (vm.isMonthView) { cells.push('<td></td>'); }
            cells.push('<td style="font-weight:bold">' + totalRain.toFixed(1) + '</td>');
            cells.push('<td></td>');
            var tfoot = $('<tfoot><tr style="font-weight:bold; background:var(--dz-accent-color,#337ab7); color:var(--dz-body-text,#fff);">' + cells.join('') + '</tr></tfoot>');
            table.append(tfoot);
        }

        function showChart(data) {
            var chartElement = $element.find('#raingraph');
            var highestValue = vm.isMonthView
                ? (data.highestDay ? data.highestDay.value : 0)
                : (data.highestMonth ? data.highestMonth.total : 0);

            var chartData = data.items.map(function (item) {
                var value = vm.isMonthView ? item.value : item.total;
                var color = 'rgba(3,190,252,0.8)';

                if (value === highestValue) {
                    color = '#FF0000';
                }

                return {
                    x: vm.isMonthView
                        ? +(new Date(item.date))
                        : Date.UTC(vm.selectedYear, item.date - 1, 1),
                    y: parseFloat(value.toFixed(1)),
                    color: color
                };
            });

            chartElement.highcharts({
                title: {
                    text: $.t('Rainfall')
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: {
                    min: 0,
                    maxPadding: 0.2,
                    title: {
                        text: $.t('Rainfall') + ' (mm)'
                    }
                },
                tooltip: {
                    headerFormat: '<span style="font-size: 10px">{point.key}</span><br/>',
                    pointFormat: '{series.name}: <b>{point.y} mm</b>',
                    xDateFormat: vm.isMonthView ? '%A, %e %B' : '%B %Y'
                },
                plotOptions: {
                    column: {
                        minPointLength: 4,
                        pointPadding: 0.1,
                        groupPadding: 0,
                        dataLabels: {
                            enabled: !vm.isMonthView,
                            color: 'white',
                            format: '{y} mm'
                        }
                    }
                },
                legend: {
                    enabled: false
                },
                series: [{
                    type: 'column',
                    name: $.t('Rainfall'),
                    color: 'rgba(3,190,252,0.8)',
                    data: chartData
                }]
            });
        }
    }
});
