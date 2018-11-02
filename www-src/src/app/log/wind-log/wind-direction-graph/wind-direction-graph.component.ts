import {Component, OnInit, AfterViewInit, Input, ViewChild, ElementRef, Inject} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-wind-direction-graph',
  templateUrl: './wind-direction-graph.component.html',
  styleUrls: ['./wind-direction-graph.component.css']
})
export class WindDirectionGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;

  @ViewChild('graph', {static: false}) chartTarget: ElementRef;

  private chart: Highcharts.Chart;

  private wind_directions = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];

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
        polar: true,
        type: 'column',
        events: {
          load: () => {
            _this.graphService.getData('day', _this.idx, 'winddir').subscribe(data => {
              if (typeof data.result_speed !== 'undefined') {
                data.result_speed.forEach((item: any, i: number) => {
                  // make the series
                  const datatable_speed = [];
                  for (let jj = 0; jj < 16; jj++) {
                    datatable_speed.push([_this.wind_directions[i], Number(item.sp[jj])]);
                  }
                  _this.chart.addSeries(<Highcharts.SeriesColumnOptions>{
                    id: String(i + 1),
                    name: item.label
                  }, false);
                  const series = <Highcharts.Series>_this.chart.get(String(i + 1));
                  series.setData(datatable_speed, false);
                });
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Wind') + ' ' + this.translationService.t('Direction') + ' ' +
          this.graphService.Get5MinuteHistoryDaysGraphTitle()
      },
      pane: {
        size: '85%'
      },
      xAxis: {
        tickmarkPlacement: 'on',
        tickWidth: 1,
        tickPosition: 'outside',
        tickLength: 5,
        tickColor: '#999',
        tickInterval: 1,
        labels: {
          formatter: function () {
            return _this.wind_directions[this.value] + '°'; // return text for label
          }
        }
      },
      yAxis: {
        min: 0,
        showLastLabel: true,
        title: {
          text: 'Frequency (%)'
        },
        labels: {
          formatter: function () {
            return this.value + '%';
          }
        },
        reversedStacks: false
      },
      tooltip: {
        formatter: function () {
          return '' + _this.wind_directions[this.x] + ': ' + this.y + ' %';
        }
      },
      plotOptions: {
        series: {
          stacking: 'normal',
          shadow: false,
          pointPlacement: 'on',
        },
        variwide: {
          groupPadding: 0,
        }
      },
      legend: {
        align: 'right',
        verticalAlign: 'top',
        y: 100,
        layout: 'vertical'
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
