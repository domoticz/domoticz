import {Injectable, Inject} from '@angular/core';
import {I18NEXT_SERVICE, ITranslationService} from 'angular-i18next';
import {ConfigService} from './config.service';
import {PermissionService} from './permission.service';
import {NotificationService} from './notification.service';
import {Observable, of, Subject, throwError} from 'rxjs';
import * as Highcharts from 'highcharts';
import {ApiService} from './api.service';
import {GraphData, GraphTempPoint} from '../_models/graphdata';
import {Utils} from '../_utils/utils';
import {map} from 'rxjs/operators';

// FIXME manage with NPM+TypeScript
declare var bootbox: any;

@Injectable()
export class GraphService {

  constructor(
    private configService: ConfigService,
    @Inject(I18NEXT_SERVICE) private translationService: ITranslationService,
    private permissionService: PermissionService,
    private notificationService: NotificationService,
    private apiService: ApiService) {
  }

  public Get5MinuteHistoryDaysGraphTitle(): string {
    const FiveMinuteHistoryDays = this.configService.config.FiveMinuteHistoryDays;
    if (FiveMinuteHistoryDays === 1) {
      return this.translationService.t('Last') + ' 24 ' + this.translationService.t('Hours');
    } else if (FiveMinuteHistoryDays === 2) {
      return this.translationService.t('Last') + ' 48 ' + this.translationService.t('Hours');
    } else {
      return this.translationService.t('Last') + ' ' + FiveMinuteHistoryDays + ' ' + this.translationService.t('Days');
    }
  }

  public getData(range: string, idx: string, sensor: string): Observable<GraphData> {
    return this.apiService.callApi<GraphData>('graph', {sensor: sensor, range: range, idx: idx}).pipe(
      map(this.fixDataTypes)
    );
  }

  public getDataWithMethod(range: string, idx: string, sensor: string, method: number): Observable<GraphData> {
    return this.apiService.callApi<GraphData>('graph', {
      sensor: sensor,
      range: range,
      idx: idx,
      method: method.toString()
    }).pipe(
      map(this.fixDataTypes)
    );
  }

  public getDataWithActYear(range: string, idx: string, sensor: string, actyear: number): Observable<GraphData> {
    return this.getDataWithActYearAndMonth(range, idx, sensor, actyear);
  }

  getDataWithActYearAndMonth(range: string, idx: string, sensor: string, actyear: number, actmonth?: number): Observable<GraphData> {
    let params: any = {sensor: sensor, range: range, idx: idx, actyear: actyear.toString()};
    if (actmonth !== undefined) {
      params = {...params, actmonth: actmonth.toString()};
    }
    return this.apiService.callApi<GraphData>('graph', params).pipe(
      map(this.fixDataTypes)
    );
  }

  getDataCustom(sensor: string, idx: string, range: string, graphtype: string,
                graphTemp: boolean,
                graphChill: boolean,
                graphHum: boolean,
                graphBaro: boolean,
                graphDew: boolean,
                graphSet: boolean): Observable<GraphData> {
    const params = {
      sensor: sensor,
      idx: idx,
      range: range,
      graphtype: graphtype,
      graphTemp: graphTemp.toString(),
      graphChill: graphChill.toString(),
      graphHum: graphHum.toString(),
      graphBaro: graphBaro.toString(),
      graphDew: graphDew.toString(),
      graphSet: graphSet.toString()
    };
    return this.apiService.callApi<GraphData>('graph', params)
      .pipe(
        map(this.fixDataTypes)
      );
  }

  // Convert strings to numbers as API is not consistent...
  private fixDataTypes(response: GraphData): GraphData {
    const newResult = response.result.map(point => {
      if (typeof point.hu === 'string') {
        point.hu = Number(point.hu);
      }
      return point;
    });
    response.result = newResult;
    return response;
  }

  // Previously in domoticzDataPointApi
  // FIXME use an observable of boolean or rewrite in a different way
  public deletePoint(deviceIdx: string, point: Highcharts.Point, isShort: boolean): Observable<void> {
    if (!this.permissionService.hasPermission('Admin')) {
      this.notificationService.HideNotify();
      this.notificationService.ShowNotify(this.translationService.t('You do not have permission to do that!'), 2500, true);
      return throwError(null);
    }

    const dateString = isShort
      ? Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', point.x)
      : Highcharts.dateFormat('%Y-%m-%d', point.x);

    const message = this.translationService.t('Are you sure to remove this value at') + ' ?:\n\n' +
      this.translationService.t('Date') + ': ' + dateString + ' \n' +
      this.translationService.t('Value') + ': ' + point.y;

    const obs = new Subject<void>();

    const _this = this;

    bootbox.confirm(message, function (result) {
      if (result !== true) {
        obs.error(null);
        return;
      }

      _this.apiService.callApi('command', {
        param: 'deletedatapoint',
        idx: deviceIdx.toString(),
        date: dateString
      }).subscribe(() => {
        obs.next(undefined);
      }, () => {
        _this.notificationService.HideNotify();
        _this.notificationService.ShowNotify(_this.translationService.t('Problem deleting data point!'), 2500, true);
        obs.error(null);
      });
    });

    return obs;
  }

  public AddDataToTempChart(data: GraphData, chart: Highcharts.Chart, isday: number, isthermostat: boolean): void {
    const datatablete: Array<[number, number]> = [];
    const datatabletm: Array<[number, number]> = [];
    const datatableta: Array<[number, number]> = [];
    const datatabletrange: Array<[number, number, number]> = [];

    const datatablehu: Array<[number, number]> = [];
    const datatablech: Array<[number, number]> = [];
    const datatablecm: Array<[number, number]> = [];
    const datatabledp: Array<[number, number]> = [];
    const datatableba: Array<[number, number]> = [];

    const datatablese: Array<[number, number]> = [];
    const datatablesm: Array<[number, number]> = [];
    const datatablesx: Array<[number, number]> = [];
    const datatablesrange: Array<[number, number, number]> = [];

    const datatablete_prev: Array<[number, number]> = [];
    const datatabletm_prev: Array<[number, number]> = [];
    const datatableta_prev: Array<[number, number]> = [];
    const datatabletrange_prev: Array<[number, number, number]> = [];

    const datatablehu_prev: Array<[number, number]> = [];
    const datatablech_prev: Array<[number, number]> = [];
    const datatablecm_prev: Array<[number, number]> = [];
    const datatabledp_prev: Array<[number, number]> = [];
    const datatableba_prev: Array<[number, number]> = [];

    const datatablese_prev: Array<[number, number]> = [];
    const datatablesm_prev: Array<[number, number]> = [];
    const datatablesx_prev: Array<[number, number]> = [];
    const datatablesrange_prev: Array<[number, number, number]> = [];

    const bHavePrev = (typeof data.resultprev !== 'undefined');
    if (bHavePrev) {
      data.resultprev.forEach((item: GraphTempPoint) => {
        if (typeof item.te !== 'undefined') {
          datatablete_prev.push([Utils.GetPrevDateFromString(item.d), item.te]);
          datatabletm_prev.push([Utils.GetPrevDateFromString(item.d), item.tm]);
          datatabletrange_prev.push([Utils.GetPrevDateFromString(item.d), item.tm, item.te]);
          if (typeof item.ta !== 'undefined') {
            datatableta_prev.push([Utils.GetPrevDateFromString(item.d), item.ta]);
          }
        }
        if (typeof item.hu !== 'undefined') {
          datatablehu_prev.push([Utils.GetPrevDateFromString(item.d), item.hu]);
        }
        if (typeof item.ch !== 'undefined') {
          datatablech_prev.push([Utils.GetPrevDateFromString(item.d), item.ch]);
          datatablecm_prev.push([Utils.GetPrevDateFromString(item.d), item.cm]);
        }
        if (typeof item.dp !== 'undefined') {
          datatabledp_prev.push([Utils.GetPrevDateFromString(item.d), item.dp]);
        }
        if (typeof item.ba !== 'undefined') {
          datatableba_prev.push([Utils.GetPrevDateFromString(item.d), item.ba]);
        }
        if (typeof item.se !== 'undefined') {
          datatablese_prev.push([Utils.GetPrevDateFromString(item.d), item.se]);
        }
        if (typeof item.sm !== 'undefined' && typeof item.sx !== 'undefined') {
          datatablesm_prev.push([Utils.GetPrevDateFromString(item.d), item.sm]);
          datatablesx_prev.push([Utils.GetPrevDateFromString(item.d), item.sx]);
          datatablesrange_prev.push([Utils.GetPrevDateFromString(item.d), item.sm, item.sx]);
        }
      });
    }

    data.result.forEach((item: GraphTempPoint) => {
      if (isday === 1) {
        if (typeof item.te !== 'undefined') {
          datatablete.push([Utils.GetUTCFromString(item.d), item.te]);
        }
        if (typeof item.hu !== 'undefined') {
          datatablehu.push([Utils.GetUTCFromString(item.d), item.hu]);
        }
        if (typeof item.ch !== 'undefined') {
          datatablech.push([Utils.GetUTCFromString(item.d), item.ch]);
        }
        if (typeof item.dp !== 'undefined') {
          datatabledp.push([Utils.GetUTCFromString(item.d), item.dp]);
        }
        if (typeof item.ba !== 'undefined') {
          datatableba.push([Utils.GetUTCFromString(item.d), item.ba]);
        }
        if (typeof item.se !== 'undefined') {
          datatablese.push([Utils.GetUTCFromString(item.d), item.se]);
        }
      } else {
        if (typeof item.te !== 'undefined') {
          datatablete.push([Utils.GetDateFromString(item.d), item.te]);
          datatabletm.push([Utils.GetDateFromString(item.d), item.tm]);
          datatabletrange.push([Utils.GetDateFromString(item.d), item.tm, item.te]);
          if (typeof item.ta !== 'undefined') {
            datatableta.push([Utils.GetDateFromString(item.d), item.ta]);
          }
        }
        if (typeof item.hu !== 'undefined') {
          datatablehu.push([Utils.GetDateFromString(item.d), item.hu]);
        }
        if (typeof item.ch !== 'undefined') {
          datatablech.push([Utils.GetDateFromString(item.d), item.ch]);
          datatablecm.push([Utils.GetDateFromString(item.d), item.cm]);
        }
        if (typeof item.dp !== 'undefined') {
          datatabledp.push([Utils.GetDateFromString(item.d), item.dp]);
        }
        if (typeof item.ba !== 'undefined') {
          datatableba.push([Utils.GetDateFromString(item.d), item.ba]);
        }
        if (typeof item.se !== 'undefined') {
          datatablese.push([Utils.GetDateFromString(item.d), item.se]); // avergae
          datatablesm.push([Utils.GetDateFromString(item.d), item.sm]); // min
          datatablesx.push([Utils.GetDateFromString(item.d), item.sx]); // max
          datatablesrange.push([Utils.GetDateFromString(item.d), item.sm, item.sx]);
        }
      }
    });

    let series;
    if (datatablehu.length !== 0) {
      chart.addSeries(<Highcharts.SeriesSplineOptions>{
        id: 'humidity',
        name: this.translationService.t('Humidity'),
        color: 'limegreen',
        yAxis: 1,
        tooltip: {
          valueSuffix: ' %',
          valueDecimals: 0
        }
      }, false);
      series = chart.get('humidity');
      series.setData(datatablehu, false);
    }

    if (datatablech.length !== 0) {
      chart.addSeries(<Highcharts.SeriesSplineOptions>{
        id: 'chill',
        name: this.translationService.t('Chill'),
        color: 'red',
        zIndex: 1,
        tooltip: {
          valueSuffix: ' \u00B0' + this.configService.config.TempSign,
          valueDecimals: 1
        },
        yAxis: 0
      }, false);
      series = chart.get('chill');
      series.setData(datatablech, false);

      if (isday === 0) {
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
          id: 'chillmin',
          name: this.translationService.t('Chill') + '_min',
          color: 'rgba(255,127,39,0.8)',
          linkedTo: ':previous',
          zIndex: 1,
          tooltip: {
            valueSuffix: ' \u00B0' + this.configService.config.TempSign,
            valueDecimals: 1
          },
          yAxis: 0
        }, false);
        series = chart.get('chillmin');
        series.setData(datatablecm, false);
      }
    }

    if (datatablese.length !== 0) {
      if (isday === 1) {
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
          id: 'setpoint',
          name: this.translationService.t('Set Point'),
          color: 'blue',
          zIndex: 1,
          tooltip: {
            valueSuffix: ' \u00B0' + this.configService.config.TempSign,
            valueDecimals: 1
          },
          yAxis: 0
        }, false);
        series = chart.get('setpoint');
        series.setData(datatablese, false);
      } else {
        chart.addSeries(<any>{
          id: 'setpointavg',
          name: this.translationService.t('Set Point') + '_avg',
          color: 'blue',
          fillOpacity: 0.7,
          zIndex: 2,
          tooltip: {
            valueSuffix: ' \u00B0' + this.configService.config.TempSign,
            valueDecimals: 1
          },
          yAxis: 0
        }, false);
        series = chart.get('setpointavg');
        series.setData(datatablese, false);
        /*
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'setpointmin',
            name: $.t('Set Point') + '_min',
            color: 'rgba(39,127,255,0.8)',
            linkedTo: ':previous',
            zIndex: 1,
            tooltip: {
                valueSuffix: ' \u00B0' + this.configService.config.TempSign,
                valueDecimals: 1
            },
            yAxis: 0
        }, false);
        series = chart.get('setpointmin');
        series.setData(datatablesm, false);

        chart.addSeries( {
            id: 'setpointmax',
            name: $.t('Set Point') + '_max',
            color: 'rgba(127,39,255,0.8)',
            linkedTo: ':previous',
            zIndex: 1,
            tooltip: {
                valueSuffix: ' \u00B0' + this.configService.config.TempSign,
                valueDecimals: 1
            },
            yAxis: 0
        }, false);
        series = chart.get('setpointmax');
        series.setData(datatablesx, false);
        */

        if (datatablesrange.length !== 0) {
          chart.addSeries(<any>{
            id: 'setpointrange',
            name: this.translationService.t('Set Point') + '_range',
            color: 'rgba(164,75,148,1.0)',
            type: 'areasplinerange',
            linkedTo: ':previous',
            zIndex: 1,
            lineWidth: 0,
            fillOpacity: 0.5,
            yAxis: 0,
            tooltip: {
              valueSuffix: ' \u00B0' + this.configService.config.TempSign,
              valueDecimals: 1
            }
          }, false);
          series = chart.get('setpointrange');
          series.setData(datatablesrange, false);
        }
        if (datatablese_prev.length !== 0) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'prev_setpoint',
            name: this.translationService.t('Past') + ' ' + this.translationService.t('Set Point'),
            color: 'rgba(223,212,246,0.8)',
            zIndex: 3,
            yAxis: 0,
            tooltip: {
              valueSuffix: ' \u00B0' + this.configService.config.TempSign,
              valueDecimals: 1
            },
            visible: false
          }, false);
          series = chart.get('prev_setpoint');
          series.setData(datatablese_prev, false);
        }
      }
    }

    let datatableTrendline;

    if (datatablete.length !== 0) {
      // Add Temperature series
      if (isday === 1) {
        chart.addSeries(<any>{
          id: 'temperature',
          name: this.translationService.t('Temperature'),
          color: 'yellow',
          yAxis: 0,
          step: (isthermostat) ? 'left' : null,
          tooltip: {
            valueSuffix: ' \u00B0' + this.configService.config.TempSign,
            valueDecimals: 1
          }
        }, false);
        series = chart.get('temperature');
        series.setData(datatablete, false);
      } else {
        // Min/Max range
        if (datatableta.length !== 0) {
          chart.addSeries(<any>{
            id: 'temperature_avg',
            name: this.translationService.t('Temperature'),
            color: 'yellow',
            fillOpacity: 0.7,
            yAxis: 0,
            zIndex: 2,
            tooltip: {
              valueSuffix: ' \u00B0' + this.configService.config.TempSign,
              valueDecimals: 1
            }
          }, false);
          series = chart.get('temperature_avg');
          series.setData(datatableta, false);
          const trandLine = Utils.CalculateTrendLine(datatableta);
          if (typeof trandLine !== 'undefined') {
            datatableTrendline = [];
            datatableTrendline.push([trandLine.x0, trandLine.y0]);
            datatableTrendline.push([trandLine.x1, trandLine.y1]);
          }
        }
        if (datatabletrange.length !== 0) {
          chart.addSeries(<any>{
            id: 'temperature',
            name: this.translationService.t('Temperature') + '_range',
            color: 'rgba(3,190,252,1.0)',
            type: 'areasplinerange',
            linkedTo: ':previous',
            zIndex: 0,
            lineWidth: 0,
            fillOpacity: 0.5,
            yAxis: 0,
            tooltip: {
              valueSuffix: ' \u00B0' + this.configService.config.TempSign,
              valueDecimals: 1
            }
          }, false);
          series = chart.get('temperature');
          series.setData(datatabletrange, false);
        }
        if (datatableta_prev.length !== 0) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'prev_temperature',
            name: this.translationService.t('Past') + ' ' + this.translationService.t('Temperature'),
            color: 'rgba(224,224,230,0.8)',
            zIndex: 3,
            yAxis: 0,
            tooltip: {
              valueSuffix: ' \u00B0' + this.configService.config.TempSign,
              valueDecimals: 1
            },
            visible: false
          }, false);
          series = chart.get('prev_temperature');
          series.setData(datatableta_prev, false);
        }
      }
    }
    if (typeof datatableTrendline !== 'undefined') {
      if (datatableTrendline.length > 0) {
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
          id: 'temp_trendline',
          name: this.translationService.t('Trendline') + ' ' + this.translationService.t('Temperature'),
          zIndex: 1,
          tooltip: {
            valueSuffix: ' \u00B0' + this.configService.config.TempSign,
            valueDecimals: 1
          },
          color: 'rgba(255,3,3,0.8)',
          dashStyle: 'LongDash',
          yAxis: 0,
          visible: false
        }, false);
        series = chart.get('temp_trendline');
        series.setData(datatableTrendline, false);
      }
    }
    return;
    // Here for legacy but had never worked since we return just above..!
    /*
    if (datatabledp.length != 0) {
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'dewpoint',
            name: $.t('Dew Point'),
            color: 'blue',
            yAxis: 0,
            tooltip: {
                valueSuffix: ' \u00B0' + this.configService.config.TempSign,
                valueDecimals: 1
            }
        }, false);
        series = chart.get('dewpoint');
        series.setData(datatabledp, false);
    }
    if (datatableba.length != 0) {
        chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'baro',
            name: $.t('Barometer'),
            color: 'pink',
            yAxis: 2,
            tooltip: {
                valueSuffix: ' hPa',
                valueDecimals: 1
            }
        }, false);
        series = chart.get('baro');
        series.setData(datatableba, false);
    }
    */
  }


  public AddDataToUtilityChart(data: GraphData, chart: Highcharts.Chart, switchtype: number, range: string) {

    const datatableEnergyUsed = [];
    const datatableEnergyGenerated = [];

    const datatableUsage1 = [];
    const datatableUsage2 = [];
    const datatableReturn1 = [];
    const datatableReturn2 = [];
    const datatableTotalUsage = [];
    const datatableTotalReturn = [];

    const datatableUsage1Prev = [];
    const datatableUsage2Prev = [];
    const datatableReturn1Prev = [];
    const datatableReturn2Prev = [];
    const datatableTotalUsagePrev = [];
    const datatableTotalReturnPrev = [];

    let bHaveFloat = false;

    let valueQuantity = 'Count';
    if (typeof data.ValueQuantity !== 'undefined') {
      valueQuantity = data.ValueQuantity;
    }

    const DividerWater = 1000;

    let valueUnits = '';
    if (typeof data.ValueUnits !== 'undefined') {
      valueUnits = data.ValueUnits;
    }

    const bHaveDelivered = (typeof data.delivered !== 'undefined');
    const bHavePrev = (typeof data.resultprev !== 'undefined');

    if (bHavePrev) {
      data.resultprev.forEach((item: GraphTempPoint) => {
        const cdate = Utils.GetPrevDateFromString(item.d);
        datatableUsage1Prev.push([cdate, Number(item.v)]);
        if (typeof item.v2 !== 'undefined') {
          datatableUsage2Prev.push([cdate, Number(item.v2)]);
        }
        if (bHaveDelivered) {
          datatableReturn1Prev.push([cdate, Number(item.r1)]);
          if (typeof item.r2 !== 'undefined') {
            datatableReturn2Prev.push([cdate, Number(item.r2)]);
          }
        }
        if (datatableUsage2Prev.length > 0) {
          datatableTotalUsagePrev.push([cdate, Number(item.v) + Number(item.v2)]);
        } else {
          datatableTotalUsagePrev.push([cdate, Number(item.v)]);
        }
        if (datatableUsage2Prev.length > 0) {
          datatableTotalReturnPrev.push([cdate, Number(item.r1) + Number(item.r2)]);
        } else {
          if (typeof item.r1 !== 'undefined') {
            datatableTotalReturnPrev.push([cdate, Number(item.r1)]);
          }
        }
      });
    }

    data.result.forEach((item: GraphTempPoint) => {
      if (range === 'day') {
        const cdate2 = Utils.GetUTCFromString(item.d);
        if (typeof item.v !== 'undefined') {
          if (switchtype !== 2) {
            const fValue = Number(item.v);
            if (fValue % 1 !== 0) {
              bHaveFloat = true;
            }
            datatableUsage1.push([cdate2, fValue]);
          } else {
            datatableUsage1.push([cdate2, Number(item.v) * DividerWater]);
          }
        }
        if (typeof item.v2 !== 'undefined') {
          datatableUsage2.push([cdate2, Number(item.v2)]);
        }
        if (bHaveDelivered) {
          datatableReturn1.push([cdate2, Number(item.r1)]);
          if (typeof item.r2 !== 'undefined') {
            datatableReturn2.push([cdate2, Number(item.r2)]);
          }
        }
        if (typeof item.eu !== 'undefined') {
          datatableEnergyUsed.push([cdate2, Number(item.eu)]);
        }
        if (typeof item.eg !== 'undefined') {
          datatableEnergyGenerated.push([cdate2, Number(item.eg)]);
        }
      } else {
        const cdate3 = Utils.GetDateFromString(item.d);
        if (switchtype !== 2) {
          datatableUsage1.push([cdate3, Number(item.v)]);
        } else {
          datatableUsage1.push([cdate3, Number(item.v) * DividerWater]);
        }
        if (typeof item.v2 !== 'undefined') {
          datatableUsage2.push([cdate3, Number(item.v2)]);
        }
        if (bHaveDelivered) {
          datatableReturn1.push([cdate3, Number(item.r1)]);
          if (typeof item.r2 !== 'undefined') {
            datatableReturn2.push([cdate3, Number(item.r2)]);
          }
        }
        if (datatableUsage2.length > 0) {
          datatableTotalUsage.push([cdate3, Number(item.v) + Number(item.v2)]);
        } else {
          datatableTotalUsage.push([cdate3, Number(item.v)]);
        }
        if (datatableUsage2.length > 0) {
          datatableTotalReturn.push([cdate3, Number(item.r1) + Number(item.r2)]);
        } else {
          if (typeof item.r1 !== 'undefined') {
            datatableTotalReturn.push([cdate3, Number(item.r1)]);
          }
        }
      }
    });

    let series;
    if ((switchtype === 0) || (switchtype === 4)) {

      // Electra Usage/Return
      if ((range === 'day') || (range === 'week')) {
        let totDecimals = 3;
        if (range === 'day') {
          // if (bHaveFloat === true) {
          //     totDecimals = 1;
          // } else {
          totDecimals = 0;
          // }
        }
        if (datatableEnergyUsed.length > 0) {
          if (datatableUsage2.length === 0) {
            // instant + counter type
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'eUsed',
              type: 'column',
              pointRange: 3600 * 1000, // 1 hour in ms
              zIndex: 5,
              animation: false,
              name: (switchtype === 0) ? this.translationService.t('Energy Usage') : this.translationService.t('Energy Generated'),
              tooltip: {
                valueSuffix: (range === 'week') ? ' kWh' : ' Wh',
                valueDecimals: totDecimals
              },
              color: 'rgba(3,190,252,0.8)',
              yAxis: 0,
              visible: datatableUsage2.length === 0
            }, false);
          } else {
            // p1 type
            chart.addSeries(<any>{
              id: 'eUsed',
              type: 'area',
              name: this.translationService.t('Energy Usage'),
              tooltip: {
                valueSuffix: (range === 'week') ? ' kWh' : ' Wh',
                valueDecimals: totDecimals
              },
              color: 'rgba(120,150,220,0.9)',
              fillOpacity: 0.2,
              yAxis: 0,
              visible: datatableUsage2.length === 0
            }, false);
          }
          series = chart.get('eUsed');
          series.setData(datatableEnergyUsed, false);
        }
        if (datatableEnergyGenerated.length > 0) {
          // p1 type
          chart.addSeries(<any>{
            id: 'eGen',
            type: 'area',
            name: this.translationService.t('Energy Returned'),
            tooltip: {
              valueSuffix: (range === 'week') ? ' kWh' : ' Wh',
              valueDecimals: totDecimals
            },
            color: 'rgba(120,220,150,0.9)',
            fillOpacity: 0.2,
            yAxis: 0,
            visible: false
          }, false);
          series = chart.get('eGen');
          series.setData(datatableEnergyGenerated, false);
        }
        if (datatableUsage1.length > 0) {
          if (datatableUsage2.length === 0) {
            if (datatableEnergyUsed.length === 0) {
              // counter type (no power)
              chart.addSeries(<Highcharts.SeriesColumnOptions>{
                id: 'usage1',
                name: (switchtype === 0) ? this.translationService.t('Energy Usage') :
                  this.translationService.t('Energy Generated'),
                tooltip: {
                  valueSuffix: (range === 'day') ? ' Wh' : ' kWh',
                  valueDecimals: totDecimals
                },
                color: 'rgba(3,190,252,0.8)',
                stack: 'susage',
                yAxis: 0
              }, false);
            } else {
              // instant + counter type
              chart.addSeries(<Highcharts.SeriesColumnOptions>{
                id: 'usage1',
                name: (switchtype === 0) ? this.translationService.t('Power Usage') : this.translationService.t('Power Generated'),
                zIndex: 10,
                type: (range === 'day') ? 'spline' : 'column', // power vs energy
                tooltip: {
                  valueSuffix: (range === 'day') ? ' Watt' : ' kWh',
                  valueDecimals: totDecimals
                },
                color: (range === 'day') ? 'rgba(255,255,0,0.8)' : 'rgba(3,190,252,0.8)', // yellow vs blue
                stack: 'susage',
                yAxis: (range === 'day' && chart.yAxis.length > 1) ? 1 : 0
              }, false);
            }
          } else {
            // p1 type
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'usage1',
              name: this.translationService.t('Usage') + ' 1',
              tooltip: {
                valueSuffix: (range === 'week') ? ' kWh' : ' Watt',
                valueDecimals: totDecimals
              },
              color: 'rgba(60,130,252,0.8)',
              stack: 'susage',
              yAxis: (range === 'week') ? 0 : 1
            }, false);
          }
          series = chart.get('usage1');
          series.setData(datatableUsage1, false);
        }
        if (datatableUsage2.length > 0) {
          // p1 type
          chart.addSeries(<Highcharts.SeriesColumnOptions>{
            id: 'usage2',
            name: this.translationService.t('Usage') + ' 2',
            tooltip: {
              valueSuffix: (range === 'week') ? ' kWh' : ' Watt',
              valueDecimals: totDecimals
            },
            color: 'rgba(3,190,252,0.8)',
            stack: 'susage',
            yAxis: (range === 'week') ? 0 : 1
          }, false);
          series = chart.get('usage2');
          series.setData(datatableUsage2, false);
        }
        if (bHaveDelivered) {
          if (datatableReturn1.length > 0) {
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'return1',
              name: this.translationService.t('Return') + ' 1',
              tooltip: {
                valueSuffix: (range === 'week') ? ' kWh' : ' Watt',
                valueDecimals: totDecimals
              },
              color: 'rgba(30,242,110,0.8)',
              stack: 'sreturn',
              yAxis: (range === 'week') ? 0 : 1
            }, false);
            series = chart.get('return1');
            series.setData(datatableReturn1, false);
          }
          if (datatableReturn2.length > 0) {
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'return2',
              name: this.translationService.t('Return') + ' 2',
              tooltip: {
                valueSuffix: (range === 'week') ? ' kWh' : ' Watt',
                valueDecimals: totDecimals
              },
              color: 'rgba(3,252,190,0.8)',
              stack: 'sreturn',
              yAxis: (range === 'week') ? 0 : 1
            }, false);
            series = chart.get('return2');
            series.setData(datatableReturn2, false);
          }
        }
      } else {
        // month/year, show total for now
        if (datatableTotalUsage.length > 0) {
          chart.addSeries(<Highcharts.SeriesColumnOptions>{
            id: 'usage',
            name: (switchtype === 0) ? this.translationService.t('Total Usage') :
              ((switchtype === 4) ? this.translationService.t('Total Generated') : this.translationService.t('Total Return')),
            zIndex: 2,
            tooltip: {
              valueSuffix: ' kWh',
              valueDecimals: 3
            },
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0
          }, false);
          series = chart.get('usage');
          series.setData(datatableTotalUsage, false);
          const trandLine1 = Utils.CalculateTrendLine(datatableTotalUsage);
          if (typeof trandLine1 !== 'undefined') {
            const datatableTrendlineUsage1 = [];

            datatableTrendlineUsage1.push([trandLine1.x0, trandLine1.y0]);
            datatableTrendlineUsage1.push([trandLine1.x1, trandLine1.y1]);

            chart.addSeries(<any>{
              id: 'usage_trendline',
              name: this.translationService.t('Trendline') + ' ' + ((switchtype === 0) ? this.translationService.t('Usage') :
                ((switchtype === 4) ? this.translationService.t('Generated') : this.translationService.t('Return'))),
              zIndex: 1,
              tooltip: {
                valueSuffix: ' kWh',
                valueDecimals: 3
              },
              color: 'rgba(252,3,3,0.8)',
              dashStyle: 'LongDash',
              yAxis: 0,
              visible: false
            }, false);
            series = chart.get('usage_trendline');
            series.setData(datatableTrendlineUsage1, false);
          }
        }
        if (bHaveDelivered) {
          if (datatableTotalReturn.length > 0) {
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'return',
              name: this.translationService.t('Total Return'),
              zIndex: 1,
              tooltip: {
                valueSuffix: ' kWh',
                valueDecimals: 3
              },
              color: 'rgba(3,252,190,0.8)',
              yAxis: 0
            }, false);
            series = chart.get('return');
            series.setData(datatableTotalReturn, false);
            const trandLine2 = Utils.CalculateTrendLine(datatableTotalReturn);
            if (typeof trandLine2 !== 'undefined') {
              const datatableTrendlineReturn = [];

              datatableTrendlineReturn.push([trandLine2.x0, trandLine2.y0]);
              datatableTrendlineReturn.push([trandLine2.x1, trandLine2.y1]);

              chart.addSeries(<any>{
                id: 'return_trendline',
                name: this.translationService.t('Trendline') + ' ' + this.translationService.t('Return'),
                zIndex: 1,
                tooltip: {
                  valueSuffix: ' kWh',
                  valueDecimals: 3
                },
                color: 'rgba(255,127,39,0.8)',
                dashStyle: 'LongDash',
                yAxis: 0,
                visible: false
              }, false);
              series = chart.get('return_trendline');
              series.setData(datatableTrendlineReturn, false);
            }
          }
        }
        if (datatableTotalUsagePrev.length > 0) {
          chart.addSeries(<Highcharts.SeriesColumnOptions>{
            id: 'usageprev',
            name: this.translationService.t('Past') + ' ' + ((switchtype === 0) ? this.translationService.t('Usage') :
              ((switchtype === 4) ? this.translationService.t('Generated') : this.translationService.t('Return'))),
            tooltip: {
              valueSuffix: ' kWh',
              valueDecimals: 3
            },
            color: 'rgba(190,3,252,0.8)',
            yAxis: 0,
            visible: false
          }, false);
          series = chart.get('usageprev');
          series.setData(datatableTotalUsagePrev, false);
        }
        if (bHaveDelivered) {
          if (datatableTotalReturnPrev.length > 0) {
            chart.addSeries(<Highcharts.SeriesColumnOptions>{
              id: 'returnprev',
              name: this.translationService.t('Past') + ' ' + this.translationService.t('Return'),
              tooltip: {
                valueSuffix: ' kWh',
                valueDecimals: 3
              },
              color: 'rgba(252,190,3,0.8)',
              yAxis: 0,
              visible: false
            }, false);
            series = chart.get('returnprev');
            series.setData(datatableTotalReturnPrev, false);
          }
        }
      }
    } else if (switchtype === 1) {
      // gas
      chart.addSeries(<Highcharts.SeriesColumnOptions>{
        id: 'gas',
        name: 'Gas',
        zIndex: 2,
        tooltip: {
          valueSuffix: ' m3',
          valueDecimals: 3
        },
        color: 'rgba(3,190,252,0.8)',
        yAxis: 0
      }, false);
      if ((range === 'month') || (range === 'year')) {
        const trandLine3 = Utils.CalculateTrendLine(datatableUsage1);
        if (typeof trandLine3 !== 'undefined') {
          const datatableTrendlineUsage3 = [];

          datatableTrendlineUsage3.push([trandLine3.x0, trandLine3.y0]);
          datatableTrendlineUsage3.push([trandLine3.x1, trandLine3.y1]);

          chart.addSeries(<any>{
            id: 'usage_trendline',
            name: 'Trendline ' + this.translationService.t('Gas'),
            zIndex: 1,
            tooltip: {
              valueSuffix: ' m3',
              valueDecimals: 3
            },
            color: 'rgba(252,3,3,0.8)',
            dashStyle: 'LongDash',
            yAxis: 0,
            visible: false
          }, false);
          series = chart.get('usage_trendline');
          series.setData(datatableTrendlineUsage3, false);
        }
        if (datatableUsage1Prev.length > 0) {
          chart.addSeries(<Highcharts.SeriesColumnOptions>{
            id: 'gasprev',
            name: this.translationService.t('Past') + ' ' + this.translationService.t('Gas'),
            tooltip: {
              valueSuffix: ' m3',
              valueDecimals: 3
            },
            color: 'rgba(190,3,252,0.8)',
            yAxis: 0,
            visible: false
          }, false);
          series = chart.get('gasprev');
          series.setData(datatableUsage1Prev, false);
        }
      }
      series = chart.get('gas');
      series.setData(datatableUsage1, false);
      chart.options.yAxis[0].title.text = 'Gas m3';
    } else if (switchtype === 2) {
      // water
      chart.addSeries(<Highcharts.SeriesColumnOptions>{
        id: 'water',
        name: 'Water',
        tooltip: {
          valueSuffix: ' Liter',
          valueDecimals: 0
        },
        color: 'rgba(3,190,252,0.8)',
        yAxis: 0
      }, false);
      chart.options.yAxis[0].title.text = 'Water Liter';
      series = chart.get('water');
      series.setData(datatableUsage1, false);
    } else if (switchtype === 3) {
      // counter
      chart.addSeries(<Highcharts.SeriesColumnOptions>{
        id: 'counter',
        name: valueQuantity,
        tooltip: {
          valueSuffix: ' ' + valueUnits,
          valueDecimals: 0
        },
        color: 'rgba(3,190,252,0.8)',
        yAxis: 0
      }, false);
      chart.options.yAxis[0].title.text = valueQuantity + ' ' + valueUnits;
      series = chart.get('counter');
      series.setData(datatableUsage1, false);
    }
  }

  public GetGraphUnit(uname: string): string {
    if (uname === this.translationService.t('Usage')) {
      return 'kWh';
    }
    if (uname === this.translationService.t('Usage') + ' 1') {
      return 'kWh';
    }
    if (uname === this.translationService.t('Usage') + ' 2') {
      return 'kWh';
    }
    if (uname === this.translationService.t('Return') + ' 1') {
      return 'kWh';
    }
    if (uname === this.translationService.t('Return') + ' 2') {
      return 'kWh';
    }
    if (uname === this.translationService.t('Gas')) {
      return 'm3';
    }
    if (uname === this.translationService.t('Past') + ' ' + this.translationService.t('Gas')) {
      return 'm3';
    }
    if (uname === this.translationService.t('Water')) {
      return 'm3';
    }
    if (uname === this.translationService.t('Power')) {
      return 'Watt';
    }
    if (uname === this.translationService.t('Total Usage')) {
      return 'kWh';
    }
    if (uname === this.translationService.t('Past') + ' ' + this.translationService.t('Usage')) {
      return 'kWh';
    }
    if (uname === this.translationService.t('Past') + ' ' + this.translationService.t('Return')) {
      return 'kWh';
    }
    if (uname === this.translationService.t('Return')) {
      return 'kWh';
    }
    if (uname === this.translationService.t('Generated')) {
      return 'kWh';
    }

    return '?';
  }

  public AddDataToCurrentChart(data: GraphData, chart: Highcharts.Chart, switchtype: number, isday: number) {
    const datatablev1 = [];
    const datatablev2 = [];
    const datatablev3 = [];
    const datatablev4 = [];
    const datatablev5 = [];
    const datatablev6 = [];
    data.result.forEach((item: GraphTempPoint) => {
      if (isday === 1) {
        datatablev1.push([Utils.GetUTCFromString(item.d), Number(item.v1)]);
        datatablev2.push([Utils.GetUTCFromString(item.d), Number(item.v2)]);
        datatablev3.push([Utils.GetUTCFromString(item.d), Number(item.v3)]);
      } else {
        datatablev1.push([Utils.GetDateFromString(item.d), Number(item.v1)]);
        datatablev2.push([Utils.GetDateFromString(item.d), Number(item.v2)]);
        datatablev3.push([Utils.GetDateFromString(item.d), Number(item.v3)]);
        datatablev4.push([Utils.GetDateFromString(item.d), Number(item.v4)]);
        datatablev5.push([Utils.GetDateFromString(item.d), Number(item.v5)]);
        datatablev6.push([Utils.GetDateFromString(item.d), Number(item.v6)]);
      }
    });

    let series;
    if (switchtype === 0) {
      // Current (A)
      if (isday === 1) {
        if (data.haveL1) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1',
            name: 'Current_L1',
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current1');
          series.setData(datatablev1, false);
        }
        if (data.haveL2) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2',
            name: 'Current_L2',
            color: 'rgba(252,190,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current2');
          series.setData(datatablev2, false);
        }
        if (data.haveL3) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3',
            name: 'Current_L3',
            color: 'rgba(190,252,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current3');
          series.setData(datatablev3, false);
        }
      } else {
        // month/year
        if (data.haveL1) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1min',
            name: 'Current_L1_Min',
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1max',
            name: 'Current_L1_Max',
            color: 'rgba(3,252,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current1min');
          series.setData(datatablev1, false);
          series = chart.get('current1max');
          series.setData(datatablev2, false);
        }
        if (data.haveL2) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2min',
            name: 'Current_L2_Min',
            color: 'rgba(252,190,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2max',
            name: 'Current_L2_Max',
            color: 'rgba(252,3,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current2min');
          series.setData(datatablev3, false);
          series = chart.get('current2max');
          series.setData(datatablev4, false);
        }
        if (data.haveL3) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3min',
            name: 'Current_L3_Min',
            color: 'rgba(190,252,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3max',
            name: 'Current_L3_Max',
            color: 'rgba(112,146,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' A',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current3min');
          series.setData(datatablev5, false);
          series = chart.get('current3max');
          series.setData(datatablev6, false);
        }
      }
      chart.options.yAxis[0].title.text = 'Current (A)';
    } else if (switchtype === 1) {
      // Watt
      if (isday === 1) {
        if (data.haveL1) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1',
            name: this.translationService.t('Usage') + ' L1',
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current1');
          series.setData(datatablev1, false);
        }
        if (data.haveL2) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2',
            name: this.translationService.t('Usage') + ' L2',
            color: 'rgba(252,190,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current2');
          series.setData(datatablev2, false);
        }
        if (data.haveL3) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3',
            name: this.translationService.t('Usage') + ' L3',
            color: 'rgba(190,252,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current3');
          series.setData(datatablev3, false);
        }
      } else {
        if (data.haveL1) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1min',
            name: this.translationService.t('Usage') + ' L1_Min',
            color: 'rgba(3,190,252,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current1max',
            name: this.translationService.t('Usage') + ' L1_Max',
            color: 'rgba(3,252,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current1min');
          series.setData(datatablev1, false);
          series = chart.get('current1max');
          series.setData(datatablev2, false);
        }
        if (data.haveL2) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2min',
            name: this.translationService.t('Usage') + ' L2_Min',
            color: 'rgba(252,190,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current2max',
            name: this.translationService.t('Usage') + ' L2_Max',
            color: 'rgba(252,3,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current2min');
          series.setData(datatablev3, false);
          series = chart.get('current2max');
          series.setData(datatablev4, false);
        }
        if (data.haveL3) {
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3min',
            name: this.translationService.t('Usage') + ' L3_Min',
            color: 'rgba(190,252,3,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          chart.addSeries(<Highcharts.SeriesSplineOptions>{
            id: 'current3max',
            name: this.translationService.t('Usage') + ' L3_Max',
            color: 'rgba(112,146,190,0.8)',
            yAxis: 0,
            tooltip: {
              valueSuffix: ' Watt',
              valueDecimals: 1
            }
          }, false);
          series = chart.get('current3min');
          series.setData(datatablev5, false);
          series = chart.get('current3max');
          series.setData(datatablev6, false);
        }
      }
      chart.options.yAxis[0].title.text = 'Usage (Watt)';
    }
  }

}
