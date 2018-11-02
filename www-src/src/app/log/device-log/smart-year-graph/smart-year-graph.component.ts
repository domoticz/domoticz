import { Component, OnInit, AfterViewInit, Input, ViewChild, ElementRef, Inject } from '@angular/core';
import { I18NEXT_SERVICE, ITranslationService } from 'angular-i18next';
import { GraphService } from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import { ClickPointEvent } from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-smart-year-graph',
  templateUrl: './smart-year-graph.component.html',
  styleUrls: ['./smart-year-graph.component.css']
})
export class SmartYearGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;
  @Input() switchtype: number;

  @ViewChild('graph', { static: false }) chartTarget: ElementRef;

  private chart: Highcharts.Chart;

  constructor(
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private graphService: GraphService
  ) { }

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
          load: () => {
            _this.graphService.getData('year', _this.idx, 'counter').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                _this.graphService.AddDataToUtilityChart(data, _this.chart, _this.switchtype, 'year');
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Last Year')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: this.translationService.t('Energy') + ' (kWh)'
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
            states: {
              hover: {
                enabled: true,
                radius: 5,
                lineWidth: 1
              }
            },
            symbol: 'circle'
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
