import {
  AfterViewInit,
  Component,
  ElementRef,
  Inject,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
  ViewChild
} from '@angular/core';
import * as Highcharts from 'highcharts';
import {ConfigService} from '../../../_shared/_services/config.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from '../../../_shared/_services/graph.service';
import {ChartService} from '../../../_shared/_services/chart.service';

@Component({
  selector: 'dz-temperature-log-chart',
  templateUrl: './temperature-log-chart.component.html',
  styleUrls: ['./temperature-log-chart.component.css']
})
export class TemperatureLogChartComponent implements OnInit, OnChanges, AfterViewInit {

  @Input()
  deviceIdx: string;
  @Input()
  deviceType: string;
  @Input()
  range: string;

  @ViewChild('chartTarget', {static: false})
  chartTarget: ElementRef;

  chart: Highcharts.Chart;

  constructor(private configService: ConfigService,
              @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
              private graphService: GraphService) {
  }

  ngOnInit() {
  }

  ngAfterViewInit(): void {
    const degreeType = this.configService.config.TempSign;

    const _this = this;

    const options: Highcharts.Options = {
      chart: {
        type: this.getChartType(),
        zoomType: 'x',
        resetZoomButton: {
          position: {
            x: -30,
            y: -36
          }
        }
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: [
        { // temp label
          labels: {
            formatter: function () {
              return this.value + '\u00B0 ' + degreeType;
            }
          },
          title: {
            text: this.translationService.t('Degrees') + ' \u00B0' + degreeType
          }
        },
        { // humidity label
          showEmpty: false,
          allowDecimals: false,	// no need to show scale with decimals
          ceiling: 100, // max limit for auto zoom, bug in highcharts makes this sometimes not considered.
          floor: 0, // min limit for auto zoom
          minRange: 10, // min range for auto zoom
          labels: {
            formatter: function () {
              return this.value + '%';
            }
          },
          title: {
            text: this.translationService.t('Humidity') + ' %'
          },
          opposite: true
        }
      ],
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

                _this.graphService.deletePoint(_this.deviceIdx, event.point, _this.range === 'day').subscribe(result => {
                  // Clear data
                  const seriesLength = _this.chart.series.length;
                  for (let i = seriesLength - 1; i > -1; i--) {
                    _this.chart.series[i].remove();
                  }

                  _this.refreshChartData();
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
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            },
            symbol: 'circle'
          }
        },
        line: {
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
        },
        areasplinerange: {
          marker: {
            enabled: false
          }
        }
      },
      title: {
        text: this.getChartTitle()
      },
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.deviceIdx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);

    this.refreshChartData();
  }

  ngOnChanges(changes: SimpleChanges) {
    if (!this.chart) {
      return;
    }

    if (changes.deviceIdx || changes.range) {
      this.refreshChartData();
    }

    if (changes.deviceType) {
      this.chart.setTitle({text: this.getChartTitle()}, {}, true);
    }
  }

  private refreshChartData() {
    this.graphService.getData(this.range, this.deviceIdx, 'temp').subscribe(graphData => {
      if (graphData.result) {
        this.graphService.AddDataToTempChart(graphData, this.chart, this.range === 'day' ? 1 : 0, (this.deviceType === 'Thermostat'));
        this.chart.redraw();
      }
      // this.chart.yAxis[1].visibility = this.range !== 'day';
      this.chart.yAxis[1].update({visible: this.range !== 'day'});
    });
  }

  private getChartType(): 'line' | 'spline' {
    if (this.deviceType === 'Thermostat') {
      return 'line';
    } else {
      return 'spline';
    }
  }

  private getChartTitle(): string {
    const titlePrefix = this.deviceType === 'Humidity' ? this.translationService.t('Humidity') : this.translationService.t('Temperature');

    if (this.range === 'day') {
      return titlePrefix + ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle();
    } else if (this.range === 'month') {
      return titlePrefix + ' ' + this.translationService.t('Last Month');
    } else if (this.range === 'year') {
      return titlePrefix + ' ' + this.translationService.t('Last Year');
    }
  }

}

// Just here to have a bit of type safety because not currently exposed in @types/highcharts
export interface ClickPointEvent extends MouseEvent, Highcharts.SeriesClickEventObject {
}
