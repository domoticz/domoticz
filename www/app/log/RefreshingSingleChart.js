define(['RefreshingChart'], function (RefreshingChart) {

    function RefreshingSingleChart(baseParams, angularParams, domoticzParams, chartParams) {
        RefreshingChart.call(this, baseParams, angularParams, domoticzParams, chartParams);
    }

    RefreshingSingleChart.prototype = Object.create(RefreshingChart.prototype);
    RefreshingSingleChart.prototype.constructor = RefreshingSingleChart;

    RefreshingSingleChart.prototype.dataPointIsShort = function () {
        return true;
    }
    RefreshingSingleChart.prototype.getDataEdgeLeft = function (data) {
        return GetLocalDateTimeFromString(data.result[0].d);
    }
    RefreshingSingleChart.prototype.getDataEdgeRight = function (data) {
        return GetLocalDateTimeFromString(data.result[data.result.length - 1].d);
    }
    RefreshingSingleChart.prototype.loadData = function (data) {
        const valueKey = this.getPointValueKey();
        const series = data.result.map(function (item) {
            return [GetLocalDateTimeFromString(item.d), parseFloat(item[valueKey])];
        });
        if (this.chart.series.length === 0) {
            this.chart.addSeries(
                {
                    showInLegend: false,
                    name: this.getSensorName(),
                    color: Highcharts.getOptions().colors[0],
                    data: series
                },
                false);
        } else {
            this.chart.series[0].setData(series, false);
        }
    }
    return RefreshingSingleChart;
});