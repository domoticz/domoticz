import {Component, ElementRef, Inject, Input, OnChanges, OnInit, SimpleChanges, ViewChild} from '@angular/core';
import * as Highcharts from 'highcharts';
import {DateTime} from 'luxon';
import {dzSettings} from 'src/app/log/device-log/settings';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import * as dateFormat from 'dateformat';
import {TooltipFormatterContextObject} from 'highcharts';
import {ChartService} from '../../../_shared/_services/chart.service';

@Component({
  selector: 'dz-device-on-off-chart',
  templateUrl: './device-on-off-chart.component.html',
  styleUrls: ['./device-on-off-chart.component.css']
})
export class DeviceOnOffChartComponent implements OnInit, OnChanges {

  @Input() log: any; // FIXME type

  @Input() view: string;

  @Input() deviceIdx: string;

  @ViewChild('chart', {static: false}) private chartRef: ElementRef;

  constructor(@Inject(I18NEXT_SERVICE) private translationService: ITranslationService) {
  }

  ngOnInit() {
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes.log && changes.log.currentValue) {
      this.renderChart(changes.log.currentValue);
    }
  }

  private getMinValue(): number | undefined {
    if (this.view === 'daily') {
      return DateTime.local().startOf('day').valueOf();
    } else if (this.view === 'weekly') {
      return DateTime.local().minus({days: 6}).startOf('day').valueOf();
    } else {
      return undefined;
    }
  }

  private getFilteredData(data: any) {
    const chartData = [];
    const min = this.getMinValue();

    let firstIndex = min === undefined
      ? data.findIndex(function (point) {
        return DateTime.fromFormat(point.Date, dzSettings.serverDateFormat).valueOf() >= min;
      }) : 0;

    // Add one point out of the range to properly render initial value
    if (firstIndex > 0) {
      firstIndex -= 1;
    }

    return firstIndex === -1
      ? []
      : data.slice(firstIndex);
  }

  private renderChart(data: any) {
    const chartData = [];

    this.getFilteredData(data).forEach((point, index, points) => {
      if (point.Status === 'On'
        || (point.Status.includes('Set Level') && point.Level > 0)
        || (point.Status.includes('Set Color'))
      ) {
        chartData.push({
          x: DateTime.fromFormat(point.Date, dzSettings.serverDateFormat).valueOf(),
          x2: points[index + 1] ? DateTime.fromFormat(points[index + 1].Date, dzSettings.serverDateFormat).valueOf() : Date.now(),
          y: 0,
          d: point.Data
        });
      }
    });

    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        zoomType: 'x',
      },
      title: {
        text: null
      },
      legend: {
        enabled: false
      },
      xAxis: {
        type: 'datetime',
        min: this.getMinValue(),
        max: DateTime.local().endOf('day').valueOf(),
        startOnTick: true,
        endOnTick: true
      },
      yAxis: {
        title: {
          text: ''
        },
        labels: {
          enabled: false
        },
        categories: ['On'],
        reversed: true
      },
      time: {
        useUTC: false,
      },
      tooltip: {
        formatter: function (this: TooltipFormatterContextObject) {
          let rStr = '';
          rStr += '<span style="font-size: 10px">';
          rStr += dateFormat(this.x, 'dddd, mmm dd yyyy HH:MM:ss');
          rStr += ' - ';
          rStr += dateFormat(this['x2'], 'dddd, mmm dd yyyy HH:MM:ss');
          rStr += '</span>';
          rStr += '<br/>';
          rStr += '<span style="color:' + this.point.color + '">\u25CF</span> ';
          rStr += _this.translationService.t(this.series.name) + ': ' + this.point.d;
          return rStr;
        }
      },
      series: [{
        type: 'xrange',
        name: 'Device Status',
        data: chartData,
        borderRadius: 0,
        borderWidth: 0,
        dataLabels: {
          enabled: false
        },
        minPointLength: 2,
        turboThreshold: 0
      }]
    };

    ChartService.setDeviceIdx(this.deviceIdx);
    Highcharts.chart(this.chartRef.nativeElement, chartOptions);
  }

}
