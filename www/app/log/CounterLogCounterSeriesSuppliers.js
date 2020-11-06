define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogCounterSeriesSuppliers', function (chart) {
        return {
            counterSeriesSuppliers: counterSeriesSuppliers,
            counterTrendlineSeriesSuppliers: counterTrendlineSeriesSuppliers,
            counterPreviousSeriesSupplier: counterPreviousSeriesSupplier
        };

        function counterSeriesSuppliers(deviceType) {
            return [
                {
                    id: 'gas',
                    dataItemKeys: ['v'],
                    convertZeroToNull: true,
                    label: 'A',
                    template: {
                        type: 'column',
                        name: 'Gas',
                        zIndex: 2,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.gas(chart.valueMultipliers.m1),
                            valueDecimals: 3
                        },
                        color: 'rgba(3,190,252,0.8)',
                        yAxis: 0
                    }
                }
            ];
        }

        function counterTrendlineSeriesSuppliers(deviceType) {
            return [
                {
                    id: 'usage_trendline',
                    dataItemKeys: ['v'],
                    aggregateDatapoints: chart.trendlineAggregator,
                    label: 'B',
                    template: {
                        name: $.t('Trendline') + ' ' + $.t('Gas'),
                        zIndex: 3,
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.gas(chart.valueMultipliers.m1),
                            valueDecimals: 3
                        },
                        color: 'rgba(252,3,3,0.8)',
                        dashStyle: 'LongDash',
                        yAxis: 0,
                        visible: false
                    }
                }
            ];
        }

        function counterPreviousSeriesSupplier(deviceType) {
            return [
                {
                    id: 'gasprev',
                    useDataItemsFromPrevious: true,
                    label: 'C',
                    template: {
                        name: $.t('Past') + ' ' + $.t('Gas'),
                        tooltip: {
                            valueSuffix: ' ' + chart.valueUnits.gas(chart.valueMultipliers.m1),
                            valueDecimals: 3
                        },
                        color: 'rgba(190,3,252,0.8)',
                        yAxis: 0,
                        visible: false
                    }
                }
            ];
        }
    });

});