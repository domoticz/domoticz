define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogSeriesSupplier', function (chart) {
        return {
            dataItemsKeysPredicatedSeriesSupplier: dataItemsKeysPredicatedSeriesSupplier,
            summingSeriesSupplier: summingSeriesSupplier,
            counterCompareSeriesSuppliers: counterCompareSeriesSuppliers
        };

        function dataItemsKeysPredicatedSeriesSupplier(dataItemValueKey, dataSeriesItemsKeysPredicate, seriesSupplierTemplate) {
            return _.merge(
                {
                    valueDecimals: 0,
                    dataSeriesItemsKeys: [],
                    dataItemKeys: [dataItemValueKey],
                    analyseDataItem: function (dataItem) {
                        const self = this;
                        dataItemKeysCollect(dataItem);
                        valueDecimalsCalculate(dataItem);

                        function dataItemKeysCollect(dataItem) {
                            Object.keys(dataItem)
                                .filter(function (key) {
                                    return !self.dataSeriesItemsKeys.includes(key);
                                })
                                .forEach(function (key) {
                                    self.dataSeriesItemsKeys.push(key);
                                });
                        }

                        function valueDecimalsCalculate(dataItem) {
                            if (self.valueDecimals === 0 && dataItem[dataItemValueKey] % 1 !== 0) {
                                self.valueDecimals = 1;
                            }
                        }
                    },
                    dataItemIsValid: function (dataItem) {
                        return dataSeriesItemsKeysPredicate.test(this.dataSeriesItemsKeys);
                    },
                    template: function () {
                        return _.merge(
                            {
                                tooltip: {
                                    valueDecimals: seriesSupplierTemplate.valueDecimals
                                }
                            },
                            seriesSupplierTemplate.series
                        );
                    }
                },
                seriesSupplierTemplate
            );
        }

        function summingSeriesSupplier(seriesSupplierTemplate) {
            return _.merge(
                {
                    dataItemKeys: [],
                    dataItemIsComplete: function (dataItem) {
                        return this.dataItemKeys.length !== 0 && dataItem[this.dataItemKeys[0]] !== undefined;
                    },
                    valuesFromDataItem: function (dataItem) {
                        const totalValue = this.dataItemKeys.reduce(addDataItemValue, 0.0);
                        return [this.convertZeroToNull && totalValue === 0 ? null : totalValue];

                        function addDataItemValue(totalValue, key) {
                            const value = dataItem[key];
                            if (value === undefined) {
                                return totalValue;
                            }
                            return totalValue + parseFloat(value);
                        }
                    },
                    template: function () {
                        return _.merge(
                            {
                                tooltip: {
                                    valueDecimals: seriesSupplierTemplate.valueDecimals
                                }
                            },
                            seriesSupplierTemplate.series
                        );
                    }
                },
                seriesSupplierTemplate
            );
        }

        function counterCompareSeriesSuppliers(ctrl) {
            return function (data) {
                if ((data.firstYear === undefined)||(data.firstYear==0)) {
                    return [];
                }
                return _.range(data.firstYear, new Date().getFullYear() + 1)
                    .reduce(
                        function (seriesSuppliers, year) {
                            return seriesSuppliers.concat({
                                id: year.toString(),
                                convertZeroToNull: true,
                                year: year,
                                template: {
                                    name: year.toString(),
                                    color: chart.yearColor(year),
                                    index: year - data.firstYear + 1
                                    /*
                                    ,stack: ctrl.groupingBy === 'year' ? 0 : 1
                                     */
                                },
                                postprocessXaxis: function (xAxis) {
                                    // xAxis.categories =
                                    //     this.dataSupplier.categories
                                        /*.reduce((categories, category) => {
                                                if (!categories.includes(category)) {
                                                    categories.push(category);
                                                }
                                                return categories;
                                            },
                                            (xAxis.categories === true ? [] : xAxis.categories)
                                        )
                                        .sort()*/;
                                },
                                initialiseDatapoints: function () {
                                    this.datapoints = this.dataSupplier.categories.map(function (category) {
                                        return null;
                                    });
                                },
                                acceptDatapointFromDataItem: function (dataItem, datapoint) {
                                    const categoryIndex = this.dataSupplier.categories.indexOf(dataItem["c"]);
                                    if (categoryIndex !== -1) {
                                        this.datapoints[categoryIndex] = datapoint;
                                    }
                                },
                                dataItemIsValid: function (dataItem) {
                                    return this.year === parseInt(dataItem["y"]);
                                },
                                dataItemIsComplete: dataItem => true,
                                datapointFromDataItem: function (dataItem) {
                                    return {
                                        y: this.valueFromDataItemValue(dataItem["s"]),
                                        trend: dataItem["t"]
                                    };
                                }
                            });
                        },
                        []
                    );
            }
        }
    });

});
