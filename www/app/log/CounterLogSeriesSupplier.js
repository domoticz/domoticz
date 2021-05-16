define(['app'], function (app) {

    app.factory('counterLogSeriesSupplier', function () {
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
                            const seriesColor = yearColor(year);

                            return seriesSuppliers.concat({
                                id: year.toString(),
                                year: year,
                                template: {
                                    name: year.toString(),
                                    color: seriesColor
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
                                    const datapoint = [];
                                    this.valuesFromDataItem(dataItem).forEach(function (valueFromDataItem) {
                                        datapoint.push(valueFromDataItem);
                                    });
                                    return datapoint;
                                },
                                valuesFromDataItem: function (dataItem) {
                                    return [this.valueFromDataItem(dataItem["s"])];
                                }
                            });

                            function yearColor(year) {
                                const s1 = .6;
                                const s2 = .8;
                                const s3 = 1;
                                const v1 = 1;
                                let h = 0;
                                const h1 = 20;
                                const yearColors0 = {
                                    '2010': toColor(hsv2rgb(h =h1*0,s1,v1)),
                                    '2011': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2012': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2013': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2014': toColor(hsv2rgb(h+=h1*3,s1,v1)),
                                    '2015': toColor(hsv2rgb(h+=h1*3,s1,v1)),
                                    '2016': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2017': toColor(hsv2rgb(h+=h1*2,s1,v1)),
                                    '2018': toColor(hsv2rgb(h+=h1*2,s1,v1)),
                                    '2019': toColor(hsv2rgb(h+=h1*3,s1,v1)),
                                    '2020': toColor(hsv2rgb(h =h1*0,s2,v1)),
                                    '2021': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2022': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2023': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2024': toColor(hsv2rgb(h+=h1*3,s2,v1)),
                                    '2025': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2026': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2027': toColor(hsv2rgb(h+=h1*2,s2,v1)),
                                    '2028': toColor(hsv2rgb(h+=h1*2,s2,v1)),
                                    '2029': toColor(hsv2rgb(h+=h1*3,s2,v1)),
                                    '2030': toColor(hsv2rgb(h =h1*0,s3,v1)),
                                    '2031': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2032': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2033': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2034': toColor(hsv2rgb(h+=h1*3,s3,v1)),
                                    '2035': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2036': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2037': toColor(hsv2rgb(h+=h1*2,s3,v1)),
                                    '2038': toColor(hsv2rgb(h+=h1*2,s3,v1)),
                                    '2039': toColor(hsv2rgb(h+=h1*3,s3,v1)),
                                }
                                const c = .02;
                                const yearColors1 = {
                                    '2010': toColor(hsv2rgb(h =h1*0,s1,v1)),
                                    '2011': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2012': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2013': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2014': toColor(hsv2rgb(h+=h1*3,s1,v1)),
                                    '2015': toColor(hsv2rgb(h+=h1*3,s1,v1)),
                                    '2016': toColor(hsv2rgb(h+=h1*1,s1,v1)),
                                    '2017': toColor(hsv2rgb(277, .86+c, .95+c)),
                                    '2018': toColor(hsv2rgb( 34, .75+c, .93+c)),
                                    '2019': toColor(hsv2rgb(  2, .64+c, .92+c)),
                                    '2020': toColor(hsv2rgb(209, .50+c, .97+c)),
                                    '2021': toColor(hsv2rgb(126, .44+c, .69+c)),
                                    '2022': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2023': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2024': toColor(hsv2rgb(h+=h1*3,s2,v1)),
                                    '2025': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2026': toColor(hsv2rgb(h+=h1*1,s2,v1)),
                                    '2027': toColor(hsv2rgb(h+=h1*2,s2,v1)),
                                    '2028': toColor(hsv2rgb(h+=h1*2,s2,v1)),
                                    '2029': toColor(hsv2rgb(h+=h1*3,s2,v1)),
                                    '2030': toColor(hsv2rgb(h =h1*0,s3,v1)),
                                    '2031': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2032': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2033': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2034': toColor(hsv2rgb(h+=h1*3,s3,v1)),
                                    '2035': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2036': toColor(hsv2rgb(h+=h1*1,s3,v1)),
                                    '2037': toColor(hsv2rgb(h+=h1*2,s3,v1)),
                                    '2038': toColor(hsv2rgb(h+=h1*2,s3,v1)),
                                    '2039': toColor(hsv2rgb(h+=h1*3,s3,v1)),
                                }
                                return yearColors0[year.toString()];

                                // input: h in [0,360] and s,v in [0,1] - output: r,g,b in [0,1]
                                function hsv2rgb(h,s,v) {
                                    let f= (n,k=(n+h/60)%6) => v - v*s*Math.max( Math.min(k,4-k,1), 0);
                                    return {'r':f(5),'g':f(3),'b':f(1)};
                                }

                                function toColor(rgb) {
                                    return 'rgba(' + Math.round(rgb['r']*255) + ',' + Math.round(rgb['g']*255) + ',' + Math.round(rgb['b']*255) + ',.8)';
                                }
                            }
                        },
                        []
                    );
            }
        }
    });

});
