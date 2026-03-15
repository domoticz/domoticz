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
            // Fill in missing days and spread zero-value runs in data.result to avoid spikes.
            // Handles two cases:
            //   1. Missing date entries (no DB row for a day)
            //   2. Zero-value entries present in DB (device offline — counter unchanged)
            // In both cases the accumulated value on the return day is spread evenly.
            if (!data || !data.result || !Array.isArray(data.result)) return;

            const result = [];
            let lastNonZeroDate = null;
            let lastNonZeroItem = null;
            let pendingZeroItems = []; // zero-value items since last non-zero item

            data.result.forEach(function(item) {
                if (!item.d) {
                    result.push(item);
                    return;
                }

                const v = parseFloat(item.v);
                if (!(v > 0)) {
                    // Zero or NaN value — buffer until we see the next non-zero day
                    pendingZeroItems.push(item);
                    return;
                }

                // Non-zero item: compute gap from last non-zero item (spans missing + zero days)
                if (lastNonZeroDate !== null) {
                    const currentDate = new Date(item.d);
                    const daysDiff = Math.round((currentDate - lastNonZeroDate) / (1000 * 60 * 60 * 24));

                    if (daysDiff > 1) {
                        const avgValue = v / daysDiff;
                        const avgV2 = item.v2 ? parseFloat(item.v2) / daysDiff : 0;
                        const avgPrice = item.p ? parseFloat(item.p) / daysDiff : 0;
                        const startCounter = parseFloat(lastNonZeroItem.c || 0);
                        const endCounter = parseFloat(item.c || 0);
                        const counterIncrement = (endCounter - startCounter) / daysDiff;

                        for (let i = 1; i < daysDiff; i++) {
                            const fillDate = new Date(lastNonZeroDate);
                            fillDate.setDate(fillDate.getDate() + i);
                            const dateStr = fillDate.toISOString().split('T')[0];

                            // Reuse existing zero-value item for this date if available
                            const existingIdx = pendingZeroItems.findIndex(function(zi) { return zi.d === dateStr; });
                            let filledItem;
                            if (existingIdx >= 0) {
                                filledItem = pendingZeroItems.splice(existingIdx, 1)[0];
                            } else {
                                filledItem = { d: dateStr };
                            }
                            filledItem.v = avgValue.toFixed(3);
                            if (item.p) filledItem.p = avgPrice.toFixed(4);
                            if (item.c) filledItem.c = (startCounter + counterIncrement * i).toFixed(3);
                            if (item.v2) filledItem.v2 = avgV2.toFixed(3);
                            result.push(filledItem);
                        }
                        item.v = avgValue.toFixed(3);
                        if (item.p) item.p = avgPrice.toFixed(4);
                        if (item.v2) item.v2 = avgV2.toFixed(3);
                    } else {
                        // No gap — push any buffered zero items unchanged
                        pendingZeroItems.forEach(function(zi) { result.push(zi); });
                    }
                } else {
                    // No previous non-zero item yet — push buffered zeros unchanged
                    pendingZeroItems.forEach(function(zi) { result.push(zi); });
                }

                pendingZeroItems = [];
                result.push(item);
                lastNonZeroDate = new Date(item.d);
                lastNonZeroItem = item;
            });

            // Flush any trailing zero items
            pendingZeroItems.forEach(function(zi) { result.push(zi); });

            data.result = result;
        }
    });

});
