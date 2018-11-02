import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import * as Highcharts from 'highcharts';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-current-month-graph',
  templateUrl: './current-month-graph.component.html',
  styleUrls: ['./current-month-graph.component.css']
})
export class CurrentMonthGraphComponent implements OnInit, AfterViewInit {

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
        type: 'spline',
        marginRight: 10,
        zoomType: 'x',
        resetZoomButton: {
          position: {
            x: -30,
            y: -36
          }
        },
        events: {
          load: function () {
            _this.graphService.getData('month', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                _this.graphService.AddDataToCurrentChart(data, _this.chart, _this.switchtype, 0);
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Last Month')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: this.translationService.t('Usage')
        },
        min: 0
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

                _this.graphService.deletePoint(_this.idx, event.point, false).subscribe(() => {
                  _this.loadGraph();
                });
              }
            }
          }
        },
        spline: {
          lineWidth: 3,
          states: {
            hover: {
              lineWidth: 3
            }
          },
          marker: {
            enabled: false,
            symbol: 'circle',
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            }
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
