define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogCounterSeriesSuppliers', function (chart) {
        return {
            counterSeriesDaySuppliers: counterSeriesDaySuppliers,
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

        function deviceTypeValueUnit(deviceTypeIndex, valueMultiplier=chart.valueMultipliers.m1) {
            const deviceType = chart.deviceTypes.fromIndex(deviceTypeIndex);
            if (deviceType !== undefined) {
                const deviceTypeValueUnits = chart.valueUnits[chart.deviceCounterSubtype[deviceType]];
                if (deviceTypeValueUnits !== undefined) {
                    return deviceTypeValueUnits(valueMultiplier);
                }
            }
            return undefined;
        }

        function counterSeriesDaySuppliers(deviceTypeIndex, valueMultiplier, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
               {
                    id: 'MeterUsagedArea',
                    dataItemKeys: ['mu'],
                    label: 'A',
                    template: function (seriesSupplier) {
                        return {
							type: 'area',
							name: $.t('Usage'),
							tooltip: {
								valueSuffix: ' '
									+ (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
										? seriesSupplier.dataSupplier.deviceValueUnit
										: deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
								valueDecimals: dataItemValueDecimals
							},
							color: 'rgba(225,167,124,0.9)',
							fillOpacity: 0.2,
							yAxis: 0,
							visible: false
						};
                    }
                },
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
                    label: '1',
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
                                        : deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgba(3,190,252,0.8)',
                            yAxis: 0
                        };
                    }
                }
            ];
        }

        function counterSeriesSuppliers(deviceTypeIndex, valueMultiplier, postprocessDataItemValue, dataItemValueDecimals=3) {
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
                    label: '1',
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
                                        : deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
                                valueDecimals: dataItemValueDecimals
                            },
                            color: 'rgba(3,190,252,0.8)',
                            yAxis: 0
                        };
                    }
                }
            ];
        }

        function counterTrendlineSeriesSuppliers(deviceTypeIndex, valueMultiplier=chart.valueMultipliers.m1, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
                {
                    id: 'counterTrendlineOverall',
                    dataItemKeys: ['v'],
                    postprocessDataItemValue: postprocessDataItemValue,
                    postprocessDatapoints: chart.aggregateTrendline,
                    label: '2',
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
                                        : deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
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
                    label: '3',
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
                                        : deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
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

        function counterPreviousSeriesSupplier(deviceTypeIndex, valueMultiplier=chart.valueMultipliers.m1, postprocessDataItemValue, dataItemValueDecimals=3) {
            return [
                {
                    id: 'counterPrevious',
                    dataItemKeys: ['v'],
                    useDataItemsFromPrevious: true,
                    postprocessDataItemValue: postprocessDataItemValue,
                    label: '4',
                    template: function (seriesSupplier) {
                        return {
                            name: $.t('Past') + ' '
                                + (seriesSupplier.dataSupplier.deviceCounterName !== undefined
                                    ? seriesSupplier.dataSupplier.deviceCounterName
                                    : deviceTypeCounterName(deviceTypeIndex)),
                            zIndex: 3,
                            tooltip: {
                                valueSuffix: ' '
                                    + (seriesSupplier.dataSupplier.deviceValueUnit !== undefined
                                        ? seriesSupplier.dataSupplier.deviceValueUnit
                                        : deviceTypeValueUnit(deviceTypeIndex, valueMultiplier)),
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