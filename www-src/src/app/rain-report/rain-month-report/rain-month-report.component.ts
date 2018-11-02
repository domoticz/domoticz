import {AfterViewInit, Component, ElementRef, Inject, OnInit, ViewChild} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {DeviceService} from '../../_shared/_services/device.service';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import * as Highcharts from 'highcharts';
import {GraphService} from '../../_shared/_services/graph.service';
import {GraphTempPoint} from '../../_shared/_models/graphdata';
import {ConfigService} from '../../_shared/_services/config.service';
import {Device} from '../../_shared/_models/device';
import {monthNames} from '../../_shared/_constants/months';
import {ChartService} from '../../_shared/_services/chart.service';

// FIXME use proper ts def
declare var $: any;

@Component({
  selector: 'dz-rain-month-report',
  templateUrl: './rain-month-report.component.html',
  styleUrls: ['./rain-month-report.component.css']
})
export class RainMonthReportComponent implements OnInit, AfterViewInit {

  idx: string;
  actYear: number;
  actMonth: number;

  device: Device;
  title: string;

  @ViewChild('reporttable', {static: false}) reportTable: ElementRef;
  @ViewChild('usagegraph', {static: false}) usageGraphElt: ElementRef;

  usageChart: Highcharts.Chart;

  tu = '-';
  vhigh = '-';

  constructor(
    private route: ActivatedRoute,
    private deviceService: DeviceService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService,
    private configService: ConfigService
  ) {
  }

  ngOnInit() {
    this.idx = this.route.snapshot.paramMap.get('idx');
    this.actYear = Number(this.route.snapshot.paramMap.get('year'));
    this.actMonth = Number(this.route.snapshot.paramMap.get('month'));

    this.deviceService.getDeviceInfo(this.idx).subscribe(device => {
      this.device = device;
      this.title = device.Name + ' ' + this.translationService.t(monthNames[this.actMonth - 1]) + ' ' + this.actYear;
    });
  }

  ngAfterViewInit() {
    const datatableOptions = {
      sDom: '<"H"rC>t<"F">',
      oTableTools: {
        sRowSelect: 'single'
      },
      aaSorting: [[0, 'asc']],
      aoColumnDefs: [
        {bSortable: false, aTargets: [2]}
      ],
      bSortClasses: false,
      bProcessing: true,
      bStateSave: false,
      bJQueryUI: true,
      aLengthMenu: [[50, 100, -1], [50, 100, 'All']],
      iDisplayLength: 50,
      language: this.configService.dataTableDefaultSettings.language
    };
    $(this.reportTable.nativeElement).dataTable(datatableOptions);

    this.updateTableAndGraph();

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
          return _this.translationService.t(Highcharts.dateFormat('%A', this.x)) + ' ' + Highcharts.dateFormat('%B %d', this.x) +
            '<br/>' + this.series.name + ': ' + this.y + ' mm';
        }
      },
      plotOptions: {
        column: {
          minPointLength: 4,
          pointPadding: 0.1,
          groupPadding: 0
        }
      },
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.usageChart = Highcharts.chart(this.usageGraphElt.nativeElement, chartOptions);
  }

  updateTableAndGraph() {
    const mTable = $(this.reportTable.nativeElement);
    const oTable = mTable.dataTable();
    oTable.fnClearTable();

    let total = 0;
    const datachart = [];

    let highest_val = -1;
    let highest_pos = 0;
    const highest_date: any = {};

    this.graphService.getDataWithActYearAndMonth('year', this.idx, 'rain', this.actYear, this.actMonth).subscribe(data => {
      let lastTotal = -1;
      if (data.result) {
        data.result.forEach((item: GraphTempPoint) => {
          const month = parseInt(item.d.substring(5, 7), 10);
          const year = parseInt(item.d.substring(0, 4), 10);

          if ((month === this.actMonth) && (year === this.actYear)) {
            const day = parseInt(item.d.substring(8, 10), 10);
            const mvalue = Number(item.mm);

            total += mvalue;

            if ((highest_val === -1) || (mvalue > highest_val)) {
              highest_val = mvalue;
              highest_pos = datachart.length;
              highest_date.day = day;
              highest_date.month = month;
              highest_date.year = year;
            }

            const cdate = Date.UTC(this.actYear, this.actMonth - 1, day);
            datachart.push({x: cdate, y: mvalue, color: 'rgba(3,190,252,0.8)'});

            let img;
            if ((lastTotal === -1) || (lastTotal === mvalue)) {
              img = '<img src="images/equal.png"></img>';
            } else if (mvalue < lastTotal) {
              img = '<img src="images/down.png"></img>';
            } else {
              img = '<img src="images/up.png"></img>';
            }
            lastTotal = mvalue;

            const addId = oTable.fnAddData([
              day,
              mvalue.toFixed(1),
              img
            ], false);
          }
        });
      }

      this.tu = total.toFixed(1);

      let hlstring = '-';
      if (highest_val !== -1) {
        hlstring = highest_date.day + ' ' + this.translationService.t(monthNames[highest_date.month - 1]) + ' ' +
          highest_date.year + ', ' + highest_val.toFixed(1) + ' mm';
        datachart[highest_pos].color = '#FF0000';
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

}
