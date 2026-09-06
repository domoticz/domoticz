define(['app', 'report/helpers'], function (app, reportHelpers) {
    app.factory('DeviceWindReportData', function (domoticzApi) {
        return {
            fetch: fetch
        };

        function fetch(deviceIdx, year, month) {
            return domoticzApi.sendCommand('graph', {
                sensor: 'wind',
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
                    highestGustDay: source.highestGustDay,
                    windiestDay: source.windiestDay,
                    windiestMonth: source.windiestMonth,
                    calmestMonth: source.calmestMonth,
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
                var speed = parseFloat(item.sp);
                var gust = parseFloat(item.gu);
                var direction = item.di;

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
                    speed: speed,
                    gust: gust,
                    direction: direction
                };
            });

            Object.keys(result.years).forEach(function (year) {
                var yearsData = result.years[year];
                yearsData.months = Object.values(yearsData.months);

                yearsData.months.sort(function (a, b) {
                    return a.date < b.date ? -1 : 1;
                });

                var yearHighestGustDay = null;
                var yearWindiestDay = null;

                yearsData.months.forEach(function (month) {
                    month.days = Object.values(month.days);

                    month.days.sort(function (a, b) {
                        return a.date < b.date ? -1 : 1;
                    });

                    month.days = reportHelpers.addTrendData(month.days, 'speed');

                    var stats = month.days.reduce(function (acc, item) {
                        acc.totalSpeed = (acc.totalSpeed || 0) + item.speed;
                        acc.count = (acc.count || 0) + 1;

                        if (!acc.highestGustDay || item.gust > acc.highestGustDay.gust) {
                            acc.highestGustDay = {
                                gust: item.gust,
                                date: item.date
                            };
                        }

                        if (!acc.windiestDay || item.speed > acc.windiestDay.speed) {
                            acc.windiestDay = {
                                speed: item.speed,
                                date: item.date
                            };
                        }

                        return acc;
                    }, {});

                    month.avgSpeed = stats.count > 0
                        ? parseFloat((stats.totalSpeed / stats.count).toFixed(1))
                        : 0;
                    month.highestGustDay = stats.highestGustDay;
                    month.windiestDay = stats.windiestDay;

                    if (!yearHighestGustDay || (stats.highestGustDay && stats.highestGustDay.gust > yearHighestGustDay.gust)) {
                        yearHighestGustDay = stats.highestGustDay;
                    }

                    if (!yearWindiestDay || (stats.windiestDay && stats.windiestDay.speed > yearWindiestDay.speed)) {
                        yearWindiestDay = stats.windiestDay;
                    }
                });

                yearsData.months = reportHelpers.addTrendData(yearsData.months, 'avgSpeed');

                var yearStats = yearsData.months.reduce(function (acc, item) {
                    if (!acc.windiestMonth || item.avgSpeed > acc.windiestMonth.avgSpeed) {
                        acc.windiestMonth = item;
                    }

                    if (!acc.calmestMonth || item.avgSpeed < acc.calmestMonth.avgSpeed) {
                        acc.calmestMonth = item;
                    }

                    return acc;
                }, {});

                yearsData.highestGustDay = yearHighestGustDay;
                yearsData.windiestDay = yearWindiestDay;
                yearsData.windiestMonth = yearStats.windiestMonth
                    ? { date: yearStats.windiestMonth.date, avgSpeed: yearStats.windiestMonth.avgSpeed }
                    : null;
                yearsData.calmestMonth = yearStats.calmestMonth
                    ? { date: yearStats.calmestMonth.date, avgSpeed: yearStats.calmestMonth.avgSpeed }
                    : null;
            });

            return result;
        }
    });

    app.component('deviceWindReport', {
        bindings: {
            device: '<',
            selectedYear: '<',
            selectedMonth: '<'
        },
        templateUrl: 'app/report/WindReport.html',
        controller: DeviceWindReportController
    });

    function DeviceWindReportController($element, dataTableDefaultSettings, DeviceWindReportData) {
        var vm = this;
        var monthNames = ["January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"];

        vm.$onInit = init;

        vm.exportExcel     = function () { reportHelpers.exportTableToExcel($element, vm.device.Name + '_report'); };
        vm.exportCSV       = function () { reportHelpers.exportTableToCSV($element, vm.device.Name + '_report'); };
        vm.exportClipboard = function () { reportHelpers.exportTableToClipboard($element); };

        function init() {
            vm.isMonthView = vm.selectedMonth > 0;
            vm.windUnit = vm.device.getUnit();
            getData();
        }

        function getData() {
            DeviceWindReportData
                .fetch(vm.device.idx, vm.selectedYear, vm.selectedMonth)
                .then(function (data) {
                    if (!data) {
                        vm.noDataAvailable = true;
                        return;
                    }

                    if (data.highestGustDay) {
                        vm.highestGustDay = {
                            date: dateFormat(data.highestGustDay.date, 'UTC:d') + ' ' + $.t(dateFormat(data.highestGustDay.date, 'UTC:mmmm')),
                            value: data.highestGustDay.gust.toFixed(1)
                        };
                    }

                    if (data.windiestDay) {
                        vm.windiestDay = {
                            date: dateFormat(data.windiestDay.date, 'UTC:d') + ' ' + $.t(dateFormat(data.windiestDay.date, 'UTC:mmmm')),
                            value: data.windiestDay.speed.toFixed(1)
                        };
                    }

                    if (!vm.isMonthView && data.windiestMonth && data.calmestMonth) {
                        vm.windiestMonth = {
                            name: $.t(monthNames[data.windiestMonth.date - 1]),
                            value: data.windiestMonth.avgSpeed.toFixed(1)
                        };
                        vm.calmestMonth = {
                            name: $.t(monthNames[data.calmestMonth.date - 1]),
                            value: data.calmestMonth.avgSpeed.toFixed(1)
                        };
                    }

                    showTable(data);
                    showSpeedChart(data);
                    if (vm.isMonthView) {
                        showGustChart(data);
                    }
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
                    title: $.t('Speed') + ' (' + vm.windUnit + ')',
                    data: 'speed',
                    render: function (data) {
                        return data.toFixed(1);
                    }
                });

                columns.push({
                    title: $.t('Gust') + ' (' + vm.windUnit + ')',
                    data: 'gust',
                    render: function (data) {
                        return data.toFixed(1);
                    }
                });

                columns.push({
                    title: $.t('Direction'),
                    data: 'direction'
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
                    title: $.t('Avg Speed') + ' (' + vm.windUnit + ')',
                    data: 'avgSpeed',
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
                dom: '<"H"r>t<"F">',
                columns: columns,
                pageLength: 50,
                order: [[0, 'asc']]
            }));

            table.dataTable().api().rows
                .add(data.items)
                .draw();
        }

        function showSpeedChart(data) {
            var chartElement = $element.find('#windgraph');
            var highestValue = vm.isMonthView
                ? (data.windiestDay ? data.windiestDay.speed : 0)
                : (data.windiestMonth ? data.windiestMonth.avgSpeed : 0);

            var chartData = data.items.map(function (item) {
                var value = vm.isMonthView ? item.speed : item.avgSpeed;
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
                    text: vm.isMonthView ? $.t('Wind Speed') : $.t('Average Wind Speed')
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: {
                    min: 0,
                    maxPadding: 0.2,
                    title: {
                        text: $.t('Wind Speed') + ' (' + vm.windUnit + ')'
                    }
                },
                tooltip: {
                    headerFormat: '<span style="font-size: 10px">{point.key}</span><br/>',
                    pointFormat: '{series.name}: <b>{point.y} ' + vm.windUnit + '</b>',
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
                            format: '{y} ' + vm.windUnit
                        }
                    }
                },
                legend: {
                    enabled: false
                },
                series: [{
                    type: 'column',
                    name: vm.isMonthView ? $.t('Wind Speed') : $.t('Avg Speed'),
                    color: 'rgba(3,190,252,0.8)',
                    data: chartData
                }]
            });
        }

        function showGustChart(data) {
            var chartElement = $element.find('#gustgraph');

            var chartData = data.items.map(function (item) {
                return {
                    x: +(new Date(item.date)),
                    low: parseFloat(item.speed.toFixed(1)),
                    high: parseFloat(item.gust.toFixed(1))
                };
            });

            chartElement.highcharts({
                title: {
                    text: $.t('Speed vs Gust')
                },
                xAxis: {
                    type: 'datetime'
                },
                yAxis: {
                    min: 0,
                    title: {
                        text: $.t('Wind Speed') + ' (' + vm.windUnit + ')'
                    }
                },
                tooltip: {
                    headerFormat: '<span style="font-size: 10px">{point.key}</span><br/>',
                    pointFormat: $.t('Speed') + ': <b>{point.low} ' + vm.windUnit + '</b><br/>' +
                        $.t('Gust') + ': <b>{point.high} ' + vm.windUnit + '</b>',
                    xDateFormat: '%A, %e %B'
                },
                legend: {
                    enabled: false
                },
                series: [{
                    type: 'areasplinerange',
                    name: $.t('Speed / Gust'),
                    color: 'rgba(3,190,252,0.8)',
                    data: chartData
                }]
            });
        }
    }
});
