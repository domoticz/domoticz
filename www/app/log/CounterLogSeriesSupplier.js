define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogSeriesSupplier', function (chart) {
        return {
            dataItemsKeysPredicatedSeriesSupplier: dataItemsKeysPredicatedSeriesSupplier,
            summingSeriesSupplier: summingSeriesSupplier,
            counterCompareSeriesSuppliers: counterCompareSeriesSuppliers
        };

        function dataItemsKeysPredicatedSeriesSupplier(dataItemValueKey, dataSeriesItemsKeysPredicate, seriesSupplier) {
            return _.merge(
                {
                    valueDecimals: 0,
                    dataSeriesItemsKeys: [],
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
                        return dataSeriesItemsKeysPredicate.test(this.dataSeriesItemsKeys) && dataItem[dataItemValueKey] !== undefined;
                    },
                    valuesFromDataItem: function (dataItem) {
                        return [parseFloat(dataItem[dataItemValueKey])];
                    },
                    template: function () {
                        return _.merge(
                            {
                                tooltip: {
                                    valueDecimals: seriesSupplier.valueDecimals
                                }
                            },
                            seriesSupplier.series
                        );
                    }
                },
                seriesSupplier
            );
        }

        function summingSeriesSupplier(seriesSupplier) {
            return _.merge(
                {
                    dataItemKeys: [],
                    dataItemIsValid:
                        function (dataItem) {
                            return this.dataItemKeys.length !== 0 && dataItem[this.dataItemKeys[0]] !== undefined;
                        },
                    valuesFromDataItem: function (dataItem) {
                        return [this.dataItemKeys.reduce(addDataItemValue, 0.0)];

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
                                    valueDecimals: seriesSupplier.valueDecimals
                                }
                            },
                            seriesSupplier.series
                        );
                    }
                },
                seriesSupplier
            );
        }

        function counterCompareSeriesSuppliers(ctrl) {
            return function (data) {
                return _.range(data.firstYear, new Date().getFullYear() + 1)
                    .reduce(
                        function (seriesSuppliers, year) {
                            return seriesSuppliers.concat({
                                id: year.toString(),
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
                                datapointFromDataItem: function (dataItem) {
                                    return {
                                        y: this.valueFromDataItem(dataItem["s"]),
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
