import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {AxisLabelsFormatterContextObject, Options} from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from '../../../_shared/_services/graph.service';
import {GraphTempPoint} from '../../../_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-barometer-day-graph',
  templateUrl: './barometer-day-graph.component.html',
  styleUrls: ['./barometer-day-graph.component.css']
})
export class BarometerDayGraphComponent implements OnInit, AfterViewInit {

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
          load: () => {
            _this.graphService.getData('day', _this.idx, 'temp').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const series = _this.chart.series[0];
                const datatable = [];
                data.result.forEach((item: GraphTempPoint) => {
                  datatable.push([Utils.GetUTCFromString(item.d), Number(item.ba)]);
                });
                series.setData(datatable); // redraws
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Barometer') + ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle()
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: this.translationService.t('Pressure') + ' (hPa)'
        },
        labels: {
          formatter: function (this: AxisLabelsFormatterContextObject) {
            return this.value.toString();
          }
        }
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
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            },
            symbol: 'circle'
          }
        }
      },
      series: [{
        id: 'baro',
        type: 'spline',
        name: 'Baro',
        tooltip: {
          valueSuffix: ' hPa',
          valueDecimals: 1
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
      }]
      ,
      legend: {
        enabled: true
      },
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
