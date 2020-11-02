define(function () {
    function DataLoader() {

    }

    DataLoader.prototype.loadData = function (data, receiver) {
        const seriesSuppliersOnCurrent = receiver.seriesSuppliers.filter(function (seriesSupplier) {
            return !seriesSupplier.useDataItemsFromPrevious;
        });
        const seriesSuppliersOnPrevious = receiver.seriesSuppliers.filter(function (seriesSupplier) {
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
                dataItems.slice(0, 48),
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
                        const datapoint = [seriesSupplier.timestampFromDataItem(dataItem)];
                        seriesSupplier.valuesFromDataItem(dataItem).forEach(function (valueFromDataItem) {
                            datapoint.push(seriesSupplier.convertZeroToNull && valueFromDataItem === 0 ? null : valueFromDataItem);
                        });
                        seriesSupplier.datapoints.push(datapoint);
                    }
                },
                function (seriesSupplier) {
                    seriesSupplier.datapoints = [];
                },
                function (seriesSupplier) {
                    if (seriesSupplier.aggregateDatapoints !== undefined) {
                        seriesSupplier.aggregateDatapoints(seriesSupplier.datapoints);
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
