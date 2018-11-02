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
import {
  DeviceEnergyMultiCounterReportDataService,
  EnergyMultiCounterReportData
} from '../device-energy-multi-counter-report-data.service';
import {ConfigService} from '../../_shared/_services/config.service';
import {DeviceUtils} from 'src/app/_shared/_utils/device-utils';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import * as dateFormat from 'dateformat';
import * as Highcharts from 'highcharts';
import {DatatableHelper} from '../../_shared/_utils/datatable-helper';
import {Router} from '@angular/router';
import {Device} from '../../_shared/_models/device';
import {ChartService} from '../../_shared/_services/chart.service';

// Proper declaration
declare var $: any;

@Component({
  selector: 'dz-device-energy-multi-counter-report',
  templateUrl: './device-energy-multi-counter-report.component.html',
  styleUrls: ['./device-energy-multi-counter-report.component.css']
})
export class DeviceEnergyMultiCounterReportComponent implements OnInit, AfterViewInit, OnChanges {

  @Input() device: Device;
  @Input() selectedYear: number;
  @Input() selectedMonth?: number;

  @ViewChild('reporttable', {static: false}) tableRef: ElementRef;
  @ViewChild('usagegraph', {static: false}) usagegraph: ElementRef;

  unit: string;
  isMonthView: boolean;
  degreeType: string;
  noDataAvailable: boolean;
  data: EnergyMultiCounterReportData;
  hasReturn: boolean;

  constructor(
    private configService: ConfigService,
    private deviceEnergyMultiCounterReportDataService: DeviceEnergyMultiCounterReportDataService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private router: Router
  ) {
  }

  ngOnInit() {
    this.unit = DeviceUtils.getUnit(this.device);
    this.isMonthView = this.selectedMonth > 0;
    this.degreeType = this.configService.config.TempSign;
  }

  ngAfterViewInit() {
    this.getData();
  }

  ngOnChanges(changes: SimpleChanges) {
    if ((changes.selectedYear && !changes.selectedYear.isFirstChange()) ||
      (changes.selectedMonth && !changes.selectedMonth.isFirstChange())) {
      this.getData();
    }
  }

  private getData() {
    this.deviceEnergyMultiCounterReportDataService.fetch(this.device.idx, this.selectedYear, this.selectedMonth)
      .subscribe((data) => {
        if (!data) {
          this.noDataAvailable = true;
          return;
        } else {
          this.noDataAvailable = false;
        }

        this.data = data;
        this.hasReturn = this.checkDataKey(data, 'return1');

        this.showTable(data);
        this.showUsageChart(data);
      });
  }

  private checkDataKey(data: EnergyMultiCounterReportData, key: string): boolean {
    return data.items.every((item) => {
      return item[key] !== undefined;
    });
  }

  private showTable(data: EnergyMultiCounterReportData) {
    const table = $(this.tableRef.nativeElement);
    const columns = [];

    const counterRenderer = (d) => {
      return d.toFixed(3);
    };

    const costRenderer = (d) => {
      return d.toFixed(2);
    };

    if (this.isMonthView) {
      columns.push({
        title: this.translationService.t('Day'),
        data: 'date',
        render: (d) => {
          return this.translationService.t(dateFormat(d, 'd'));
        }
      });

      columns.push({
        title: '',
        data: 'date',
        render: (d) => {
          return this.translationService.t(dateFormat(d, 'dddd'));
        }
      });
    } else {
      columns.push({
        title: this.translationService.t('Month'),
        width: 150,
        data: 'date',
        render: (d) => {
          const date = new Date(d);
          const link = '<a class="ng-link" href="/Devices/' + this.device.idx + '/Report/' + this.selectedYear +
            '/' + (date.getMonth() + 1) + '"><img src="images/next.png" /></a>';
          return dateFormat(d, 'mm. mmmm') + ' ' + link;
        }
      });
    }

    [
      ['usage1', 'Usage T1', 'Counter T1'],
      ['usage2', 'Usage T2', 'Counter T2'],
      ['return1', 'Return T1', 'Counter R1'],
      ['return2', 'Return T2', 'Counter R2']
    ].forEach((item: string[]) => {
      if (!this.checkDataKey(data, item[0])) {
        return;
      }
      if (this.isMonthView) {
        columns.push({title: this.translationService.t(item[2]), data: item[0] + '.counter', render: counterRenderer});
      }

      columns.push({title: this.translationService.t(item[1]), data: item[0] + '.usage', render: counterRenderer});
      columns.push({title: this.translationService.t('Costs'), data: item[0] + '.cost', render: costRenderer});
    });

    columns.push({title: this.translationService.t('Total'), data: 'cost', render: costRenderer});

    columns.push({
      title: '<>',
      orderable: false,
      data: 'trend',
      render: (d) => {
        return '<img src="images/' + d + '.png">';
      }
    });

    table.dataTable(Object.assign({}, this.configService.dataTableDefaultSettings, {
      sDom: '<"H"rC>t<"F">',
      columns: columns,
      pageLength: 50,
      order: [[0, 'asc']]
    }));

    table.dataTable().api().rows
      .add(data.items)
      .draw();

    // Fix links so that it uses Angular router
    DatatableHelper.fixAngularLinks('.ng-link', this.router);
  }

  private showUsageChart(data: EnergyMultiCounterReportData) {
    const hasUsage2 = this.checkDataKey(data, 'usage2');

    const series = [];

    series.push({
      name: hasUsage2 ? this.translationService.t('Usage') + ' 1' : this.translationService.t('Usage'),
      color: hasUsage2 ? 'rgba(60,130,252,0.8)' : 'rgba(3,190,252,0.8)',
      stack: 'susage',
      yAxis: 0,
      data: data.items.map((item) => {
        return {
          x: +(new Date(item.date)),
          y: item.usage1.usage
        };
      })
    });

    if (hasUsage2) {
      series.push({
        name: this.translationService.t('Usage') + ' 2',
        color: 'rgba(3,190,252,0.8)',
        stack: 'susage',
        yAxis: 0,
        data: data.items.map((item) => {
          return {
            x: +(new Date(item.date)),
            y: item.usage2.usage
          };
        })
      });
    }

    if (this.checkDataKey(data, 'return1')) {
      series.push({
        name: this.translationService.t('Return') + ' 1',
        color: 'rgba(30,242,110,0.8)',
        stack: 'sreturn',
        yAxis: 0,
        data: data.items.map((item) => {
          return {
            x: +(new Date(item.date)),
            y: item.return1.usage
          };
        })
      });
    }

    if (this.checkDataKey(data, 'return2')) {
      series.push({
        name: this.translationService.t('Return') + ' 2',
        color: 'rgba(3,252,190,0.8)',
        stack: 'sreturn',
        yAxis: 0,
        data: data.items.map((item) => {
          return {
            x: +(new Date(item.date)),
            y: item.return2.usage
          };
        })
      });
    }

    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        type: 'column',
      },
      title: {
        text: ''
      },
      xAxis: {
        type: 'datetime',
        minTickInterval: this.isMonthView ? 24 * 3600 * 1000 : 28 * 24 * 3600 * 1000
      },
      yAxis: {
        title: {
          text: this.translationService.t('Energy') + ' (' + this.unit + ')'
        },
        maxPadding: 0.2,
        min: 0
      },
      tooltip: {
        formatter: function () {
          return _this.translationService.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%Y-%m-%d', this.x) +
            '<br/>' + _this.translationService.t(this.series.name) + ': ' + Highcharts.numberFormat(this.y, 3) +
            ' ' + _this.unit + '<br/>Total: ' + this.point['stackTotal'] + ' ' + _this.unit;
        }
      },
      plotOptions: {
        column: {
          stacking: 'normal',
          minPointLength: 4,
          pointPadding: 0.1,
          groupPadding: 0,
        }
      },
      legend: {
        enabled: true
      },
      series: series
    };

    ChartService.setDeviceIdx(this.device.idx);
    Highcharts.chart(this.usagegraph.nativeElement, chartOptions);
  }

}
