import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {GraphTempPoint} from 'src/app/_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-fan-day-graph',
  templateUrl: './fan-day-graph.component.html',
  styleUrls: ['./fan-day-graph.component.css']
})
export class FanDayGraphComponent implements OnInit, AfterViewInit {

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
          load: function () {
            _this.graphService.getData('day', _this.idx, 'fan').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const series = _this.chart.series[0];
                const datatable = [];

                data.result.forEach((item: GraphTempPoint) => {
                  datatable.push([Utils.GetUTCFromString(item.d), Number(item.v)]);
                });
                series.setData(datatable); // redraws
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('RPM') + ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle()
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: 'RPM'
        },
        allowDecimals: false,
        minorGridLineWidth: 0,
        alternateGridColor: null
      },
      tooltip: {
        crosshairs: true,
        shared: true
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
        },
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
        id: 'fan',
        type: 'spline',
        // name: sensor,
        tooltip: {
          valueSuffix: ' rpm',
          valueDecimals: 0
        },
      }]
      ,
      navigation: {
        menuItemStyle: {
          fontSize: '10px'
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
