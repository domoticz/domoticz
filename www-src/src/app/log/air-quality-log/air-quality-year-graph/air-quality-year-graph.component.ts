import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {Options} from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {GraphTempPoint} from 'src/app/_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-air-quality-year-graph',
  templateUrl: './air-quality-year-graph.component.html',
  styleUrls: ['./air-quality-year-graph.component.css']
})
export class AirQualityYearGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;

  @ViewChild('graph', {static: false}) chartTarget: ElementRef;

  private chart: Highcharts.Chart;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.loadGraph();
  }

  private loadGraph() {
    const _this = this;

    const options: Options = {
      chart: {
        type: 'spline',
        zoomType: 'x',
        resetZoomButton: {
          position: {
            x: -30,
            y: -36
          }
        },
        marginRight: 10,
        events: {
          load: function () {
            _this.graphService.getData('year', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const datatable1 = [];
                const datatable2 = [];
                const datatable3 = [];
                const datatable_prev = [];

                data.result.forEach((item: GraphTempPoint) => {
                  datatable1.push([Utils.GetDateFromString(item.d), Number(item.co2_min)]);
                  datatable2.push([Utils.GetDateFromString(item.d), Number(item.co2_max)]);
                  const avg_val = Number(item.co2_avg);
                  if (avg_val !== 0) {
                    datatable3.push([Utils.GetDateFromString(item.d), avg_val]);
                  }
                });

                data.resultprev.forEach((item: GraphTempPoint) => {
                  const cdate = Utils.GetPrevDateFromString(item.d);
                  datatable_prev.push([cdate, Number(item.co2_max)]);
                });

                const series1 = _this.chart.series[0];
                const series2 = _this.chart.series[1];
                series1.setData(datatable1, false);
                series2.setData(datatable2, false);
                if (datatable3.length > 0) {
                  const series3 = _this.chart.series[2];
                  series3.setData(datatable3);
                } else {
                  _this.chart.series[2].remove();
                }
                if (datatable_prev.length !== 0) {
                  _this.chart.addSeries(<Highcharts.SeriesSplineOptions>{
                    id: 'prev_co2',
                    name: _this.translationService.t('Past') + ' co2',
                    color: 'rgba(223,212,246,0.8)',
                    zIndex: 3,
                    yAxis: 0,
                    tooltip: {
                      valueSuffix: ' ppm',
                      valueDecimals: 0
                    },
                    visible: false
                  }, false);
                  const series = <Highcharts.Series>_this.chart.get('prev_co2');
                  series.setData(datatable_prev, false);
                }
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Air Quality') + ' ' + this.translationService.t('Last Year')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: 'co2 (ppm)'
        },
        minorGridLineWidth: 0,
        gridLineWidth: 0,
        alternateGridColor: null,
        plotBands: [{ // Excellent
          from: 0,
          to: 700,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Excellent'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Good
          from: 700,
          to: 900,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Good'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Fair
          from: 900,
          to: 1100,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Fair'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Mediocre
          from: 1100,
          to: 1600,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Mediocre'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Bad
          from: 1600,
          to: 6000,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Bad'),
            style: {
              color: '#CCCCCC'
            }
          }
        }
        ]
      },
      tooltip: {
        crosshairs: true,
        shared: true
      },
      plotOptions: {
        spline: {
          lineWidth: 3,
          states: {
            hover: {
              lineWidth: 3
            }
          },
          marker: {
            enabled: false,
            symbol: 'circle',
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            }
          }
        }
      },
      series: [
        {
          id: 'co2_min',
          type: 'spline',
          name: 'min',
          tooltip: {
            valueSuffix: ' ppm',
            valueDecimals: 0
          },
          point: {
            events: {
              click: (event: ClickPointEvent): boolean => {
                if (event.shiftKey !== true) {
                  return;
                }

                _this.graphService.deletePoint(_this.idx, event.point, false).subscribe(() => {
                  _this.loadGraph();
                });
              }
            }
          }
        },
        {
          id: 'co2_max',
          type: 'spline',
          name: 'max',
          tooltip: {
            valueSuffix: ' ppm',
            valueDecimals: 0
          },
          point: {
            events: {
              click: (event: ClickPointEvent): boolean => {
                if (event.shiftKey !== true) {
                  return;
                }

                _this.graphService.deletePoint(_this.idx, event.point, false).subscribe(() => {
                  _this.loadGraph();
                });
              }
            }
          }
        },
        {
          id: 'co2_avg',
          type: 'spline',
          name: 'avg',
          tooltip: {
            valueSuffix: ' ppm',
            valueDecimals: 0
          },
          point: {
            events: {
              click: (event: ClickPointEvent): boolean => {
                if (event.shiftKey !== true) {
                  return;
                }

                _this.graphService.deletePoint(_this.idx, event.point, false).subscribe(() => {
                  _this.loadGraph();
                });
              }
            }
          }
        }
      ],
      navigation: {
        menuItemStyle: {
          fontSize: '10px'
        }
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
