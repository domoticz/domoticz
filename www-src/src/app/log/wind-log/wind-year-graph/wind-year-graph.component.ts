import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {ConfigService} from 'src/app/_shared/_services/config.service';
import * as Highcharts from 'highcharts';
import {GraphTempPoint} from '../../../_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-wind-year-graph',
  templateUrl: './wind-year-graph.component.html',
  styleUrls: ['./wind-year-graph.component.css']
})
export class WindYearGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;
  @Input() lscales: Array<{ from: number, to: number }> = [];

  @ViewChild('graph', {static: false}) chartTarget: ElementRef;

  private chart: Highcharts.Chart;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    this.loadGraph();
  }

  private loadGraph() {
    const _this = this;

    const options: Highcharts.Options = {
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
          load: () => {
            _this.graphService.getData('year', _this.idx, 'wind').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const seriessp = _this.chart.series[0];
                const seriesgu = _this.chart.series[1];
                const series_prev_sp = _this.chart.series[2];
                const series_prev_gu = _this.chart.series[3];
                const datatablesp = [];
                const datatablegu = [];
                const datatable_prev_sp = [];
                const datatable_prev_gu = [];

                data.result.forEach((item: GraphTempPoint) => {
                  datatablesp.push([Utils.GetDateFromString(item.d), Number(item.sp)]);
                  datatablegu.push([Utils.GetDateFromString(item.d), Number(item.gu)]);
                });
                const bHavePrev = (typeof data.resultprev !== 'undefined');
                if (bHavePrev) {
                  data.resultprev.forEach((item: GraphTempPoint) => {
                    datatable_prev_sp.push([Utils.GetPrevDateFromString(item.d), Number(item.sp)]);
                    datatable_prev_gu.push([Utils.GetPrevDateFromString(item.d), Number(item.gu)]);
                  });
                }
                seriessp.setData(datatablesp, false);
                seriesgu.setData(datatablegu, false);
                series_prev_sp.setData(datatable_prev_sp, false);
                series_prev_gu.setData(datatable_prev_gu, false);
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Wind') + ' ' + this.translationService.t('speed/gust') + ' ' + this.translationService.t('Last Year')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: this.translationService.t('Speed') + ' (' + this.configService.config.WindSign + ')'
        },
        min: 0,
        minorGridLineWidth: 0,
        gridLineWidth: 0,
        alternateGridColor: null,
        plotBands: [{ // Light air
          from: this.lscales[0].from,
          to: this.lscales[0].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Light air'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Light breeze
          from: this.lscales[1].from,
          to: this.lscales[1].to,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Light breeze'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Gentle breeze
          from: this.lscales[2].from,
          to: this.lscales[2].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Gentle breeze'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Moderate breeze
          from: this.lscales[3].from,
          to: this.lscales[3].to,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Moderate breeze'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Fresh breeze
          from: this.lscales[4].from,
          to: this.lscales[4].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Fresh breeze'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Strong breeze
          from: this.lscales[5].from,
          to: this.lscales[5].to,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Strong breeze'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // High wind
          from: this.lscales[6].from,
          to: this.lscales[6].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('High wind'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // fresh gale
          from: this.lscales[7].from,
          to: this.lscales[7].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Fresh gale'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // strong gale
          from: this.lscales[8].from,
          to: this.lscales[8].to,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Strong gale'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // storm
          from: this.lscales[9].from,
          to: this.lscales[9].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Storm'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // Violent storm
          from: this.lscales[10].from,
          to: this.lscales[10].to,
          color: 'rgba(68, 170, 213, 0.2)',
          label: {
            text: this.translationService.t('Violent storm'),
            style: {
              color: '#CCCCCC'
            }
          }
        }, { // hurricane
          from: this.lscales[11].from,
          to: this.lscales[11].to,
          color: 'rgba(68, 170, 213, 0.1)',
          label: {
            text: this.translationService.t('Hurricane'),
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
      series: [{
        id: 'speed',
        type: 'spline',
        name: this.translationService.t('Speed'),
        tooltip: {
          valueSuffix: ' ' + this.configService.config.WindSign,
          valueDecimals: 1
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
      }, {
        id: 'gust',
        type: 'spline',
        name: this.translationService.t('Gust'),
        tooltip: {
          valueSuffix: ' ' + this.configService.config.WindSign,
          valueDecimals: 1
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
      }, {
        id: 'past_speed',
        type: 'spline',
        name: this.translationService.t('Past') + ' ' + this.translationService.t('Speed'),
        visible: false,
        tooltip: {
          valueSuffix: ' ' + this.configService.config.WindSign,
          valueDecimals: 1
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
      }, {
        id: 'past_gust',
        type: 'spline',
        name: this.translationService.t('Past') + ' ' + this.translationService.t('Gust'),
        visible: false,
        color: 'rgba(234,234,240,0.8)',
        tooltip: {
          valueSuffix: ' ' + this.configService.config.WindSign,
          valueDecimals: 1
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
      }],
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
