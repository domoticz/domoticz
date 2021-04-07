define(['app'], function (app) {

    app.factory('counterLogSeriesSupplier', function () {
        return {
            dataItemsKeysPredicatedSeriesSupplier: dataItemsKeysPredicatedSeriesSupplier,
            summingSeriesSupplier: summingSeriesSupplier,
            counterCompareYearSeriesSuppliers: counterCompareYearSeriesSuppliers
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

        function counterCompareYearSeriesSuppliers(deviceTypeIndex) {
            return function (data) {
                return _.range(data.firstYear, new Date().getFullYear() + 1)
                    .reduce(
                        function (seriesSuppliers, year) {
                            return seriesSuppliers.concat({
                                id: year.toString(),
                                year: year,
                                template: {
                                    name: year.toString()
                                },
                                postprocessXaxis: function (xAxis) {
                                    xAxis.categories =
                                        _.range(this.dataSupplier.firstYear, new Date().getFullYear() + 1)
                                            .map(year => year.toString())
                                            .reduce((categories, year) => {
                                                    if (!categories.includes(year)) {
                                                        categories.push(year);
                                                    }
                                                    return categories;
                                                },
                                                (xAxis.categories === true ? [] : xAxis.categories)
                                            )
                                            .sort();
                                },
                                datapointFromDataItem: function (dataItem) {
                                    const datapoint = [];
                                    this.valuesFromDataItem(dataItem).forEach(function (valueFromDataItem) {
                                        datapoint.push(valueFromDataItem);
                                    });
                                    return datapoint;
                                },
                                valuesFromDataItem: function (dataItem) {
                                    const year = this.valueFromDataItem(dataItem["y"]);
                                    if (year !== this.year) {
                                        return [null];
                                    }
                                    const previousLast = this.valueFromDataItem(dataItem["pl"]);
                                    const currentFirst = this.valueFromDataItem(dataItem["cf"]);
                                    const currentLast = this.valueFromDataItem(dataItem["cl"]);
                                    if (currentLast == null || currentFirst == null && previousLast == null) {
                                        return [null];
                                    }
                                    return [currentLast - (previousLast || currentFirst)];
                                }
                            });
                        },
                        []
                    );
            }
        }
    });

});
