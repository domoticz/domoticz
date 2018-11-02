import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import * as Highcharts from 'highcharts';
import {Options} from 'highcharts';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-counter-day-graph',
  templateUrl: './counter-day-graph.component.html',
  styleUrls: ['./counter-day-graph.component.css']
})
export class CounterDayGraphComponent implements OnInit, AfterViewInit {

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

    let graph_title = (this.switchtype === 4) ? this.translationService.t('Generated') : this.translationService.t('Usage');
    graph_title += ' ' + this.graphService.Get5MinuteHistoryDaysGraphTitle();

    const options: Options = {
      chart: {
        type: 'column',
        marginRight: 10,
        zoomType: 'x',
        events: {
          load: () => {
            _this.graphService.getData('day', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                _this.graphService.AddDataToUtilityChart(data, _this.chart, _this.switchtype, 'day');
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: graph_title
      },
      xAxis: {
        type: 'datetime',
        labels: {
          formatter: function () {
            return Highcharts.dateFormat('%H:%M', this.value);
          }
        }
      },
      yAxis: {
        title: {
          text: this.translationService.t('Energy') + ' (Wh)'
        }
      },
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

                _this.graphService.deletePoint(_this.idx, event.point, true).subscribe(() => {
                  _this.loadGraph();
                });
              }
            }
          }
        },
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
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
