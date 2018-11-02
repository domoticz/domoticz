import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import {Options, Chart} from 'highcharts';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-counter-spline-day-graph',
  templateUrl: './counter-spline-day-graph.component.html',
  styleUrls: ['./counter-spline-day-graph.component.css']
})
export class CounterSplineDayGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;
  @Input() switchtype: number;

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

    let graph_title = (this.switchtype === 4) ? this.translationService.t('Generated') : this.translationService.t('Usage');
    graph_title += ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle();

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
        alignTicks: false, // necessary for y-axis extremes matching
        events: {
          load: () => {
            _this.graphService.getDataWithMethod('day', _this.idx, 'counter', 1).subscribe(data => {
              if (typeof data.result !== 'undefined') {
                _this.graphService.AddDataToUtilityChart(data, _this.chart, _this.switchtype, 'day');
                _this.chart.redraw();
              }
            });
          },
          redraw: function () {
            const me: Chart & { redrawCnt?: number } = this;
            let iMin = 0, iMax = 0, bRedraw = false;
            me.redrawCnt = me.redrawCnt || 0; // failsafe
            me.redrawCnt++;
            me.series.forEach((oSerie_) => {
              const iAxisMin = me.yAxis[oSerie_.options.yAxis].min;
              if (iAxisMin < iMin) {
                bRedraw = bRedraw || (0 !== iMin);
                iMin = iAxisMin;
              }
              const iAxisMax = me.yAxis[oSerie_.options.yAxis].max;
              if (iAxisMax > iMax) {
                bRedraw = bRedraw || (0 !== iMax);
                iMax = iAxisMax;
              }
            });
            if (bRedraw && me.redrawCnt === 1) {
              _this.chart.yAxis.forEach((oAxis_) => {
                oAxis_.setExtremes((iMin !== 0) ? iMin : null, (iMax !== 0) ? iMax : null, false);
              });
              me.redraw();
            } else {
              _this.chart.yAxis.forEach((oAxis_) => {
                oAxis_.setExtremes(null, null, false); // next time rescale yaxis if necessary
              });
              me.redrawCnt = 0;
            }
          }
        }
      },
      title: {
        text: graph_title
      },
      xAxis: {
        type: 'datetime',
      },
      yAxis: [{
        title: {
          text: this.translationService.t('Energy') + ' (Wh)'
        }
      }, {
        title: {
          text: this.translationService.t('Power') + ' (Watt)'
        },
        opposite: true
      }],
      tooltip: {
        crosshairs: true,
        shared: false
      },
      plotOptions: {
        series: {
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
          // matchExtremes: true
        },
        spline: {
          lineWidth: 3,
          states: {
            hover: {
              lineWidth: 3
            }
          },
          marker: {
            symbol: 'circle',
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            }
          }
        },
        areaspline: {
          lineWidth: 3,
          marker: {
            enabled: false
          },
          states: {
            hover: {
              lineWidth: 3
            }
          }
        },
        column: {
          minPointLength: 4,
          pointPadding: 0.1,
          groupPadding: 0,
          dataLabels: {
            enabled: false,
            color: 'white'
          }
        }
      },
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }


}
