define(function () {
    function ChartZoomer() {

    }

    ChartZoomer.prototype.analyseChart = function (receiver) {
        return analyseChartEdges(receiver.chart);

        function analyseChartEdges(chart) {
            const timestamps = seriesSuppliersDatapointsTimestamps(receiver.seriesSuppliers);
            const chartEdgeLeft = timestamps.reduce(min, Infinity);
            const chartEdgeRight = timestamps.reduce(max, -Infinity);
            receiver.consoledebug(
                'chartEdgeLeft:' + chartEdgeLeft + ' (' + new Date(chartEdgeLeft).toString() + ')'
                + ', chartEdgeRight:' + chartEdgeRight + ' (' + new Date(chartEdgeRight).toString() + ')');
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
                'zoomEdgeLeft:' + zoomEdgeLeft + ' (' + new Date(zoomEdgeLeft).toString() + ')'
                + ', zoomEdgeRight:' + zoomEdgeRight + ' (' + new Date(zoomEdgeRight).toString() + ')');

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
                'dataEdgeLeft:' + dataEdgeLeft + ' (' + new Date(dataEdgeLeft).toString() + ')'
                + ', dataEdgeRight:' + dataEdgeRight + ' (' + new Date(dataEdgeRight).toString() + ')');
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
                    'zoomEdgeLeft1:' + zoomEdgeLeft1 + ' (' + new Date(zoomEdgeLeft1).toString() + ')'
                    + ', zoomEdgeRight1:' + zoomEdgeRight1 + ' (' + new Date(zoomEdgeRight1).toString() + ')');
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
