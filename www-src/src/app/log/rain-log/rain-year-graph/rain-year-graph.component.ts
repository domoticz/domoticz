import {AfterViewInit, Component, ElementRef, Inject, Input, OnInit, ViewChild} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {GraphService} from 'src/app/_shared/_services/graph.service';
import * as Highcharts from 'highcharts';
import {GraphTempPoint} from '../../../_shared/_models/graphdata';
import {Utils} from 'src/app/_shared/_utils/utils';
import {ClickPointEvent} from 'src/app/log/device-log/temperature-log-chart/temperature-log-chart.component';
import {ChartService} from "../../../_shared/_services/chart.service";

@Component({
  selector: 'dz-rain-year-graph',
  templateUrl: './rain-year-graph.component.html',
  styleUrls: ['./rain-year-graph.component.css']
})
export class RainYearGraphComponent implements OnInit, AfterViewInit {

  @Input() idx: string;

  @ViewChild('yeargraph', {static: false}) chartTarget: ElementRef;

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
          load: () => {
            _this.graphService.getData('year', _this.idx, 'rain').subscribe(data => {
              if (typeof data.result !== 'undefined') {
                let series = _this.chart.series[0];
                let datatable = [];
                data.result.forEach((item: GraphTempPoint) => {
                  datatable.push([Utils.GetDateFromString(item.d), Number(item.mm)]);
                });
                series.setData(datatable, false);
                const bHavePrev = (typeof data.resultprev !== 'undefined');
                if (bHavePrev) {
                  datatable = [];
                  series = _this.chart.series[1];
                  data.resultprev.forEach((item: GraphTempPoint) => {
                    datatable.push([Utils.GetPrevDateFromString(item.d), Number(item.mm)]);
                  });
                  series.setData(datatable, false);
                }
                _this.chart.redraw();
              }
            });
          }
        }
      },
      title: {
        text: this.translationService.t('Rainfall') + ' ' + this.translationService.t('Last Year')
      },
      xAxis: {
        type: 'datetime'
      },
      yAxis: {
        title: {
          text: this.translationService.t('Rainfall') + ' (mm)'
        },
        min: 0
      },
      tooltip: {
        crosshairs: true,
        shared: true
      },
      plotOptions: {
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
      series: [
        {
          id: 'mm',
          type: 'spline',
          name: 'mm',
          color: 'rgba(3,190,252,0.8)',
          tooltip: {
            valueSuffix: ' mm',
            valueDecimals: 1
          },
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
        {
          id: 'past_mm',
          type: 'spline',
          name: this.translationService.t('Past') + ' mm',
          visible: false,
          color: 'rgba(190,3,252,0.8)',
          tooltip: {
            valueSuffix: ' mm',
            valueDecimals: 1
          },
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
        }
      ],
      legend: {
        enabled: true
      }
    };

    ChartService.setDeviceIdx(this.idx);
    this.chart = Highcharts.chart(this.chartTarget.nativeElement, options);
  }

}
