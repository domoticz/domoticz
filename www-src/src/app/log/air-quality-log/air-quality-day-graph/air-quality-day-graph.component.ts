import {Component, OnInit, AfterViewInit, Input, ViewChild, ElementRef, Inject} from '@angular/core';
import * as Highcharts from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {GraphTempPoint} from 'src/app/_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {Options} from 'highcharts';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-air-quality-day-graph',
  templateUrl: './air-quality-day-graph.component.html',
  styleUrls: ['./air-quality-day-graph.component.css']
})
export class AirQualityDayGraphComponent implements OnInit, AfterViewInit {

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
            _this.graphService.getData('day', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const series = _this.chart.series[0];
                const datatable = [];

                data.result.forEach((item: GraphTempPoint) => {
                  datatable.push([Utils.GetUTCFromString(item.d), Number(item.co2)]);
                });
                series.setData(datatable); // redraws
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Air Quality') + ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle()
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
          id: 'co2',
          type: 'spline',
          name: 'co2',
          showInLegend: false,
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

                _this.graphService.deletePoint(_this.idx, event.point, true).subscribe(() => {
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
