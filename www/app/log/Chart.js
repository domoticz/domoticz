define(['app'], function (app) {

    const deviceTypes = {
        EnergyUsed: 0,
        Gas: 1,
        Water: 2,
        Counter: 3,
        EnergyGenerated: 4,
        Time: 5,
        fromIndex: function (switchTypeVal) {
            if (switchTypeVal === this.EnergyUsed) {
                return 'EnergyUsed';
            } else if (switchTypeVal === this.Gas) {
                return 'Gas';
            } else if (switchTypeVal === this.Water) {
                return 'Water';
            } else if (switchTypeVal === this.Counter) {
                return 'Counter';
            } else if (switchTypeVal === this.EnergyGenerated) {
                return 'EnergyGenerated';
            } else if (switchTypeVal === this.Time) {
                return 'Time';
            }
        }
    };

    const deviceCounterSubtype = {
        Gas: 'gas',
        Water: 'water',
        Counter: 'counter'
    };

    const deviceCounterName = {
        Gas: 'Gas',
        Water: 'Water',
        Counter: 'Counter'
    };

    const valueMultipliers = {
        m1: 'm1',
        m1000: 'm1000',
        forRange: function (range) {
            return range === 'day' ? this.m1
                : range === 'week' ? this.m1000
                    : null;
        }
    }

    const valueUnits = {
        Wh: 'Wh',
        kWh: 'kWh',
        W: 'Watt',
        kW: 'kW',
        m3: 'm³',
        liter: 'liter',
        energy: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return valueUnits.Wh;
            }
            if (multiplier === valueMultipliers.m1000) {
                return valueUnits.kWh;
            }
            return '';
        },
        power: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return valueUnits.W;
            }
            if (multiplier === valueMultipliers.m1000) {
                return valueUnits.kW;
            }
            return '';
        },
        gas: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return valueUnits.m3;
            }
            return '';
        },
        water: function (multiplier) {
            if (multiplier === valueMultipliers.m1) {
                return valueUnits.liter;
            }
            return '';
        }
    }

    app.constant('chart', {
        baseParams: baseParams,
        angularParams: angularParams,
        domoticzParams: domoticzParams,
        deviceTypes: deviceTypes,
        deviceCounterSubtype: deviceCounterSubtype,
        deviceCounterName: deviceCounterName,
        valueMultipliers: valueMultipliers,
        valueUnits: valueUnits,
        aggregateTrendline: aggregateTrendline,
        aggregateTrendlineZoomed: aggregateTrendlineZoomed
    });

    function baseParams(jquery) {
        return {
            jquery: jquery
        };
    }

    function angularParams(location, route, scope, timeout, element) {
        return {
            location: location,
            route: route,
            scope: scope,
            timeout: timeout,
            element: element
        };
    }

    function domoticzParams(globals, api, datapointApi) {
        return {
            globals: globals,
            api: api,
            datapointApi: datapointApi
        };
    }

    function aggregateTrendline(datapoints) {
        const trendline = CalculateTrendLine(datapoints);
        datapoints.length = 0;
        if (trendline !== undefined) {
            datapoints.push([trendline.x0, trendline.y0]);
            datapoints.push([trendline.x1, trendline.y1]);
        }
    }

    function aggregateTrendlineZoomed(datapoints) {
        this.datapointsSource = datapoints.slice();
        aggregateTrendline(datapoints);
        this.reaggregateTrendlineZoomed = function (chart, zoomLeft, zoomRight) {
            this.datapoints = this.datapointsSource.filter(function (datapoint) {
                return zoomLeft <= datapoint[0] && datapoint[0] <= zoomRight;
            })
            aggregateTrendline(this.datapoints);
            chart.get(this.id).setData(this.datapoints, false);
        }
    }
});