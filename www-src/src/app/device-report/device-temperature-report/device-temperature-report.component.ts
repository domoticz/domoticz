import {
  AfterViewInit,
  ChangeDetectorRef,
  Component,
  ElementRef,
  Inject,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
  ViewChild
} from '@angular/core';
import {ConfigService} from '../../_shared/_services/config.service';
import {DeviceReportData, DeviceTemperatureReportDataService} from '../device-temperature-report-data.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import * as dateFormat from 'dateformat';
import * as Highcharts from 'highcharts';
import {DatatableHelper} from '../../_shared/_utils/datatable-helper';
import {Router} from '@angular/router';
import {Device} from '../../_shared/_models/device';
import {monthNames} from '../../_shared/_constants/months';
import {ChartService} from '../../_shared/_services/chart.service';

// FIXME use proper ts def
declare var $: any;

// FIXME clean this component
@Component({
  selector: 'dz-device-temperature-report',
  templateUrl: './device-temperature-report.component.html',
  styleUrls: ['./device-temperature-report.component.css']
})
export class DeviceTemperatureReportComponent implements OnInit, OnChanges, AfterViewInit {

  @Input() device: Device;
  @Input() selectedYear: number;
  @Input() selectedMonth?: number;

  @ViewChild('reporttable', {static: false}) table: ElementRef;
  @ViewChild('usagegraph', {static: false}) usagegraph: ElementRef;
  @ViewChild('variationgraph', {static: false}) variationgraph: ElementRef;

  max: MinMax;
  min: MinMax;

  degreeType: any;
  degreeDays: any;

  isMonthView: boolean;
  noDataAvailable: boolean;
  isVariationChartVisible: boolean;

  constructor(
    private configService: ConfigService,
    private deviceTemperatureReportDataService: DeviceTemperatureReportDataService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private changeDetectorRef: ChangeDetectorRef,
    private router: Router
  ) {
  }

  ngOnInit() {
    this.isMonthView = this.selectedMonth && this.selectedMonth > 0;
    this.degreeType = this.configService.config.TempSign;
  }

  ngOnChanges(changes: SimpleChanges) {
    if ((changes.selectedYear && !changes.selectedYear.isFirstChange()) ||
      (changes.selectedMonth && !changes.selectedMonth.isFirstChange())) {
      this.getData();
    }
  }

  ngAfterViewInit() {
    this.getData();
  }

  private getData() {
    this.deviceTemperatureReportDataService.fetch(this.device.idx, this.selectedYear, this.selectedMonth).subscribe(data => {
      if (!data) {
        this.noDataAvailable = true;
        return;
      } else {
        this.noDataAvailable = false;
      }

      this.isVariationChartVisible = !(data.isOnlyHumidity && this.isMonthView);

      // Explicit changes detection to avoid weird Highcharts sizing
      // FIXME should be fixed if we use proper lib or dedicated component
      this.changeDetectorRef.detectChanges();

      this.showTable(data);
      this.showAverageChart(data);

      if (this.isVariationChartVisible) {
        this.showVariationChart(data);
      } else {
        this.variationgraph.nativeElement.remove();
      }

      if (!data.isOnlyHumidity) {
        this.min = {
          date: dateFormat(data.minDate, 'd') + ' ' + this.translationService.t(dateFormat(data.minDate, 'mmmm')),
          value: data.min
        };

        this.max = {
          date: dateFormat(data.maxDate, 'd') + ' ' + this.translationService.t(dateFormat(data.maxDate, 'mmmm')),
          value: data.max
        };

        if (data.degreeDays) {
          this.degreeDays = data.degreeDays.toFixed(1);
        }
      }
    });
  }

  private showTable(data: DeviceReportData) {
    const tableElt = this.table.nativeElement;

    // Remove previous table (in case of input changes)
    if ($.fn.DataTable.isDataTable(tableElt)) {
      $(tableElt).DataTable().clear();
      $(tableElt).DataTable().destroy();
    }

    const columns = [];

    const humidityRenderer = (d: number) => {
      if (typeof data !== 'undefined') {
        return d.toFixed(0);
      }
      return 0;
    };

    const baroRenderer = (d: number) => {
      if (typeof data !== 'undefined') {
        return d.toFixed(1);
      }
      return 0;
    };

    const temperatureRenderer = (d: number) => {
      if (typeof data !== 'undefined') {
        return d.toFixed(1);
      }
      return 0;
    };

    if (this.isMonthView) {
      columns.push({
        title: this.translationService.t('Day'),
        data: 'date',
        render: (day: string) => {
          return this.translationService.t(dateFormat(day, 'd'));
        }
      });

      columns.push({
        title: '',
        data: 'date',
        render: (day: string) => {
          return this.translationService.t(dateFormat(day, 'dddd'));
        }
      });
    } else {
      columns.push({
        title: this.translationService.t('Month'),
        data: 'date',
        render: (month: number) => {
          const link = '<a class="ng-link" href="/Devices/' + this.device.idx + '/Report/' + this.selectedYear +
            '/' + month + '"><img src="images/next.png" /></a>';
          return month.toString().padStart(2, '0') + '. ' + this.translationService.t(monthNames[month - 1]) + ' ' + link;
        }
      });
    }

    if (data.isHumidityAvailable && !data.isOnlyHumidity) {
      columns.push({title: this.translationService.t('Humidity'), data: 'humidity', render: humidityRenderer});
    }
    if (data.isBaroAvailable) {
      columns.push({title: this.translationService.t('Avg. Baro (hPa)'), data: 'baro', render: baroRenderer});
    }

    if (data.isOnlyHumidity && this.isMonthView) {
      columns.push({title: this.translationService.t('Humidity (%)'), data: 'avg', render: humidityRenderer});
    } else if (data.isOnlyHumidity) {
      columns.push({title: this.translationService.t('Avg. Hum (%)'), data: 'avg', render: humidityRenderer});
      columns.push({title: this.translationService.t('Min. Hum (%)'), data: 'min', render: humidityRenderer});
      columns.push({title: this.translationService.t('Max. Hum (%)'), data: 'max', render: humidityRenderer});
    } else {
      columns.push({
        title: this.translationService.t('Avg. Temp (\u00B0' + this.degreeType + ')'),
        data: 'avg',
        render: temperatureRenderer
      });
      columns.push({
        title: this.translationService.t('Min. Temp (\u00B0' + this.degreeType + ')'),
        data: 'min',
        render: temperatureRenderer
      });
      columns.push({
        title: this.translationService.t('Max. Temp (\u00B0' + this.degreeType + ')'),
        data: 'max',
        render: temperatureRenderer
      });
    }

    columns.push({
      title: '<>',
      orderable: false,
      data: 'trend',
      render: (trend: string) => {
        return '<img src="images/' + trend + '.png">';
      }
    });

    $(tableElt).dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      sDom: '<"H"rC>t<"F">',
      columns: columns,
      pageLength: 50,
      order: [[0, 'asc']]
    }));

    $(tableElt).dataTable().api().rows
      .add(data.items)
      .draw();

    // Fix links so that it uses Angular router
    DatatableHelper.fixAngularLinks('.ng-link', this.router);
  }

  private showAverageChart(data: DeviceReportData): void {
    const chartData = data.items.map((item) => {
      let color = 'rgba(3,190,252,0.8)';

      if (item.min === data.min) {
        color = '#1100CC';
      }

      if (item.max === data.max) {
        color = '#FF0000';
      }

      return {
        x: this.isMonthView
          ? +(new Date(item.date))
          : Date.UTC(this.selectedYear, item.date - 1, 1),
        y: parseFloat(item.avg.toFixed(1)),
        color: color
      };
    });

    const chartOptions: Highcharts.Options = {
      title: {
        text: data.isOnlyHumidity
          ? this.translationService.t('Average Humidity')
          : this.translationService.t('Average Temperature')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        maxPadding: 0.2,
        title: {
          text: data.isOnlyHumidity
            ? this.translationService.t('Humidity')
            : this.translationService.t('Temperature')
        }
      },
      tooltip: {
        valueSuffix: data.isOnlyHumidity ? '%' : ' °' + this.degreeType
      },
      plotOptions: {
        column: {
          minPointLength: 4,
          pointPadding: 0.1,
          groupPadding: 0,
          dataLabels: {
            enabled: !this.isMonthView,
            color: 'white',
            format: data.isOnlyHumidity ? '{y}%' : '{y} °' + this.degreeType
          }
        }
      },
      legend: {
        enabled: true
      },
      series: [{
        type: 'column',
        name: data.isOnlyHumidity ? this.translationService.t('Humidity') : this.translationService.t('Temperature'),
        showInLegend: false,
        yAxis: 0,
        color: 'rgba(3,190,252,0.8)',
        data: chartData
      }]
    };

    ChartService.setDeviceIdx(this.device.idx);
    Highcharts.chart(this.usagegraph.nativeElement, chartOptions);
  }

  private showVariationChart(data: DeviceReportData) {
    const chartData = data.items.map((item) => {
      return {
        x: this.isMonthView
          ? +(new Date(item.date))
          : Date.UTC(this.selectedYear, item.date - 1, 1),
        low: parseFloat(item.min.toFixed(1)),
        high: parseFloat(item.max.toFixed(1)),
      };
    });

    const avgData = data.items.map((item) => {
      return {
        x: this.isMonthView
          ? +(new Date(item.date))
          : Date.UTC(this.selectedYear, item.date - 1, 1),
        y: parseFloat(item.avg.toFixed(1))
      };
    });

    const chartOptions: Highcharts.Options = {
      title: {
        text: data.isOnlyHumidity
          ? this.translationService.t('Humidity Variation')
          : this.translationService.t('Temperature Variation')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        maxPadding: 0.2,
        title: {
          text: data.isOnlyHumidity
            ? this.translationService.t('Humidity')
            : this.translationService.t('Temperature')
        }
      },
      tooltip: {
        shared: true,
        valueSuffix: data.isOnlyHumidity
          ? '%'
          : ' °' + this.degreeType
      },
      plotOptions: {
        columnrange: {
          dataLabels: {
            enabled: !this.isMonthView,
            format: data.isOnlyHumidity
              ? '{y}%'
              : '{y} °' + this.degreeType
          }
        }
      },
      legend: {
        enabled: false
      },
      series: [
        {
          type: 'spline',
          name: data.isOnlyHumidity
            ? this.translationService.t('Average Humidity')
            : this.translationService.t('Average Temperature'),
          color: 'yellow',
          data: avgData
        }, {
          type: 'areasplinerange',
          name: data.isOnlyHumidity
            ? this.translationService.t('Humidity Variation')
            : this.translationService.t('Temperature Variation'),
          color: 'rgb(3,190,252)',
          fillOpacity: 0.3,
          data: chartData
        }
      ]
    };

    ChartService.setDeviceIdx(this.device.idx);
    Highcharts.chart(this.variationgraph.nativeElement, chartOptions);
  }

}

class MinMax {
  date: string;
  value: number;
}

