import {ApiService} from '../_shared/_services/api.service';
import {Observable} from 'rxjs';
import {GraphData, GraphTempPoint} from '../_shared/_models/graphdata';
import {map} from 'rxjs/operators';
import {ReportHelpers} from '../_shared/_utils/report-helpers';
import {ConfigService} from '../_shared/_services/config.service';
import {Injectable} from '@angular/core';

@Injectable()
export class DeviceTemperatureReportDataService {

  constructor(
    private apiService: ApiService,
    private configService: ConfigService) {
  }

  fetch(deviceIdx: string, year: number, month?: number): Observable<DeviceReportData> {
    const params: any = {
      sensor: 'temp',
      range: 'year',
      idx: deviceIdx,
      actyear: year.toString()
    };
    if (month) {
      params.actmonth = month.toString();
    }

    return this.apiService.callApi<GraphData>('graph', params).pipe(
      map(response => {
        if (!response.result || !response.result.length) {
          return null;
        }

        const isHumidityAvailable = (typeof response.result[0].hu !== 'undefined');
        const isOnlyHumidity = ((typeof response.result[0].ta === 'undefined') && isHumidityAvailable);
        const isBaroAvailable = (typeof response.result[0].ba !== 'undefined');

        const data = this.getGroupedData(response.result, isOnlyHumidity);
        const source = month
          ? data.years[year].months.find(function (item) {
            return item.date === month;
          })
          : data.years[year];

        if (!source) {
          return null;
        }

        return <DeviceReportData>{
          isOnlyHumidity: isOnlyHumidity,
          isHumidityAvailable: isHumidityAvailable,
          isBaroAvailable: isBaroAvailable,
          min: source.min,
          minDate: source.minDate,
          max: source.max,
          maxDate: source.maxDate,
          degreeDays: source.degreeDays,
          items: month ? source.days : source.months
        };
      }));
  }

  private getGroupedData(data: Array<GraphTempPoint>, isOnlyHumidity: boolean): any {
    // FIXME type it for safety
    const result = {
      years: {}
    };

    data.forEach(item => {
      const month = parseInt(item.d.substring(5, 7), 10);
      const year = parseInt(item.d.substring(0, 4), 10);
      const day = parseInt(item.d.substring(8, 10), 10);

      if (!result.years[year]) {
        result.years[year] = {
          months: {}
        };
      }

      if (!result.years[year].months[month]) {
        result.years[year].months[month] = {
          date: month,
          days: {}
        };
      }

      result.years[year].months[month].days[day] = !isOnlyHumidity
        ? {
          date: item.d,
          humidity: Number(item.hu) || undefined,
          baro: Number(item.ba) || undefined,
          min: item.tm,
          max: item.te,
          avg: item.ta
        }
        : {
          date: item.d,
          min: Number(item.hu),
          max: Number(item.hu),
          avg: Number(item.hu)
        };
    });

    Object.keys(result.years).forEach(year => {
      const yearsData = result.years[year];
      yearsData.months = Object.values(yearsData.months);

      yearsData.months.sort((a, b) => {
        return a.date < b.date ? -1 : 1;
      });

      yearsData.months.forEach(month => {
        month.days = Object.values(month.days);

        month.days.sort((a, b) => {
          return a.date < b.date ? -1 : 1;
        });

        month.days = ReportHelpers.addTrendData(month.days, 'avg');

        const stats = month.days.reduce((acc, item) => {
          if (!acc.min || acc.min.value > item.min) {
            acc.min = {
              value: item.min,
              date: item.date
            };
          }

          if (!acc.max || acc.max.value < item.max) {
            acc.max = {
              value: item.max,
              date: item.date
            };
          }

          if (item.avg < this.configService.config.DegreeDaysBaseTemperature) {
            acc.degreeDays = (acc.degreeDays || 0) + (this.configService.config.DegreeDaysBaseTemperature - item.avg);
          }

          acc.total = (acc.total || 0) + item.avg;
          acc.totalHumidity = (acc.totalHumidity || 0) + item.humidity;
          acc.totalBaro = (acc.totalBaro || 0) + item.baro;

          return acc;
        }, {});

        month.min = stats.min.value;
        month.minDate = stats.min.date;
        month.max = stats.max.value;
        month.maxDate = stats.max.date;
        month.avg = (stats.total / month.days.length);
        month.degreeDays = stats.degreeDays || 0;
        month.humidity = stats.totalHumidity ? (stats.totalHumidity / month.days.length) : undefined;
        month.baro = stats.totalBaro ? (stats.totalBaro / month.days.length) : undefined;
      });

      yearsData.months = ReportHelpers.addTrendData(yearsData.months, 'avg');

      const statsBis = this.getGroupStats(yearsData.months);

      yearsData.min = statsBis.min.value;
      yearsData.minDate = statsBis.min.date;
      yearsData.max = statsBis.max.value;
      yearsData.maxDate = statsBis.max.date;
      yearsData.degreeDays = statsBis.degreeDays || 0;
      yearsData.avg = (statsBis.total / yearsData.months.length);
    });

    return result;
  }

  // FIXME type
  private getGroupStats(values: Array<any>): any {
    return values.reduce((acc, item) => {
      if (!acc.min || acc.min.value > item.min) {
        acc.min = {
          value: item.min,
          date: item.minDate
        };
      }

      if (!acc.max || acc.max.value < item.max) {
        acc.max = {
          value: item.max,
          date: item.maxDate
        };
      }

      acc.degreeDays = (acc.degreeDays || 0) + item.degreeDays;
      acc.total = (acc.total || 0) + item.avg;

      return acc;
    }, {});
  }

}

export interface DeviceReportData {
  isOnlyHumidity: boolean;
  isHumidityAvailable: boolean;
  isBaroAvailable: boolean;
  min: number;
  minDate: string;
  max: number;
  maxDate: string;
  degreeDays: number;
  items: Array<any>; // FIXME type
}
