define([
    'app',
    'dashboardDynamic/widgetRegistry.service'
], function(app, widgetRegistry) {
    'use strict';

    widgetRegistry.register({
        type:                  'energy-dashboard',
        transparentBackground: true,
        label:       'Energy Dashboard',
        description: 'Full energy overview — Weather, Grid, Solar, Gas, Water, Battery and self-sufficiency balance bar',
        category:    'Energy',
        icon:        'fa-solid fa-gauge',
        defaultW:    6,
        defaultH:    4,
        minW:        2,
        minH:        2,
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
                key:     'showWater',
                type:    'boolean',
                label:   'Show water card',
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
                step:    1,
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
                ctrl.water       = null;
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
                    water:       -1,
                    battEnergyIn:  -1,
                    battEnergyOut: -1,
                    battSoc:     -1,
                    battWatt:    -1,
                    battVolt:    -1
                };
                ctrl.ids = ids; // exposed to template for ng-href log links

                // A card is only rendered when its device is actually configured in
                // Setup > Settings > Energy Dashboard. Until those settings are known
                // (or when they could not be fetched) every card stays visible.
                ctrl.settingsLoaded = false;

                ctrl.hasDevice = function(name) {
                    if (!ctrl.settingsLoaded) { return true; }
                    if (name === 'battery') {
                        return ids.battEnergyIn !== -1 || ids.battEnergyOut !== -1 ||
                               ids.battSoc !== -1 || ids.battWatt !== -1 || ids.battVolt !== -1;
                    }
                    return ids[name] !== -1;
                };

                ctrl.hasAnyDevice = function() {
                    return ctrl.hasDevice('p1') || ctrl.hasDevice('solar') || ctrl.hasDevice('weather') ||
                           ctrl.hasDevice('gas') || ctrl.hasDevice('water') || ctrl.hasDevice('battery');
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

                function getWeatherScene(forecastStr) {
                    if (!forecastStr) { return 'fcw-cloudy'; }
                    var s = forecastStr.toLowerCase();
                    if (s.indexOf('thunderstorm') >= 0) { return 'fcw-thunderstorm'; }
                    if (s.indexOf('heavy rain') >= 0) { return 'fcw-heavyrain'; }
                    if (s.indexOf('rain') >= 0 || s.indexOf('shower') >= 0) { return 'fcw-rain'; }
                    if (s.indexOf('heavy snow') >= 0 || s.indexOf('blizzard') >= 0) { return 'fcw-heavysnow'; }
                    if (s.indexOf('snow') >= 0 || s.indexOf('sleet') >= 0) { return 'fcw-snow'; }
                    if (s.indexOf('unstable') >= 0) { return 'fcw-cloudy'; }
                    if (s.indexOf('stable') >= 0) { return 'fcw-sunny'; }
                    if (s.indexOf('sunny') >= 0 || (s.indexOf('clear') >= 0 && s.indexOf('night') < 0)) { return 'fcw-sunny'; }
                    if (s.indexOf('partly') >= 0 || s.indexOf('scattered') >= 0 || s.indexOf('some clouds') >= 0) { return 'fcw-partlycloudy'; }
                    if (s.indexOf('night') >= 0) { return 'fcw-night'; }
                    return 'fcw-cloudy';
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
                            timeout:           gr.HaveTimeout === true,
                            price:             gr.hasOwnProperty('price') ? (parseFloat(gr.price) || 0) : null
                        };
                    } else {
                        ctrl.grid = null;
                    }

                    // Solar
                    var sl = get(ids.solar);
                    if (sl) {
                        ctrl.solar = {
                            usageWatt:    Math.round(parseWatt(sl.Usage)),
                            counterToday: sl.CounterToday || '',
                            timeout:      sl.HaveTimeout === true
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
                            timeout:     wt.HaveTimeout === true,
                            forecastStr: wt.ForecastStr || '',
                            scene:       getWeatherScene(wt.ForecastStr || ''),
                            dewPoint:    parseFloat(wt.DewPoint) || 0
                        };
                    } else {
                        ctrl.weather = null;
                    }

                    // Gas
                    var gs = get(ids.gas);
                    if (gs) {
                        var rawGasPrice = parseFloat(gs.price);
                        ctrl.gas = {
                            counterToday: gs.CounterToday || '',
                            counter:      gs.Counter || '',
                            price: (!isNaN(rawGasPrice) && rawGasPrice !== 1000 && rawGasPrice !== 0) ? rawGasPrice : null,
                            timeout:      gs.HaveTimeout === true
                        };
                    } else {
                        ctrl.gas = null;
                    }

                    // Water
                    var wm = get(ids.water);
                    if (wm) {
                        var rawWaterPrice = parseFloat(wm.price);
                        ctrl.water = {
                            counterToday: wm.CounterToday || '',
                            counter:      wm.Counter || '',
                            price: (!isNaN(rawWaterPrice) && rawWaterPrice !== 1000 && rawWaterPrice !== 0) ? rawWaterPrice : null,
                            timeout:      wm.HaveTimeout === true
                        };
                    } else {
                        ctrl.water = null;
                    }

                    // Battery energy meters
                    var batImportedKwh = 0, batExportedKwh = 0, timeout = false;
                    var bi = get(ids.battEnergyIn);
                    if (bi) { batImportedKwh = parseKwh(bi.CounterToday); timeout = bi.HaveTimeout === true; }
                    var bo = get(ids.battEnergyOut);
                    if (bo) { batExportedKwh = parseKwh(bo.CounterToday); timeout = timeout || bo.HaveTimeout === true; }

                    if (bi || bo) {
                        ctrl.battery = { importedKwh: batImportedKwh, exportedKwh: batExportedKwh, timeout: timeout };
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
                            watt:    bw  ? Math.round(parseWatt(bw.Usage || bw.Data)) : null,
                            voltage: bv  ? bv.Data : null
                        };
                    } else {
                        ctrl.batteryLive = null;
                    }

                    // Energy balance calculation
                    // battery_net  = energy stored net in battery today (positive = charged)
                    // house        = P1_import + solar - P1_export - battery_net
                    // self_suff    = min(100, (solar - P1_export + bat_discharge) / house)
                    //               subtracting P1_export removes exported solar; adding
                    //               bat_discharge cancels the battery's share of P1_export,
                    //               so the numerator reduces to (solarToHouse + batToHouse)
                    if (ctrl.grid && ctrl.solar) {
                        var solarKwh  = parseKwh(ctrl.solar.counterToday);
                        var importKwh = parseKwh(ctrl.grid.counterToday);
                        var exportKwh = parseKwh(ctrl.grid.counterDelivToday);
                        var batNet    = batImportedKwh - batExportedKwh;
                        var houseKwh  = importKwh + solarKwh - exportKwh - batNet;
                        var selfSuff  = calcSelfSufficiency(solarKwh, exportKwh, batExportedKwh, houseKwh);
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
                    if (ids.water        !== -1) { fetchIds.push(ids.water);        }
                    if (ids.battEnergyIn !== -1) { fetchIds.push(ids.battEnergyIn); }
                    if (ids.battEnergyOut !== -1) { fetchIds.push(ids.battEnergyOut); }
                    if (ids.battSoc      !== -1) { fetchIds.push(ids.battSoc);      }
                    if (ids.battWatt     !== -1) { fetchIds.push(ids.battWatt);     }
                    if (ids.battVolt     !== -1) { fetchIds.push(ids.battVolt);     }

                    if (!fetchIds.length) { return; }

                    if (cancelToken) { cancelToken.resolve(); }
                    cancelToken = $q.defer();

                    var t = cancelToken.promise;
                    var reqs = [
                        $http.get('json.htm', {
                            params:  { type: 'command', param: 'getdevices', rid: fetchIds.join(',') },
                            timeout: t
                        })
                    ];

                    // Fetch year-to-date totals in parallel when configured
                    var gasIdx   = ids.gas   !== -1 ? ids.gas   : null;
                    var waterIdx = ids.water !== -1 ? ids.water : null;
                    if (gasIdx !== null) {
                        reqs.push($http.get('json.htm', {
                            params: { type: 'command', param: 'graph', sensor: 'counter', idx: gasIdx, range: 'year', actyear: new Date().getFullYear() },
                            timeout: t
                        }));
                    }
                    if (waterIdx !== null) {
                        reqs.push($http.get('json.htm', {
                            params: { type: 'command', param: 'graph', sensor: 'counter', idx: waterIdx, range: 'year', actyear: new Date().getFullYear() },
                            timeout: t
                        }));
                    }

                    $q.all(reqs).then(function(results) {
                        applyDevices(results[0].data && results[0].data.result);

                        var rIdx = 1;
                        if (gasIdx !== null) {
                            var gasYear = results[rIdx++];
                            if (gasYear && ctrl.gas) {
                                var yearData = gasYear.data && gasYear.data.result;
                                if (yearData && yearData.length) {
                                    var sum = yearData.reduce(function(acc, item) {
                                        return acc + (parseFloat(item.v) || 0);
                                    }, 0);
                                    ctrl.gas.counterYear = sum.toFixed(3);
                                }
                            }
                        }
                        if (waterIdx !== null) {
                            var waterYear = results[rIdx++];
                            if (waterYear && ctrl.water) {
                                var wYearData = waterYear.data && waterYear.data.result;
                                if (wYearData && wYearData.length) {
                                    var wSum = wYearData.reduce(function(acc, item) {
                                        return acc + (parseFloat(item.v) || 0);
                                    }, 0);
                                    // Detect unit from counterToday (m3 or Liter)
                                    var isLiter = ctrl.water.counterToday && ctrl.water.counterToday.indexOf('Liter') >= 0;
                                    ctrl.water.counterYear = isLiter
                                        ? Math.round(wSum * 1000) + ' Liter'
                                        : wSum.toFixed(3) + ' m3';
                                }
                            }
                        }
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
                            ids.water         = s.idWater            || -1;
                            ids.battEnergyIn  = s.idBatteryEnergyIn  || -1;
                            ids.battEnergyOut = s.idBatteryEnergyOut || -1;
                            ids.battSoc       = s.idBatterySoc       || -1;
                            ids.battWatt      = s.idBatteryWatt      || -1;
                            ids.battVolt      = s.idBatteryVolt      || -1;
                            ctrl.settingsLoaded = true;
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
                    var tracked = [ids.p1, ids.solar, ids.weather, ids.gas, ids.water,
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
