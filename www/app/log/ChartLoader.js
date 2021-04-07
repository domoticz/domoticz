define(['lodash'], function (_) {
    function ChartLoader(params) {
        const self = this;
        self.extendSeriesNameWithLabel = params.extendSeriesNameWithLabel;
    }

    ChartLoader.prototype.loadChart = function (receiver) {
        const self = this;
        const chart = receiver.chart;
        const seriesSuppliers = receiver.seriesSuppliers;

        loadDataInChart();
        seriesSuppliers.forEach(function (seriesSupplier) {
            if (seriesSupplier.postprocessXaxis !== undefined) {
                seriesSupplier.postprocessXaxis(seriesSupplier.xAxis);
            }
            if (seriesSupplier.postprocessYaxis !== undefined) {
                seriesSupplier.postprocessYaxis(seriesSupplier.yAxis);
            }
        });

        function loadDataInChart() {
            seriesSuppliers.forEach(function (seriesSupplier) {
                const chartSeries = chart.get(seriesSupplier.id);
                if (seriesSupplier.datapoints !== undefined && seriesSupplier.datapoints.length !== 0) {
                    if (chartSeries === undefined) {
                        const series =
                            _.merge(
                                {
                                    id: seriesSupplier.id,
                                    data: seriesSupplier.datapoints,
                                    color: seriesSupplier.colorIndex !== undefined ? Highcharts.getOptions().colors[seriesSupplier.colorIndex] : undefined
                                },
                                typeof seriesSupplier.template === 'function' ? seriesSupplier.template(seriesSupplier) : seriesSupplier.template
                            );
                        if (self.extendSeriesNameWithLabel && seriesSupplier.label !== undefined) {
                            series.name = '[' + seriesSupplier.label + '] ' + series.name;
                        }
                        const chartSeriesCreated = chart.addSeries(series, false);
                        seriesSupplier.xAxis = chartSeriesCreated.xAxis;
                        seriesSupplier.yAxis = chartSeriesCreated.yAxis;
                    } else {
                        chartSeries.setData(seriesSupplier.datapoints, false);
                        seriesSupplier.xAxis = chartSeries.xAxis;
                        seriesSupplier.yAxis = chartSeries.yAxis;
                    }
                } else {
                    if (chartSeries !== undefined) {
                        chartSeries.setData(seriesSupplier.datapoints, false);
                        seriesSupplier.xAxis = chartSeries.xAxis;
                        seriesSupplier.yAxis = chartSeries.yAxis;
                    }
                }
            });
        }
    };

    return ChartLoader;
});
