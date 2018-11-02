import {Component, ElementRef, Inject, Input, OnChanges, OnInit, SimpleChanges, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {dzSettings} from 'src/app/log/device-log/settings';
import {DateTime} from 'luxon';
import {ChartService} from '../../../_shared/_services/chart.service';

@Component({
  selector: 'dz-device-level-chart',
  templateUrl: './device-level-chart.component.html',
  styleUrls: ['./device-level-chart.component.css']
})
export class DeviceLevelChartComponent implements OnInit, OnChanges {

  @Input() log: any; // FIXME type
  @Input() deviceIdx: string;

  @ViewChild('chart', {static: false}) private chartRef: ElementRef;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService
  ) {
  }

  ngOnInit() {
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.log && changes.log.currentValue) {
      this.renderChart(changes.log.currentValue);
    }
  }

  private renderChart(data: any) {
    const chartData = [];

    data.forEach((item) => {
      let level = -1;

      if (['Off', 'Disarm', 'No Motion', 'Normal'].includes(item.Status)) {
        level = 0;
      } else if (data.HaveSelector === true) {
        level = Number(item.Level);
      } else if (item.Status.indexOf('Set Level:') === 0) {
        let lstr = item.Status.substr(11);
        const idx = lstr.indexOf('%');

        if (idx !== -1) {
          lstr = lstr.substr(0, idx - 1);
          level = Number(lstr);
        }
      } else {
        let idx = item.Status.indexOf('Level: ');

        if (idx !== -1) {
          let lstr = item.Status.substr(idx + 7);
          idx = lstr.indexOf('%');
          if (idx !== -1) {
            lstr = lstr.substr(0, idx - 1);
            level = Number(lstr);
            if (level > 100) {
              level = 100;
            }
          }
        } else if (item.Level > 0 && item.Level <= 100) {
          level = item.Level;
        } else {
          level = 100;
        }
      }

      if (level !== -1) {
        chartData.push([
          DateTime.fromFormat(item.Date, dzSettings.serverDateFormat).valueOf(),
          level
        ]);
      }
    });

    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        type: 'line',
        zoomType: 'x',
        resetZoomButton: {
          position: {
            x: -30,
            y: 10
          }
        },
        marginRight: 10
      },
      title: {
        text: null
      },
      legend: {
        enabled: false
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        min: 0,
        max: 100,
        title: {
          text: this.translationService.t('Percentage') + ' (%)'
        }
      },
      time: {
        useUTC: false,
      },
      tooltip: {
        valueSuffix: '%'
      },
      plotOptions: {
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
        }
      },
      series: [<Highcharts.SeriesLineOptions>{
        name: 'Level',
        step: 'left',
        data: chartData
      }],
      navigation: {
        menuItemStyle: {
          fontSize: '10px'
        }
      }
    };

    ChartService.setDeviceIdx(this.deviceIdx);
    Highcharts.chart(this.chartRef.nativeElement, chartOptions);
  }

}
