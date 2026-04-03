define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'energy-dashboard',
        transparentBackground: true,
        label:       'Energy Dashboard',
        description: 'Full energy overview — Weather, Grid, Solar, Gas, Battery and self-sufficiency balance bar',
        category:    'Energy',
        icon:        'fa-solid fa-gauge',
        defaultW:    6,
        defaultH:    4,
        minW:        4,
        minH:        3,
        maxW:        12,
        maxH:        8,
        configSchema: [
            {
                key:     'showWeather',
                type:    'boolean',
                label:   'Show weather card',
                default: true
            },
            {
                key:     'showSolar',
                type:    'boolean',
                label:   'Show solar card',
                default: true
            },
            {
                key:     'showGas',
                type:    'boolean',
                label:   'Show gas card',
                default: true
            },
            {
                key:     'showBattery',
                type:    'boolean',
                label:   'Show battery card',
                default: true
            },
            {
                key:     'showBalance',
                type:    'boolean',
                label:   'Show energy balance bar',
                default: true
            },
            {
                key:     'refreshInterval',
                type:    'number',
                label:   'Refresh interval (seconds)',
                default: 60
            }
        ]
    });

    app.directive('ddEnergyDashboardWidget', [function() {
        return {
            restrict:         'E',
            templateUrl:      'views/dashboardDynamic/widgets/energy-dashboard.html',
            scope: {
                widgetDef: '=',
                editMode:  '<'
            },
            controllerAs:     'ctrl',
            bindToController: true,
            controller: ['$scope', '$http', '$interval', '$q', function($scope, $http, $interval, $q) {
                var ctrl = this;

                ctrl.grid        = null;
                ctrl.solar       = null;
                ctrl.gas         = null;
                ctrl.battery     = null;
                ctrl.batteryLive = null;
                ctrl.weather     = null;
                ctrl.balance     = null;
                ctrl.isNight     = false;
                ctrl.currentTime = '';
                ctrl.sunrise     = '';
                ctrl.sunset      = '';

                var cancelToken         = null;
                var cancelSettingsToken = null;
                var timer               = null;

                // Device IDs resolved from getenergydashboarddevices
                var ids = {
                    p1:          -1,
                    solar:       -1,
                    weather:     -1,
                    gas:         -1,
                    battEnergyIn:  -1,
                    battEnergyOut: -1,
                    battSoc:     -1,
                    battWatt:    -1,
                    battVolt:    -1
                };

                function parseWatt(str) {
                    if (!str) { return 0; }
                    return parseFloat(String(str).replace(' Watt', '').replace(',', '.')) || 0;
                }

                function parseKwh(str) {
                    if (!str) { return 0; }
                    return parseFloat(String(str).replace(' kWh', '').replace(',', '.')) || 0;
                }

                function computeIsNight(sunriseStr, sunsetStr) {
                    if (!sunriseStr || !sunsetStr) { return false; }
                    var now = new Date();
                    function toMinutes(s) {
                        var p = s.split(':');
                        return parseInt(p[0], 10) * 60 + parseInt(p[1], 10);
                    }
                    var nowMin = now.getHours() * 60 + now.getMinutes();
                    return nowMin < toMinutes(sunriseStr) || nowMin >= toMinutes(sunsetStr);
                }

                function applyDevices(result) {
                    if (!result || !result.length) { return; }

                    var byIdx = {};
                    result.forEach(function(item) { byIdx[String(item.idx)] = item; });

                    function get(id) { return id !== -1 ? (byIdx[String(id)] || null) : null; }

                    // Grid / P1
                    var gr = get(ids.p1);
                    if (gr) {
                        ctrl.grid = {
                            usageWatt:         parseWatt(gr.Usage),
                            usageDelivWatt:    parseWatt(gr.UsageDeliv),
                            counterToday:      gr.CounterToday || '',
                            counterDelivToday: gr.CounterDelivToday || '',
                            price:             gr.hasOwnProperty('price') ? (parseFloat(gr.price) || 0) : null
                        };
                    } else {
                        ctrl.grid = null;
                    }

                    // Solar
                    var sl = get(ids.solar);
                    if (sl) {
                        ctrl.solar = {
                            usageWatt:    parseWatt(sl.Usage),
                            counterToday: sl.CounterToday || ''
                        };
                    } else {
                        ctrl.solar = null;
                    }

                    // Weather
                    var wt = get(ids.weather);
                    if (wt) {
                        ctrl.weather = {
                            temp:        parseFloat(wt.Temp) || 0,
                            humidity:    parseFloat(wt.Humidity) || 0,
                            barometer:   parseInt(wt.Barometer, 10) || 0,
                            forecastStr: wt.ForecastStr || '',
                            dewPoint:    parseFloat(wt.DewPoint) || 0
                        };
                    } else {
                        ctrl.weather = null;
                    }

                    // Gas
                    var gs = get(ids.gas);
                    if (gs) {
                        ctrl.gas = {
                            counterToday: gs.CounterToday || '',
                            counter:      gs.Counter || ''
                        };
                    } else {
                        ctrl.gas = null;
                    }

                    // Battery energy meters
                    var batImportedKwh = 0, batExportedKwh = 0;
                    var bi = get(ids.battEnergyIn);
                    if (bi) { batImportedKwh = parseKwh(bi.CounterToday); }
                    var bo = get(ids.battEnergyOut);
                    if (bo) { batExportedKwh = parseKwh(bo.CounterToday); }

                    if (bi || bo) {
                        ctrl.battery = { importedKwh: batImportedKwh, exportedKwh: batExportedKwh };
                    } else {
                        ctrl.battery = null;
                    }

                    // Battery live (SOC, Watt, Voltage)
                    var soc = get(ids.battSoc);
                    var bw  = get(ids.battWatt);
                    var bv  = get(ids.battVolt);
                    if (soc || bw || bv) {
                        ctrl.batteryLive = {
                            soc:     soc ? soc.Data : null,
                            watt:    bw  ? (bw.Usage || bw.Data) : null,
                            voltage: bv  ? bv.Data : null
                        };
                    } else {
                        ctrl.batteryLive = null;
                    }

                    // Energy balance calculation
                    // battery_net  = energy stored net in battery today (positive = charged)
                    // house        = P1_import + solar - P1_export - battery_net
                    // self_suff    = 1 - net_grid_draw / house  (net_grid = P1_import - P1_export)
                    if (ctrl.grid && ctrl.solar) {
                        var solarKwh  = parseKwh(ctrl.solar.counterToday);
                        var importKwh = parseKwh(ctrl.grid.counterToday);
                        var exportKwh = parseKwh(ctrl.grid.counterDelivToday);
                        var batNet    = batImportedKwh - batExportedKwh;
                        var houseKwh  = importKwh + solarKwh - exportKwh - batNet;
                        var netGrid   = importKwh - exportKwh;
                        var selfSuff  = (houseKwh > 0) ? Math.max(0, (1 - Math.max(0, netGrid) / houseKwh) * 100) : 0;
                        ctrl.balance = {
                            selfSufficiency: selfSuff,
                            solarToday:      solarKwh.toFixed(1),
                            houseToday:      houseKwh,
                            batNet:          batNet
                        };
                    } else {
                        ctrl.balance = null;
                    }
                }

                function fetchDevices() {
                    var fetchIds = [];
                    if (ids.p1           !== -1) { fetchIds.push(ids.p1);           }
                    if (ids.solar        !== -1) { fetchIds.push(ids.solar);        }
                    if (ids.weather      !== -1) { fetchIds.push(ids.weather);      }
                    if (ids.gas          !== -1) { fetchIds.push(ids.gas);          }
                    if (ids.battEnergyIn !== -1) { fetchIds.push(ids.battEnergyIn); }
                    if (ids.battEnergyOut !== -1) { fetchIds.push(ids.battEnergyOut); }
                    if (ids.battSoc      !== -1) { fetchIds.push(ids.battSoc);      }
                    if (ids.battWatt     !== -1) { fetchIds.push(ids.battWatt);     }
                    if (ids.battVolt     !== -1) { fetchIds.push(ids.battVolt);     }

                    if (!fetchIds.length) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getdevices', rid: fetchIds.join(',') },
                        timeout: cancelToken.promise
                    }).then(function(resp) {
                        applyDevices(resp.data && resp.data.result);
                    }).catch(function(err) {
                        if (err.status === -1) { return; }
                    });
                }

                function load() {
                    if (cancelSettingsToken) { cancelSettingsToken.resolve(); }
                    cancelSettingsToken = $q.defer();

                    $http.get('json.htm', {
                        params:  { type: 'command', param: 'getenergydashboarddevices' },
                        timeout: cancelSettingsToken.promise
                    }).then(function(resp) {
                        var d = resp.data;
                        if (d && d.result && d.result.ESettings) {
                            var s = d.result.ESettings;
                            ids.p1            = s.idP1               || -1;
                            ids.solar         = s.idSolar            || -1;
                            ids.weather       = s.idOutsideTempSensor || -1;
                            ids.gas           = s.idGas              || -1;
                            ids.battEnergyIn  = s.idBatteryEnergyIn  || -1;
                            ids.battEnergyOut = s.idBatteryEnergyOut || -1;
                            ids.battSoc       = s.idBatterySoc       || -1;
                            ids.battWatt      = s.idBatteryWatt      || -1;
                            ids.battVolt      = s.idBatteryVolt      || -1;
                        }
                        fetchDevices();
                    }).catch(function() {
                        fetchDevices();
                    });
                }

                ctrl.batNetClass = function() {
                    if (!ctrl.balance) { return ''; }
                    return ctrl.balance.batNet >= 0 ? 'dd-energy-import' : 'dd-energy-export';
                };

                ctrl.batNetSign = function() {
                    if (!ctrl.balance) { return ''; }
                    return ctrl.balance.batNet >= 0 ? '+' : '';
                };

                ctrl.battNetClass = function() {
                    if (!ctrl.battery) { return ''; }
                    return ctrl.battery.importedKwh >= ctrl.battery.exportedKwh ? 'dd-energy-import' : 'dd-energy-export';
                };

                ctrl.battNetSign = function() {
                    if (!ctrl.battery) { return ''; }
                    return ctrl.battery.importedKwh >= ctrl.battery.exportedKwh ? '+' : '';
                };

                $scope.$on('device_update', function(event, device) {
                    if (!device || !device.idx) { return; }
                    var updIdx = String(device.idx);
                    var tracked = [ids.p1, ids.solar, ids.weather, ids.gas,
                                   ids.battEnergyIn, ids.battEnergyOut,
                                   ids.battSoc, ids.battWatt, ids.battVolt];
                    for (var i = 0; i < tracked.length; i++) {
                        if (tracked[i] !== -1 && String(tracked[i]) === updIdx) {
                            fetchDevices();
                            return;
                        }
                    }
                });

                $scope.$on('time_update', function(event, data) {
                    if (data.serverTime) {
                        ctrl.currentTime = data.serverTime.substring(11);
                    }
                    if (data.sunrise && data.sunset) {
                        ctrl.sunrise = data.sunrise;
                        ctrl.sunset  = data.sunset;
                        ctrl.isNight = computeIsNight(data.sunrise, data.sunset);
                    }
                });

                $scope.$on('dd:widget:refresh', load);

                $scope.$on('$destroy', function() {
                    if (cancelToken)         { cancelToken.resolve();         cancelToken         = null; }
                    if (cancelSettingsToken) { cancelSettingsToken.resolve(); cancelSettingsToken = null; }
                    if (timer)               { $interval.cancel(timer);       timer               = null; }
                });

                ctrl.$onInit = function() {
                    // Fetch sunrise/sunset for weather icon day/night state
                    $http.get('json.htm', {
                        params: { type: 'command', param: 'getSunRiseSet' }
                    }).then(function(resp) {
                        var data = resp.data;
                        if (data && data.Sunrise) {
                            ctrl.sunrise = data.Sunrise;
                            ctrl.sunset  = data.Sunset;
                            ctrl.isNight = computeIsNight(data.Sunrise, data.Sunset);
                        }
                    });

                    load();

                    var cfg = (ctrl.widgetDef && ctrl.widgetDef.config) || {};
                    var intervalMs = (parseInt(cfg.refreshInterval, 10) || 60) * 1000;
                    timer = $interval(load, intervalMs);
                };
            }]
        };
    }]);
});
