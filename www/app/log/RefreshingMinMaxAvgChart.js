define(['RefreshingChart'], function (RefreshingChart) {

    function RefreshingMinMaxAvgChart(baseParams, angularParams, domoticzParams, chartParams) {
        RefreshingChart.call(this, baseParams, angularParams, domoticzParams, chartParams);
    }

    RefreshingMinMaxAvgChart.prototype = Object.create(RefreshingChart.prototype);
    RefreshingMinMaxAvgChart.prototype.constructor = RefreshingMinMaxAvgChart;

    RefreshingMinMaxAvgChart.prototype.dataPointIsShort = function () {
        return false;
    }
    RefreshingMinMaxAvgChart.prototype.getDataEdgeLeft = function (data) {
        return GetLocalDateFromString(data.result[0].d);
    }
    RefreshingMinMaxAvgChart.prototype.getDataEdgeRight = function (data) {
        return GetLocalDateFromString(data.result[data.result.length - 1].d);
    }
    RefreshingMinMaxAvgChart.prototype.loadData = function (data) {
        const valueKey = this.getPointValueKey();
        const series = {min: [], max: [], avg: []};
        data.result.forEach(function (item) {
            series.min.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_min'])]);
            series.max.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_max'])]);
            if (item[valueKey + '_avg'] !== undefined) {
                series.avg.push([GetLocalDateFromString(item.d), parseFloat(item[valueKey + '_avg'])]);
            }
        });
        if (this.chart.series.length === 0) {
            this.chart.addSeries(
                {
                    name: 'min',
                    color: Highcharts.getOptions().colors[3],
                    data: series.min
                }
            );
            this.chart.addSeries(
                {
                    name: 'max',
                    color: Highcharts.getOptions().colors[2],
                    data: series.max
                }
            );
            if (series.avg.length !== 0) {
                this.chart.addSeries(
                    {
                        name: 'avg',
                        color: Highcharts.getOptions().colors[0],
                        data: series.avg
                    }
                );
            }
        } else {
            this.chart.series[0].setData(series.min, false);
            this.chart.series[1].setData(series.max, false);
            if (series.avg.length !== 0) {
                if (this.chart.series.length !== 3) {
                    this.chart.addSeries(
                        {
                            name: 'avg',
                            color: Highcharts.getOptions().colors[0],
                            data: series.avg
                        }
                    );
                } else {
                    this.chart.series[2].setData(series.avg, false);
                }
            } else {
                if (this.chart.series.length === 3) {
                    this.chart.series[2].remove();
                }
            }
        }
    }
    return RefreshingMinMaxAvgChart;
});