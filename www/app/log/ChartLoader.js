define(['lodash'], function (_) {
    function ChartLoader() {

    }

    ChartLoader.prototype.loadChart = function (receiver) {
        const chart = receiver.chart;
        const seriesSuppliers = receiver.seriesSuppliers;

        loadDataInChart();

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
                        if (seriesSupplier.extendSeriesNameWithLabel && seriesSupplier.label !== undefined) {
                            series.name = '[' + seriesSupplier.label + '] ' + series.name;
                        }
                        chart.addSeries(series, false);
                    } else {
                        chartSeries.setData(seriesSupplier.datapoints, false);
                    }
                } else {
                    if (chartSeries !== undefined) {
                        chartSeries.setData(seriesSupplier.datapoints, false);
                    }
                }
            });
        }
    };

    return ChartLoader;
});
