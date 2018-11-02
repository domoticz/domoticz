import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-counter-week-graph',
  templateUrl: './counter-week-graph.component.html',
  styleUrls: ['./counter-week-graph.component.css']
})
export class CounterWeekGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;
  @Input() switchtype: number;

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

    const options: Highcharts.Options = {
      chart: {
        type: 'column',
        marginRight: 10,
        events: {
          load: () => {
            _this.graphService.getData('week', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                _this.graphService.AddDataToUtilityChart(data, _this.chart, _this.switchtype, 'week');
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Last Week')
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
        title: {
          text: this.translationService.t('Energy') + ' (kWh)'
        },
        maxPadding: 0.2
      },
      tooltip: {
        crosshairs: true,
        shared: true
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
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
