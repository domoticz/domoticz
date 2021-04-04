define(function () {
    function DataLoader() {

    }

    DataLoader.prototype.loadData = function (data, receiver) {
        if (receiver.dataSupplier.preprocessData !== undefined) {
            receiver.dataSupplier.preprocessData(data);
        }
        if (receiver.dataSupplier.preprocessDataItems !== undefined) {
            receiver.dataSupplier.preprocessDataItems(data.result);
            if (data.resultprev !== undefined) {
                receiver.dataSupplier.preprocessDataItems(data.resultprev);
            }
        }

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
                        seriesSupplier.datapoints.push(seriesSupplier.datapointFromDataItem(dataItem));
                    }
                },
                function (seriesSupplier) {
                    seriesSupplier.datapoints = [];
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
