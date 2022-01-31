define(['Base'], function (Base) {
    function ChartZoomer() {

    }

    ChartZoomer.prototype.analyseChart = function (receiver) {
        return analyseChartEdges(receiver.chart);

        function analyseChartEdges(chart) {
            if (receiver.seriesSuppliers === undefined) {
                return {
                    chartEdgeLeft: -1,
                    chartEdgeRight: -1
                };
            }
            const timestamps = seriesSuppliersDatapointsTimestamps(receiver.seriesSuppliers);
            const chartEdgeLeft = timestamps.reduce(min, Infinity);
            const chartEdgeRight = timestamps.reduce(max, -Infinity);
            receiver.consoledebug(
                'chartEdgeLeft:' + chartEdgeLeft + ' (' + Base.dateToString(chartEdgeLeft) + ')'
                + ', chartEdgeRight:' + chartEdgeRight + ' (' + Base.dateToString(chartEdgeRight) + ')');
            return {
                chartEdgeLeft: chartEdgeLeft,
                chartEdgeRight: chartEdgeRight
            };
        }
    };

    ChartZoomer.prototype.zoomChart = function (receiver, chartAnalysis) {
        let zoomEdgeLeft;
        let zoomEdgeRight;
        let dataEdgeLeft;
        let dataEdgeRight;

        if (receiver.isZoomed) {
            analyseZoomEdges(receiver.chart);
            analyseDataEdges(receiver.seriesSuppliers);
            applyNewZoomEdges(receiver)
        }

        function analyseZoomEdges(chart) {
            zoomEdgeLeft = getZoomEdgeLeft();
            zoomEdgeRight = getZoomEdgeRight();
            receiver.consoledebug(
                'zoomEdgeLeft:' + zoomEdgeLeft + ' (' + Base.dateToString(zoomEdgeLeft) + ')'
                + ', zoomEdgeRight:' + zoomEdgeRight + ' (' + Base.dateToString(zoomEdgeRight) + ')');

            function getZoomEdgeLeft() {
                if (chart.series.length === 0) {
                    return -1;
                }
                return chart.xAxis[0].min;
            }

            function getZoomEdgeRight() {
                if (chart.series.length === 0) {
                    return -1;
                }
                return chart.xAxis[0].max;
            }
        }

        function analyseDataEdges(seriesSuppliers) {
            const timestamps = seriesSuppliersDatapointsTimestamps(seriesSuppliers);
            dataEdgeLeft = timestamps.reduce(min, Infinity);
            dataEdgeRight = timestamps.reduce(max, -Infinity);
            receiver.consoledebug(
                'dataEdgeLeft:' + dataEdgeLeft + ' (' + Base.dateToString(dataEdgeLeft) + ')'
                + ', dataEdgeRight:' + dataEdgeRight + ' (' + Base.dateToString(dataEdgeRight) + ')');
        }

        function applyNewZoomEdges(receiver) {
            const zoomEdgeRight1 =
                receiver.isZoomRightSticky || zoomEdgeRight === -1 || chartAnalysis.chartEdgeRight === -1
                    ? zoomEdgeRight
                    : dataEdgeRight + (zoomEdgeRight - chartAnalysis.chartEdgeRight);
            const zoomEdgeLeft1 =
                receiver.isZoomLeftSticky || zoomEdgeLeft === -1 || chartAnalysis.chartEdgeLeft === -1
                    ? zoomEdgeLeft
                    : (zoomEdgeLeft - zoomEdgeRight) + zoomEdgeRight1;
            if (zoomEdgeLeft1 !== zoomEdgeLeft || zoomEdgeRight1 !== zoomEdgeRight) {
                receiver.consoledebug(
                    'zoomEdgeLeft1:' + zoomEdgeLeft1 + ' (' + Base.dateToString(zoomEdgeLeft1) + ')'
                    + ', zoomEdgeRight1:' + zoomEdgeRight1 + ' (' + Base.dateToString(zoomEdgeRight1) + ')');
                receiver.chart.xAxis[0].setExtremes(zoomEdgeLeft1, zoomEdgeRight1, false);
            }
        }
    };

    function seriesSuppliersDatapointsTimestamps(seriesSuppliers) {
        return seriesSuppliers
            .filter(function (seriesSupplier) {
                return seriesSupplier.datapoints !== undefined && seriesSupplier.datapoints.length !== 0;
            })
            .flatMap(function (seriesSupplier) {
                return [seriesSupplier.datapoints[0][0], seriesSupplier.datapoints[seriesSupplier.datapoints.length - 1][0]];
            });
    }

    function min(valueMin, value) {
        return Math.min(value, valueMin);
    }

    function max(valueMax, value) {
        return Math.max(value, valueMax);
    }

    return ChartZoomer;
});
