define(function () {
    function DataLoader() {

    }

    DataLoader.prototype.prepareData = function (data, receiver) {
        if (receiver.dataSupplier.preprocessData !== undefined) {
            receiver.dataSupplier.preprocessData(data);
        }
        if (receiver.dataSupplier.preprocessDataItems !== undefined) {
            receiver.dataSupplier.preprocessDataItems(data.result);
            if (data.resultprev !== undefined) {
                receiver.dataSupplier.preprocessDataItems(data.resultprev);
            }
        }
        receiver.seriesSuppliers = completeSeriesSuppliers(receiver.dataSupplier);

        function completeSeriesSuppliers(dataSupplier) {
            const seriesSuppliers = typeof dataSupplier.seriesSuppliers === 'function' ? dataSupplier.seriesSuppliers(data) : dataSupplier.seriesSuppliers;
            return seriesSuppliers
                .map(function (seriesSupplierOrFunction) {
                    if (typeof seriesSupplierOrFunction === 'function') {
                        return seriesSupplierOrFunction(dataSupplier);
                    } else {
                        return seriesSupplierOrFunction;
                    }
                })
                .map(function (seriesSupplier) {
                    return _.merge({
                        useDataItemsFromPrevious: false,
                        dataSupplier: dataSupplier,
                        dataItemKeys: [],
                        dataItemIsValid:
                            function (dataItem) {
                                return this.dataItemKeys.every(function (dataItemKey) {
                                    return dataItem[dataItemKey] !== undefined;
                                });
                            },
                        initialiseDatapoints: function () {
                            this.datapoints = [];
                        },
                        acceptDatapointFromDataItem: function (dataItem, datapoint) {
                            this.datapoints.push(datapoint);
                        },
                        datapointFromDataItem:
                            function (dataItem) {
                                const datapoint = [this.timestampFromDataItem(dataItem)];
                                this.valuesFromDataItem(dataItem).forEach(function (valueFromDataItem) {
                                    datapoint.push(valueFromDataItem);
                                });
                                return datapoint;
                            },
                        valuesFromDataItem:
                            function (dataItem) {
                                const self = this;
                                const valueFromDataItem = this.valueFromDataItem;
                                return this.dataItemKeys.map(function (dataItemKey) {
                                    return self.valueFromDataItem.call(self, (dataItem[dataItemKey]));
                                });
                            },
                        valueFromDataItem:
                            function (dataItemValue) {
                                const parsedValue = parseFloat(dataItemValue);
                                const processedValue =
                                    this.postprocessDataItemValue === undefined ? parsedValue : this.postprocessDataItemValue(parsedValue);
                                return this.convertZeroToNull && processedValue === 0 ? null : processedValue;
                            },
                        timestampFromDataItem: function (dataItem) {
                            if (!this.useDataItemsFromPrevious) {
                                return dataSupplier.timestampFromDataItem(dataItem);
                            } else {
                                return dataSupplier.timestampFromDataItem(dataItem, 1);
                            }
                        }
                    }, seriesSupplier);
                });
        }
    };

    DataLoader.prototype.loadData = function (data, receiver) {
        const seriesSuppliersOnData = receiver.seriesSuppliers.filter(function (seriesSupplier) {
            return seriesSupplier.dataIsValid === undefined || seriesSupplier.dataIsValid(data);
        });
        const seriesSuppliersOnCurrent = seriesSuppliersOnData.filter(function (seriesSupplier) {
            return !seriesSupplier.useDataItemsFromPrevious;
        });
        const seriesSuppliersOnPrevious = seriesSuppliersOnData.filter(function (seriesSupplier) {
            return seriesSupplier.useDataItemsFromPrevious;
        });

        analyseDataItems(data.result, seriesSuppliersOnCurrent);
        collectDatapoints(data.result, seriesSuppliersOnCurrent);
        if (data.resultprev !== undefined) {
            analyseDataItems(data.resultprev, seriesSuppliersOnPrevious);
            collectDatapoints(data.resultprev, seriesSuppliersOnPrevious);
        }

        function analyseDataItems(dataItems, seriesSuppliers) {
            runSeriesSuppliersFunction(
                dataItems.slice(0, 150),
                seriesSuppliers
                    .filter(function (seriesSupplier) {
                        return seriesSupplier.analyseDataItem !== undefined;
                    }),
                function (seriesSupplier, dataItem) {
                    seriesSupplier.analyseDataItem(dataItem);
                }
            );
        }

        function collectDatapoints(dataItems, seriesSuppliers) {
            runSeriesSuppliersFunction(
                dataItems,
                seriesSuppliers.filter(function (seriesSupplier) {
                    return seriesSupplier.valuesFromDataItem !== undefined;
                }),
                function (seriesSupplier, dataItem) {
                    if (seriesSupplier.dataItemIsValid === undefined || seriesSupplier.dataItemIsValid(dataItem)) {
                        seriesSupplier.acceptDatapointFromDataItem(dataItem, seriesSupplier.datapointFromDataItem(dataItem));
                    }
                },
                function (seriesSupplier) {
                    seriesSupplier.initialiseDatapoints();
                },
                function (seriesSupplier) {
                    if (seriesSupplier.postprocessDatapoints !== undefined) {
                        seriesSupplier.postprocessDatapoints.call(seriesSupplier, seriesSupplier.datapoints);
                    }
                }
            );
        }

        function runSeriesSuppliersFunction(dataItems, seriesSuppliers, dataItemFunction, starterFunction, finisherFunction) {
            if (seriesSuppliers.length !== 0) {
                if (starterFunction !== undefined) {
                    seriesSuppliers.forEach(starterFunction);
                }
                dataItems.forEach(function (dataItem) {
                    seriesSuppliers
                        .forEach(function (seriesSupplier) {
                            dataItemFunction(seriesSupplier, dataItem);
                        });
                });
                if (finisherFunction !== undefined) {
                    seriesSuppliers.forEach(finisherFunction);
                }
            }
        }
    }

    return DataLoader;
});
