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
        EnergyUsed: 'energy',
        Gas: 'gas',
        Water: 'water',
        Counter: 'counter',
        EnergyGenerated: 'energy'
    };

    const deviceCounterName = {
        EnergyUsed: 'Usage',
        Gas: 'Gas',
        Water: 'Water',
        Counter: 'Counter',
        EnergyGenerated: 'Generated'
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
            if (multiplier === valueMultipliers.m1000) {
                return valueUnits.m3;
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
        aggregateTrendlineZoomed: aggregateTrendlineZoomed,
        yearColor: yearColor,
		chartParamsCompare: chartParamsCompare,
		chartParamsWeekTemplate: chartParamsWeekTemplate,
		chartParamsCompareTemplate: chartParamsCompareTemplate,
		compareSeriesSuppliers: compareSeriesSuppliers
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

    const colors = [
        {h:130,s:.87,v:.78},
        {h: 60,s:.70,v:1  },
        {h:317,s:.68,v:.94},
        {h:  0,s:.02,v:.63},
        {h:225,s:.99,v:1  },
        {h: 46,s:1  ,v:1  },
        {h:271,s:.80,v:.88},
        {h: 29,s:1  ,v:1  },
        {h:357,s:.80,v:.90},
        {h:189,s:1  ,v:1  },
    ];

    const yearColors = {
        '2010': rgbToString(hsvToRgb(hsv(colors[0]['h'],colors[0]['s']-.2,colors[0]['v']   ))),
        '2011': rgbToString(hsvToRgb(hsv(colors[1]['h'],colors[1]['s']-.2,colors[1]['v']   ))),
        '2012': rgbToString(hsvToRgb(hsv(colors[2]['h'],colors[2]['s']-.2,colors[2]['v']   ))),
        '2013': rgbToString(hsvToRgb(hsv(colors[3]['h'],colors[3]['s']-.2,colors[3]['v']   ))),
        '2014': rgbToString(hsvToRgb(hsv(colors[4]['h'],colors[4]['s']-.2,colors[4]['v']   ))),
        '2015': rgbToString(hsvToRgb(hsv(colors[5]['h'],colors[5]['s']-.2,colors[5]['v']   ))),
        '2016': rgbToString(hsvToRgb(hsv(colors[6]['h'],colors[6]['s']-.2,colors[6]['v']   ))),
        '2017': rgbToString(hsvToRgb(hsv(colors[7]['h'],colors[7]['s']-.2,colors[7]['v']   ))),
        '2018': rgbToString(hsvToRgb(hsv(colors[8]['h'],colors[8]['s']-.2,colors[8]['v']   ))),
        '2019': rgbToString(hsvToRgb(hsv(colors[9]['h'],colors[9]['s']-.2,colors[9]['v']   ))),
        '2020': rgbToString(hsvToRgb(hsv(colors[0]['h'],colors[0]['s'],   colors[0]['v']   ))),
        '2021': rgbToString(hsvToRgb(hsv(colors[1]['h'],colors[1]['s'],   colors[1]['v']   ))),
        '2022': rgbToString(hsvToRgb(hsv(colors[2]['h'],colors[2]['s'],   colors[2]['v']   ))),
        '2023': rgbToString(hsvToRgb(hsv(colors[3]['h'],colors[3]['s'],   colors[3]['v']   ))),
        '2024': rgbToString(hsvToRgb(hsv(colors[4]['h'],colors[4]['s'],   colors[4]['v']   ))),
        '2025': rgbToString(hsvToRgb(hsv(colors[5]['h'],colors[5]['s'],   colors[5]['v']   ))),
        '2026': rgbToString(hsvToRgb(hsv(colors[6]['h'],colors[6]['s'],   colors[6]['v']   ))),
        '2027': rgbToString(hsvToRgb(hsv(colors[7]['h'],colors[7]['s'],   colors[7]['v']   ))),
        '2028': rgbToString(hsvToRgb(hsv(colors[8]['h'],colors[8]['s'],   colors[8]['v']   ))),
        '2029': rgbToString(hsvToRgb(hsv(colors[9]['h'],colors[9]['s'],   colors[9]['v']   ))),
        '2030': rgbToString(hsvToRgb(hsv(colors[0]['h'],colors[0]['s'],   colors[0]['v']-.2))),
        '2031': rgbToString(hsvToRgb(hsv(colors[1]['h'],colors[1]['s'],   colors[1]['v']-.2))),
        '2032': rgbToString(hsvToRgb(hsv(colors[2]['h'],colors[2]['s'],   colors[2]['v']-.2))),
        '2033': rgbToString(hsvToRgb(hsv(colors[3]['h'],colors[3]['s'],   colors[3]['v']-.2))),
        '2034': rgbToString(hsvToRgb(hsv(colors[4]['h'],colors[4]['s'],   colors[4]['v']-.2))),
        '2035': rgbToString(hsvToRgb(hsv(colors[5]['h'],colors[5]['s'],   colors[5]['v']-.2))),
        '2036': rgbToString(hsvToRgb(hsv(colors[6]['h'],colors[6]['s'],   colors[6]['v']-.2))),
        '2037': rgbToString(hsvToRgb(hsv(colors[7]['h'],colors[7]['s'],   colors[7]['v']-.2))),
        '2038': rgbToString(hsvToRgb(hsv(colors[8]['h'],colors[8]['s'],   colors[8]['v']-.2))),
        '2039': rgbToString(hsvToRgb(hsv(colors[9]['h'],colors[9]['s'],   colors[9]['v']-.2))),
    }

  function chartParamsCompare(domoticzGlobals, ctrl, chartParamsTemplate, dataSupplierTemplate, seriesSuppliers) {
		return _.merge(
			{
				highchartTemplate: {
					xAxis: {
						type: 'category'
					},
					plotOptions: {
						column: {
							pointPlacement: 0,
							stacking: undefined
						},
						series: {
							stacking: undefined
						}
					},
					chart: {
						marginRight: 10
					},
					tooltip: {
						crosshairs: false
					}
				},
				ctrl: ctrl,
				range: ctrl.range,
				device: ctrl.device,
				sensorType: ctrl.sensorType,
				groupingBy: ctrl.groupingBy,
				autoRefreshIsEnabled: function () {
					return ctrl.logCtrl.autoRefresh;
				},
				dataSupplier:
					_.merge(
						{
							seriesSuppliers: seriesSuppliers
						},
						dataSupplierTemplate
					)
			},
			chartParamsTemplate
		);
	}
	
	function chartParamsWeekTemplate() {
		const template = {
			highchartTemplate: {
				plotOptions: {
					column: {
						dataLabels: {
							enabled: true
						}
					}
				}
			}
		};
		return template;
	}

	function chartParamsCompareTemplate(ctrl, what, deviceUnit) {
		const template = {
			chartName: $.t('Comparing'),
			trendValuationIsReversed: function () {
				return false;
			},
			highchartTemplate: {
				chart: {
					type: 'column'
				},
				plotOptions: {
					column: {
						stacking: ctrl.groupingBy === 'year' ? 'normal' : undefined
					}
				},
				xAxis: {
					labels: {
						formatter: function () {
							return categoryKeyToString(this.value);
						}
					}
				},
				tooltip: {
					useHTML: true,
					formatter: function () {
						return ''
							+ '<table>'
							+ '<tr><td colspan="2"><b>' + categoryKeyToString(this.x) + '</b></td></tr>'
							+ this.points.reduce(
								function (rowsHtml, point) {
									return rowsHtml
										+ '<tr>'
										+ '<td><span style="color:' + point.color + '">●</span> ' + point.series.name + ': </td>'
										+ '<td><b>' + Highcharts.numberFormat(point.y) + '</b></td>'
										+ '<td><b>' + (deviceUnit ? '&nbsp;' + deviceUnit : '') + '</b></td>'
										+ '<td style="text-align: center; padding-left: 3px;">' + fontAwesomeIcon(
												trendToFontAwesomeIconNameAndSize(point.point.options.trend),
												trendToColor(point.point.options.trend)) + '</td>'
										+ '</tr>';
								}, '')
							+ '</table>';

						function trendToFontAwesomeIconNameAndSize(trend) {
							if (trend === 'up' || trend === 'down') {
								return {
									name: 'caret-' + trend,
									size: '1.3em'
								};
							}
							if (trend === 'equal') {
								return {
									name: 'equals',
									size: '0.9em'
								};
							}
						}

						function trendToColor(trend) {
							const valuation = trendValuation(trend);
							if (valuation === 'better') {
								return 'rgb(125,220,78)';
							}
							if (valuation === 'worse') {
								return 'rgb(255,107,107)';
							}
							if (valuation === 'same') {
								return 'rgb(192,192,192)';
							}
						}

						function trendValuation(trend) {
							if (template.trendValuationIsReversed()) {
								if (trend === 'up') {
									return 'better';
								}
								if (trend === 'down') {
									return 'worse';
								}
							} else {
								if (trend === 'up') {
									return 'worse';
								}
								if (trend === 'down') {
									return 'better';
								}
							}
							if (trend === 'equal') {
								return 'same';
							}
						}

						function fontAwesomeIcon(nameAndSize, color) {
							if (nameAndSize === undefined) {
								return '';
							}
							return '<i class="fas fa-' + nameAndSize.name + '" style="font-size: ' + nameAndSize.size + '; color: ' + color + ';"></i>';
						}
					}
				}
			}
		};
		return template;

		function categoryKeyToString(categoryKey) {
			return ctrl.groupingBy === 'month' ? monthToString(categoryKey) : categoryKey;
		}

		function monthToString(month) {
			const months = {
				'01': $.t('Jan'),
				'02': $.t('Feb'),
				'03': $.t('Mar'),
				'04': $.t('Apr'),
				'05': $.t('May'),
				'06': $.t('Jun'),
				'07': $.t('Jul'),
				'08': $.t('Aug'),
				'09': $.t('Sep'),
				'10': $.t('Oct'),
				'11': $.t('Nov'),
				'12': $.t('Dec'),
			};
			return months[month];
		}
	}
			
	function compareSeriesSuppliers(ctrl) {
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
								color: yearColor(year),
								index: year - data.firstYear + 1
							},
							postprocessXaxis: function (xAxis) {
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

    // input: h in [0,360] and s,v in [0,1] - output: r,g,b in [0,1]
    function hsvToRgb(hsv) {
        let f= (n,k=(n+hsv['h']/60)%6) => hsv['v'] - hsv['v']*hsv['s']*Math.max( Math.min(k,4-k,1), 0);
        return {'r':f(5),'g':f(3),'b':f(1)};
    }

    function hsv(h, s, v) {
        return {'h':h, 's':s+.02, 'v':v+.02};
    }

    function rgbToString(rgb) {
        return 'rgba(' + Math.round(rgb['r']*255) + ',' + Math.round(rgb['g']*255) + ',' + Math.round(rgb['b']*255) + ',.8)';
    }

    function yearColor(year) {
        return yearColors[year.toString()];
    }

});