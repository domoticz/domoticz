define(['app', 'log/Chart'], function (app) {

    app.factory('counterLogSeriesSupplier', function (chart) {
        return {
            dataItemsKeysPredicatedSeriesSupplier: dataItemsKeysPredicatedSeriesSupplier,
            summingSeriesSupplier: summingSeriesSupplier,
            counterCompareSeriesSuppliers: counterCompareSeriesSuppliers,
            fillMissingDays: fillMissingDays
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
                                postprocessYaxis: function (yAxis) {
									if (this.dataSupplier.deviceCounterName !== undefined) {
										yAxis.options.title.text = this.dataSupplier.deviceCounterName;
									}
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

        function fillMissingDays(data) {
            // Fill in truly missing days (no DB row for a day) and spread the
            // accumulated value on the return day evenly across the gap.
            // Zero-value entries that are present in the DB are passed through
            // unchanged — only absent dates are filled.
            if (!data || !data.result || !Array.isArray(data.result)) return;

            const result = [];
            let lastDate = null;
            let lastItem = null;

            data.result.forEach(function(item) {
                if (!item.d) {
                    result.push(item);
                    return;
                }

                const v = parseFloat(item.v);
                const currentDate = new Date(item.d);

                if (lastDate !== null && v > 0) {
                    const daysDiff = Math.round((currentDate - lastDate) / (1000 * 60 * 60 * 24));

                    if (daysDiff > 1) {
                        // Missing days between last entry and this one — fill and spread value
                        const avgValue = v / daysDiff;
                        const avgV2 = item.v2 ? parseFloat(item.v2) / daysDiff : 0;
                        const avgPrice = item.p ? parseFloat(item.p) / daysDiff : 0;
                        const startCounter = parseFloat(lastItem && lastItem.c ? lastItem.c : 0);
                        const endCounter = parseFloat(item.c || 0);
                        const counterIncrement = (endCounter - startCounter) / daysDiff;

                        for (let i = 1; i < daysDiff; i++) {
                            const fillDate = new Date(lastDate);
                            fillDate.setDate(fillDate.getDate() + i);
                            const filledItem = { d: fillDate.toISOString().split('T')[0] };
                            filledItem.v = avgValue.toFixed(3);
                            if (item.p) filledItem.p = avgPrice.toFixed(4);
                            if (item.c) filledItem.c = (startCounter + counterIncrement * i).toFixed(3);
                            if (item.v2) filledItem.v2 = avgV2.toFixed(3);
                            result.push(filledItem);
                        }
                        item.v = avgValue.toFixed(3);
                        if (item.p) item.p = avgPrice.toFixed(4);
                        if (item.v2) item.v2 = avgV2.toFixed(3);
                    }
                }

                result.push(item);
                lastDate = currentDate;
                lastItem = item;
            });

            data.result = result;
        }
    });

});
