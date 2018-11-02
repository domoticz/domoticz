import {
  Component,
  OnInit,
  Input,
  OnChanges,
  AfterViewInit,
  ElementRef,
  ViewChild,
  Inject,
  SimpleChanges
} from '@angular/core';
import {ITranslationService, I18NEXT_SERVICE} from 'angular-i18next';
import {DeviceUtils} from '../../../_shared/_utils/device-utils';
import * as Highcharts from 'highcharts';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {Utils} from 'src/app/_shared/_utils/utils';
import {GraphTempPoint} from '../../../_shared/_models/graphdata';
import {Device} from "../../../_shared/_models/device";
import {ChartService} from "../../../_shared/_services/chart.service";


@Component({
  selector: 'dz-device-log-chart',
  templateUrl: './device-log-chart.component.html',
  styleUrls: ['./device-log-chart.component.css']
})
export class DeviceLogChartComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() device: Device;
  @Input() deviceType?: string;
  @Input() degreeType?: string;
  @Input() range: string;

  @ViewChild('graph', {static: false}) graphElt: ElementRef;

  chart: Highcharts.Chart;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService
  ) {
  }

  ngOnInit() {
  }

  ngAfterViewInit() {
    const _this = this;

    const sensor = this.getSensorName();
    const unit = this.getChartUnit();
    const axisTitle = this.device.SubType === 'Custom Sensor' ? unit : sensor + ' (' + unit + ')';

    ChartService.setDeviceIdx(this.device.idx);
    this.chart = Highcharts.chart(this.graphElt.nativeElement, {
      chart: {
        type: 'spline',
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
      yAxis: {
        title: {
          text: axisTitle
        },
        labels: {
          formatter: function () {
            const value = unit === 'vM'
              ? Highcharts.numberFormat(this.value, 0)
              : this.value;

            return value + ' ' + unit;
          }
        }
      },
      tooltip: {
        crosshairs: true,
        shared: true,
        valueSuffix: ' ' + unit
      },
      plotOptions: {
        series: {
          point: {
            events: {
              click: (event: ClickPointEvent): boolean => {
                if (event.shiftKey !== true) {
                  return;
                }

                _this.graphService.deletePoint(_this.device.idx, event.point, _this.range === 'day').subscribe(() => {
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
        }
      },
      title: {
        text: this.getChartTitle()
      },
      series: [],
      legend: {
        enabled: true
      }
    });

    this.refreshChartData();
  }

  ngOnChanges(changesObj: SimpleChanges) {
    if (!this.chart) {
      return;
    }

    if (changesObj.deviceIdx || changesObj.range) {
      this.refreshChartData();
    }

    if (changesObj.deviceType) {
      this.chart.setTitle({text: this.getChartTitle()}, {}, true);
    }
  }

  private refreshChartData() {
    this.graphService.getData(this.range, this.device.idx, this.getChartType()).subscribe(data => {
      if (typeof data.result === 'undefined') {
        return;
      }

      const valueKey = this.getPointValueKey();

      if (this.range === 'day') {
        const sensor = this.getSensorName();

        this.chart.addSeries(<Highcharts.SeriesSplineOptions>{
          showInLegend: false,
          name: sensor,
          data: data.result.map((item: GraphTempPoint) => {
            return <[number, number]>[Utils.GetUTCFromString(item.d), Number(item[valueKey])];
          })
        }, false);
      }

      if (this.range === 'month' || this.range === 'year') {
        const series = [
          {name: 'min', data: []},
          {name: 'max', data: []},
          {name: 'avg', data: []}
        ];

        data.result.forEach((item: GraphTempPoint) => {
          series[0].data.push([Utils.GetDateFromString(item.d), parseFloat(item[valueKey + '_min'])]);
          series[1].data.push([Utils.GetDateFromString(item.d), parseFloat(item[valueKey + '_max'])]);

          if (item[valueKey + '_avg'] !== undefined) {
            series[2].data.push([Utils.GetDateFromString(item.d), parseFloat(item[valueKey + '_avg'])]);
          }
        });

        series
          .filter((item) => {
            return item.data.length > 0;
          })
          .forEach((item) => {
            this.chart.addSeries(<Highcharts.SeriesSplineOptions>item, false);
          });
      }

      this.chart.redraw();
    });
  }

  private getPointValueKey(): string {
    if (this.device.SubType === 'Lux') {
      return 'lux';
    } else if (this.device.Type === 'Usage') {
      return 'u';
    } else {
      return 'v';
    }
  }

  private getChartTitle(): string {
    if (this.range === 'day') {
      return this.graphService.Get5MinuteHistoryDaysGraphTitle();
    } else if (this.range === 'month') {
      return this.translationService.t('Last Month');
    } else if (this.range === 'year') {
      return this.translationService.t('Last Year');
    }
  }

  private getChartUnit(): string {
    return DeviceUtils.getUnit(this.device);
  }

  private getChartType(): string {
    if (['Custom Sensor', 'Waterflow', 'Percentage'].includes(this.device.SubType)) {
      return 'Percentage';
    } else {
      return 'counter';
    }
  }

  private getSensorName(): string {
    if (this.device.Type === 'Usage') {
      return this.translationService.t('Usage');
    } else {
      return this.translationService.t(this.device.SubType);
    }
  }

}
