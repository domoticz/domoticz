import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {DeviceService} from '../../_shared/_services/device.service';
import * as Highcharts from 'highcharts';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from '../../_shared/_services/graph.service';
import {GraphTempPoint} from '../../_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ConfigService} from '../../_shared/_services/config.service';
import {DatatableHelper} from '../../_shared/_utils/datatable-helper';
import {Device} from '../../_shared/_models/device';
import {monthNames} from '../../_shared/_constants/months';
import {ChartService} from '../../_shared/_services/chart.service';

// FIXME use proper ts def
declare var $: any;

@Component({
  selector: 'dz-rain-report',
  templateUrl: './rain-report.component.html',
  styleUrls: ['./rain-report.component.css']
})
export class RainReportComponent implements OnInit, AfterViewInit {

  idx: string;
  device: Device;
  actYear: number;

  tu = '-';
  mhigh = '-';
  mlow = '-';
  vhigh = '-';

  @ViewChild('reporttable', {static: false}) reportTable: ElementRef;
  @ViewChild('usagegraph', {static: false}) usageGraphElt: ElementRef;

  usageChart: Highcharts.Chart;

  years: Array<number> = [];

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService,
    private configService: ConfigService,
    private router: Router
  ) {
  }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');

    this.deviceService.getDeviceInfo(this.idx).subscribe(device => {
      this.device = device;
    });

    const d = new Date();
    // const actMonth = d.getMonth() + 1;
    this.actYear = d.getFullYear();

    // List of years
    const min = 2012;
    const max = new Date().getFullYear();
    for (let i = min; i <= max; i++) {
      this.years.push(i);
    }
  }

  ngAfterViewInit() {
    const datatableOptions = {
      'sDom': '<"H"rC>t<"F">',
      'oTableTools': {
        'sRowSelect': 'single'
      },
      'aaSorting': [[0, 'asc']],
      'aoColumnDefs': [
        {'bSortable': false, 'aTargets': [2]}
      ],
      'bSortClasses': false,
      'bProcessing': true,
      'bStateSave': false,
      'bJQueryUI': true,
      'aLengthMenu': [[50, 100, -1], [50, 100, 'All']],
      'iDisplayLength': 50,
      language: this.configService.dataTableDefaultSettings.language
    };
    $(this.reportTable.nativeElement).dataTable(datatableOptions);

    const _this = this;

    const chartOptions: Highcharts.Options = {
      chart: {
        type: 'column'
      },
      title: {
        text: ''
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        min: 0,
        maxPadding: 0.2,
        title: {
          text: this.translationService.t('Rain')
        }
      },
      tooltip: {
        formatter: function () {
          return _this.translationService.t(Highcharts.dateFormat('%B', this.x)) + '<br/>' + this.series.name + ': ' + this.y + ' mm';
        }
      },
      plotOptions: {
        column: {
          minPointLength: 4,
          pointPadding: 0.1,
          groupPadding: 0,
          dataLabels: {
            enabled: true,
            color: 'white'
          }
        }
      },
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.usageChart = Highcharts.chart(this.usageGraphElt.nativeElement, chartOptions);

    this.updateTableAndGraph();
  }

  updateTableAndGraph() {
    const mTable = $(this.reportTable.nativeElement);
    const oTable = mTable.dataTable();
    oTable.fnClearTable();

    let total = 0;
    let global = 0;
    const datachart = [];

    let highest_month_val = -1;
    let highest_month = 0;
    let highest_month_pos = 0;
    let lowest_month_val = -1;
    let lowest_month = 0;
    let highest_val = -1;
    const highest_date: any = {};

    this.graphService.getDataWithActYear('year', this.idx, 'rain', this.actYear).subscribe(data => {

      let lastTotal = -1;
      let lastMonth = -1;

      if (data.result) {
        data.result.forEach((item: GraphTempPoint) => {
          const month = parseInt(item.d.substring(5, 7), 10);
          const year = parseInt(item.d.substring(0, 4), 10);
          const day = parseInt(item.d.substring(8, 10), 10);
          if (year === this.actYear) {
            if (lastMonth === -1) {
              lastMonth = month;
            }
            if (lastMonth !== month) {
              // add totals to table
              lastTotal = this.Add2YearTableReportRain(oTable, total, lastTotal, lastMonth, this.actYear);

              if ((highest_month_val === -1) || (total > highest_month_val)) {
                highest_month_val = total;
                highest_month_pos = datachart.length;
                highest_month = lastMonth;
              }
              if ((lowest_month_val === -1) || (total < lowest_month_val)) {
                lowest_month_val = total;
                lowest_month = lastMonth;
              }

              const cdate = Date.UTC(this.actYear, lastMonth - 1, 1);
              datachart.push({x: cdate, y: parseFloat(total.toFixed(1)), color: 'rgba(3,190,252,0.8)'});

              lastMonth = month;
              global += total;
              total = 0;
            }
            const mvalue = Number(item.mm);
            total += mvalue;

            if ((highest_val === -1) || (mvalue > highest_val)) {
              highest_val = mvalue;
              highest_date.day = day;
              highest_date.month = month;
              highest_date.year = year;
            }
          }
        });
      }

      // add last month
      if (total !== 0) {
        lastTotal = this.Add2YearTableReportRain(oTable, total, lastTotal, lastMonth, this.actYear);

        if ((highest_month_val === -1) || (lastTotal > highest_month_val)) {
          highest_month_val = lastTotal;
          highest_month_pos = datachart.length;
          highest_month = lastMonth;
        }
        if ((lowest_month_val === -1) || (lastTotal < lowest_month_val)) {
          lowest_month_val = lastTotal;
          lowest_month = lastMonth;
        }

        const cdate = Date.UTC(this.actYear, lastMonth - 1, 1);
        datachart.push({x: cdate, y: parseFloat(total.toFixed(1)), color: 'rgba(3,190,252,0.8)'});
        global += lastTotal;
      }

      this.tu = global.toFixed(1);

      let hlstring = '-';
      if (highest_month_val !== -1) {
        hlstring = this.translationService.t(monthNames[highest_month - 1]) + ', ' + highest_month_val.toFixed(1) + ' mm';
        datachart[highest_month_pos].color = '#FF0000';
      }
      this.mhigh = hlstring;

      hlstring = '-';
      if (lowest_month_val !== -1) {
        hlstring = this.translationService.t(monthNames[lowest_month - 1]) + ', ' + lowest_month_val.toFixed(1) + ' mm';
      }
      this.mlow = hlstring;

      hlstring = '-';
      if (highest_val !== -1) {
        hlstring = highest_date.day + ' ' + this.translationService.t(monthNames[highest_date.month - 1]) + ' ' +
          highest_date.year + ', ' + highest_val.toFixed(1) + ' mm';
      }
      this.vhigh = hlstring;

      this.usageChart.addSeries(<Highcharts.SeriesColumnOptions>{
        id: 'rain',
        name: this.translationService.t('Rain'),
        showInLegend: false,
        color: 'rgba(3,190,252,0.8)',
        yAxis: 0
      });
      const series = <Highcharts.Series>this.usageChart.get('rain');
      series.setData(datachart);

      mTable.fnDraw();

      DatatableHelper.fixAngularLinks('.ng-link', this.router);

      /* Add a click handler to the rows - this could be used as a callback */
      $('#reporttable tbody tr').click(function (e) {
        if ($(this).hasClass('row_selected')) {
          $(this).removeClass('row_selected');
        } else {
          oTable.$('tr.row_selected').removeClass('row_selected');
          $(this).addClass('row_selected');
        }
      });
    });
  }

  private Add2YearTableReportRain(oTable: any, total: number, lastTotal: number, lastMonth: number, actYear: number) {
    let img;
    if ((lastTotal === -1) || (lastTotal === total)) {
      img = '<img src="images/equal.png"></img>';
    } else if (total < lastTotal) {
      img = '<img src="images/down.png"></img>';
    } else {
      img = '<img src="images/up.png"></img>';
    }
    let monthtxt = Utils.addLeadingZeros(lastMonth, 2) + '. ' + this.translationService.t(monthNames[lastMonth - 1]) + ' ';
    monthtxt += '<a class="ng-link" href="/RainReport/' + this.idx + '/' + actYear + '/' + lastMonth + '"><img src="images/next.png">';

    const addId = oTable.fnAddData([
      monthtxt,
      total.toFixed(1),
      img
    ], false);
    return total;
  }

}
