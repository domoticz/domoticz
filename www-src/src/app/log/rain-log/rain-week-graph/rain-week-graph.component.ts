import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import {GraphTempPoint} from '../../../_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-rain-week-graph',
  templateUrl: './rain-week-graph.component.html',
  styleUrls: ['./rain-week-graph.component.css']
})
export class RainWeekGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;

  @ViewChild('weekgraph', {static: false}) chartTarget: ElementRef;

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
        type: 'column',
        events: {
          load: () => {
            _this.graphService.getData('week', _this.idx, 'rain').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                const series = _this.chart.series[0];
                const datatable = [];
                data.result.forEach((item: GraphTempPoint) => {
                  datatable.push([Utils.GetDateFromString(item.d), Number(item.mm)]);
                });
                series.setData(datatable); // redraws
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Rainfall') + ' ' + this.translationService.t('Last Week')
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: {
          day: {
            main: '%a'
          }
        },
        tickInterval: 24 * 3600 * 1000
      },
      yAxis: {
        min: 0,
        maxPadding: 0.2,
        title: {
          text: this.translationService.t('Rainfall') + ' (mm)'
        }
      },
      tooltip: {
        formatter: function () {
          return '' + _this.translationService.t(Highcharts.dateFormat('%A', this.x)) + '<br/>' +
            Highcharts.dateFormat('%Y-%m-%d', this.x) + ': ' + this.y + ' mm';
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
      series: [{
        id: 'mm',
        type: 'column',
        name: 'mm',
        color: 'rgba(3,190,252,0.8)'
      }]
      ,
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
