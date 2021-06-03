define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogCounterSeriesSuppliers', function (chart) {
        return {
            counterSeriesSuppliers: counterSeriesSuppliers,
            counterTrendlineSeriesSuppliers: counterTrendlineSeriesSuppliers,
            counterPreviousSeriesSupplier: counterPreviousSeriesSupplier
        };

        function deviceTypeCounterName(deviceTypeIndex) {
            const deviceType = chart.deviceTypes.fromIndex(deviceTypeIndex);
            if (deviceType !== undefined) {
                return chart.deviceCounterName[deviceType];
            }
            return undefined;
        }

        function deviceTypeValueUnit(deviceTypeIndex) {
            const deviceType = chart.deviceTypes.fromIndex(deviceTypeIndex);
            if (deviceType !== undefined) {
                const deviceTypeValueUnits = chart.valueUnits[chart.deviceCounterSubtype[deviceType]];
                if (deviceTypeValueUnits !== undefined) {
                    return deviceTypeValueUnits(chart.valueMultipliers.m1);
                }
            }
            return undefined;
        }

        function counterSeriesSuppliers(deviceTypeIndex, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
                {
                    id: 'counter',
                    dataItemKeys: ['v'],
                    convertZeroToNull: true,
                    postprocessDataItemValue: postprocessDataItemValue,
                    postprocessYaxis: function (yAxis) {
                        if (this.dataSupplier.deviceCounterName !== undefined) {
                            yAxis.options.title.text = this.dataSupplier.deviceCounterName;
                        }
                    },
                    label: 'A',
                    template: function (seriesSupplier) {
                        return {
                            type: 'column',
                            name:
                                seriesSupplier.dataSupplier.deviceCounterName !== undefined
                                    ? seriesSupplier.dataSupplier.deviceCounterName
                                    : deviceTypeCounterName(deviceTypeIndex),
                            zIndex: 2,
                            tooltip: {
                                valueSuffix: ' '
                                    + (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
                                        ? seriesSupplier.dataSupplier.deviceValueUnit
                                        : deviceTypeValueUnit(deviceTypeIndex)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgba(3,190,252,0.8)',
                            yAxis: 0
                        };
                    }
                }
            ];
        }

        function counterTrendlineSeriesSuppliers(deviceTypeIndex, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
                {
                    id: 'counterTrendlineOverall',
                    dataItemKeys: ['v'],
                    postprocessDataItemValue: postprocessDataItemValue,
                    postprocessDatapoints: chart.aggregateTrendline,
                    label: 'B',
                    template: function (seriesSupplier) {
                        return {
                            name: $.t('Trendline') + ' '
                                + (seriesSupplier.dataSupplier.deviceCounterName !== undefined
                                    ? seriesSupplier.dataSupplier.deviceCounterName
                                    : deviceTypeCounterName(deviceTypeIndex))
                                + ' ' + $.t('Overall'),
                            zIndex: 3,
                            tooltip: {
                                valueSuffix: ' '
                                    + (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
                                        ? seriesSupplier.dataSupplier.deviceValueUnit
                                        : deviceTypeValueUnit(deviceTypeIndex)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgba(252,3,3,0.8)',
                            dashStyle: 'LongDash',
                            yAxis: 0,
                            visible: false
                        };
                    }
                },
                {
                    id: 'counterTrendlineZoomed',
                    dataItemKeys: ['v'],
                    postprocessDataItemValue: postprocessDataItemValue,
                    postprocessDatapoints: chart.aggregateTrendlineZoomed,
                    chartZoomLevelChanged: function (chart, zoomLeft, zoomRight) {
                        this.reaggregateTrendlineZoomed(chart, zoomLeft, zoomRight);
                    },
                    label: 'C',
                    template: function (seriesSupplier) {
                        return {
                            name: $.t('Trendline') + ' '
                                + (seriesSupplier.dataSupplier.deviceCounterName !== undefined
                                    ? seriesSupplier.dataSupplier.deviceCounterName
                                    : deviceTypeCounterName(deviceTypeIndex)),
                            zIndex: 3,
                            tooltip: {
                                valueSuffix: ' '
                                    + (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
                                        ? seriesSupplier.dataSupplier.deviceValueUnit
                                        : deviceTypeValueUnit(deviceTypeIndex)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgb(252,3,3,0.8)',
                            dashStyle: 'DashDot',
                            yAxis: 0,
                            visible: false
                        };
                    }
                }
            ];
        }

        function counterPreviousSeriesSupplier(deviceTypeIndex, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
                {
                    id: 'counterPrevious',
                    dataItemKeys: ['v'],
                    useDataItemsFromPrevious: true,
                    postprocessDataItemValue: postprocessDataItemValue,
                    label: 'D',
                    template: function (seriesSupplier) {
                        return {
                            name: $.t('Past') + ' '
                                + (seriesSupplier.dataSupplier.deviceCounterName !== undefined
                                    ? seriesSupplier.dataSupplier.deviceCounterName
                                    : deviceTypeCounterName(deviceTypeIndex)),
                            tooltip: {
                                valueSuffix: ' '
                                    + (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
                                        ? seriesSupplier.dataSupplier.deviceValueUnit
                                        : deviceTypeValueUnit(deviceTypeIndex)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgba(190,3,252,0.8)',
                            yAxis: 0,
                            visible: false
                        };
                    }
                }
            ];
        }
    });
});